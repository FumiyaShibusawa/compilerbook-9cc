#include "9cc.h"

static VarList *locals;
static VarList *globals;

Token *peek(char *op)
{
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return NULL;
  return token;
}

// 次のトークンが期待している記号のときには、トークンを返して、
// トークンを1つ読み進める。それ以外の場合にはNULLを返す。
Token *consume(char *op)
{
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

Token *consume_ident(void)
{
  if (token->kind != TK_IDENT)
    return false;
  Token *tok = token;
  token = token->next;
  return tok;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op)
{
  if (!peek(op))
    error_tok(token, "expected \"%s\"", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(void)
{
  if (token->kind != TK_NUM)
    error_tok(token, "expected a number");
  int val = token->val;
  token = token->next;
  return val;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてその値を返す。
// それ以外の場合にはエラーを報告する。
char *expect_ident(void)
{
  if (token->kind != TK_IDENT)
    error_tok(token, "expected an identifier");
  char *ident = strndup(token->str, token->len);
  token = token->next;
  return ident;
}

bool at_eof(void)
{
  return token->kind == TK_EOF;
}

Node *new_node(NodeKind kind, Token *tok)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
};

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok)
{
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
};

Node *new_unary(NodeKind kind, Node *lhs, Token *tok)
{
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  return node;
};

Node *new_node_num(int val, Token *tok)
{
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

Node *new_node_var(Var *lvar, Token *tok)
{
  Node *node = new_node(ND_VAR, tok);
  node->var = lvar;
  return node;
}

Var *new_var(char *name, Type *ty, bool is_local)
{
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = ty;
  var->is_local = is_local;
  return var;
}

static Var *new_lvar(char *name, Type *ty)
{
  Var *var = new_var(name, ty, true);
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}

static Var *new_gvar(char *name, Type *ty)
{
  Var *var = new_var(name, ty, false);
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = globals;
  globals = vl;
  return var;
}

static char *new_label(void)
{
  static int cnt = 0;
  char buf[20];
  sprintf(buf, ".L.data.%d", cnt++);
  return strndup(buf, 20);
}

Var *find_var(Token *tok)
{
  for (VarList *vl = locals; vl; vl = vl->next)
    if (strlen(vl->var->name) == tok->len &&
        !strncmp(tok->str, vl->var->name, tok->len))
      return vl->var;

  for (VarList *vl = globals; vl; vl = vl->next)
    if (strlen(vl->var->name) == tok->len &&
        !strncmp(tok->str, vl->var->name, tok->len))
      return vl->var;
  return NULL;
}

static Type *basetype(void)
{
  Type *ty;
  if (consume("char"))
    ty = char_type;
  else
  {
    expect("int");
    ty = int_type;
  }

  while (consume("*"))
    ty = pointer_to(ty);
  return ty;
}

static Type *read_type_suffix(Type *base)
{
  if (!consume("["))
    return base;
  int sz = expect_number();
  expect("]");
  base = read_type_suffix(base);
  return array_of(base, sz);
}

static bool is_function(void)
{
  // 一旦トークンを退避して先読みしたあと元に戻すという手順を踏む
  Token *tok = token;
  basetype();
  bool isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
};

static void global_var(void)
{
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  expect(";");
  new_gvar(name, ty);
};

Program *program(void)
{
  Function head = {};
  Function *cur = &head;
  globals = NULL;

  while (!at_eof())
  {
    if (is_function())
    {
      cur->next = function();
      cur = cur->next;
    }
    else
      global_var();
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->globals = globals;
  prog->fns = head.next;
  return prog;
}

VarList *read_func_param(void)
{
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = new_lvar(name, ty);
  return vl;
}

VarList *read_func_params(void)
{
  if (consume(")"))
    return NULL;

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")"))
  {
    expect(",");
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

Function *function(void)
{
  locals = NULL;
  Function *fn = calloc(1, sizeof(Function));
  basetype();
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params();
  expect("{");
  Node head = {};
  Node *cur = &head;
  while (!consume("}"))
  {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

Node *declaration(void)
{
  Token *tok = token;
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  Var *var = new_lvar(name, ty);

  if (consume(";"))
    return new_node(ND_NULL, tok);

  expect("=");
  Node *lhs = new_node_var(var, tok);
  Node *rhs = expr();
  expect(";");
  Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
  return new_unary(ND_EXPR_STMT, node, tok);
}

Node *read_expr_stmt(void)
{
  Token *tok = token;
  return new_unary(ND_EXPR_STMT, expr(), tok);
}

static bool is_typename(void)
{
  return peek("char") || peek("int");
}

Node *stmt(void)
{
  Node *node = stmt2();
  add_type(node);
  return node;
}

Node *stmt2(void)
{
  Node *node;
  Token *tok;
  if (tok = consume("return"))
  {
    node = new_node(ND_RETURN, tok);
    node->lhs = expr();
    expect(";");
    return node;
  }
  if (tok = consume("if"))
  {
    node = new_node(ND_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }
  if (tok = consume("while"))
  {
    node = new_node(ND_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }
  if (tok = consume("for"))
  {
    node = new_node(ND_FOR, tok);
    expect("(");
    if (!consume(";"))
    {
      node->init = expr();
      expect(";");
    }
    if (!consume(";"))
    {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")"))
    {
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
    return node;
  }
  if (tok = consume("{"))
  {
    node = new_node(ND_BLOCK, tok);
    Node head = {};
    Node *cur = &head;
    while (!consume("}"))
    {
      cur->next = stmt();
      cur = cur->next;
    }
    node->body = head.next;
    return node;
  }
  if (is_typename())
    return declaration();

  node = read_expr_stmt();
  expect(";");
  return node;
}

Node *expr()
{
  return assign();
}

Node *assign()
{
  Node *node = equality();
  Token *tok;
  for (;;)
  {
    if (tok = consume("="))
      node = new_binary(ND_ASSIGN, node, assign(), tok);
    else
      return node;
  }
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
  Node *node = relational();
  Token *tok;
  for (;;)
  {
    if (tok = consume("=="))
      node = new_binary(ND_EQ, node, relational(), tok);
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational(), tok);
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational()
{
  Node *node = add();
  Token *tok;
  for (;;)
  {
    if (tok = consume("<"))
      node = new_binary(ND_LT, node, add(), tok);
    else if (consume("<="))
      node = new_binary(ND_LE, node, add(), tok);
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node, tok);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node, tok);
    else
      return node;
  }
}

Node *new_add(Node *lhs, Node *rhs, Token *tok)
{
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_ADD, lhs, rhs, tok);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_ADD, lhs, rhs, tok);
  if (is_integer(lhs->ty) && rhs->ty->base)
    return new_binary(ND_PTR_ADD, rhs, lhs, tok);
  error_tok(tok, "invalid operands");
}

Node *new_sub(Node *lhs, Node *rhs, Token *tok)
{
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_SUB, lhs, rhs, tok);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_SUB, lhs, rhs, tok);
  if (lhs->ty->base && rhs->ty->base)
    return new_binary(ND_PTR_DIFF, lhs, rhs, tok);
  error_tok(tok, "invalid operands");
}

// add = mul ("+" mul | "-" mul)*
Node *add()
{
  Node *node = mul();
  Token *tok;
  for (;;)
  {
    if (tok = consume("+"))
      node = new_add(node, mul(), tok);
    else if (tok = consume("-"))
      node = new_sub(node, mul(), tok);
    else
      return node;
  }
}

Node *mul()
{
  Node *node = unary();
  Token *tok;
  for (;;)
  {
    if (tok = consume("*"))
      node = new_binary(ND_MUL, node, unary(), tok);
    else if (tok = consume("/"))
      node = new_binary(ND_DIV, node, unary(), tok);
    else
      return node;
  }
}

Node *unary()
{
  Token *tok;
  if (consume("+"))
    return unary();
  if (tok = consume("-"))
    return new_binary(ND_SUB, new_node_num(0, tok), unary(), tok);
  if (tok = consume("&"))
    return new_unary(ND_ADDR, unary(), tok);
  if (tok = consume("*"))
    return new_unary(ND_DEREF, unary(), tok);
  return postfix();
}

static Node *func_args()
{
  if (consume(")"))
    return NULL;

  Node *head = assign();
  Node *cur = head;
  while (consume(","))
  {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}

Node *postfix(void)
{
  Node *node = primary();
  Token *tok;

  while (tok = consume("["))
  {
    Node *exp = new_add(node, expr(), tok);
    expect("]");
    node = new_unary(ND_DEREF, exp, tok);
  }
  return node;
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
// statement expression is a GNU C extension
static Node *stmt_expr(Token *tok)
{
  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = stmt();
  Node *cur = node->body;

  while (!consume("}"))
  {
    cur->next = stmt();
    cur = cur->next;
  }
  expect(")");

  if (cur->kind != ND_EXPR_STMT)
    error_tok(tok, "stmt expre returning void is not supported");
  memcpy(cur, cur->lhs, sizeof(Node));
  return node;
}

Node *primary()
{
  Token *tok;

  if (consume("("))
  {
    if (consume("{"))
      return stmt_expr(tok);

    Node *node = expr();
    expect(")");
    return node;
  }

  if (tok = consume("sizeof"))
  {
    Node *node = unary();
    add_type(node);
    return new_node_num(node->ty->size, tok);
  }

  if (tok = consume_ident())
  {
    if (consume("("))
    {
      Node *node = new_node(ND_FUNCALL, tok);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    Var *var = find_var(tok);
    if (!var)
      error_tok(tok, "undefined variable");
    return new_node_var(var, tok);
  }

  tok = token;
  if (tok->kind == TK_STR)
  {
    token = token->next;
    Type *ty = array_of(char_type, tok->cont_len);
    Var *var = new_gvar(new_label(), ty);
    var->contents = tok->contents;
    var->cont_len = tok->cont_len;
    return new_node_var(var, tok);
  }

  if (tok->kind != TK_NUM)
    error_tok(tok, "expected expression");
  return new_node_num(expect_number(), tok);
}
