#include "9cc.h"

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

Node *new_node_lvar(LVar *lvar, Token *tok)
{
  Node *node = new_node(ND_LVAR, tok);
  node->var = lvar;
  return node;
}

LVar *new_lvar(char *name, Type *ty)
{
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->name = name;
  lvar->ty = ty;
  LVarList *vl = calloc(1, sizeof(LVarList));
  vl->var = lvar;
  vl->next = locals;
  locals = vl;
  return lvar;
}

LVar *find_lvar(Token *tok)
{
  for (LVarList *vl = locals; vl; vl = vl->next)
    if (strlen(vl->var->name) == tok->len &&
        !strncmp(tok->str, vl->var->name, tok->len))
      return vl->var;
  return NULL;
}

Function *program(void)
{
  Function head = {};
  Function *cur = &head;
  while (!at_eof())
  {
    cur->next = function();
    cur = cur->next;
  }
  return head.next;
}

Type *basetype(void)
{
  expect("int");
  Type *ty = int_type;
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

LVarList *read_func_param(void)
{
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);

  LVarList *vl = calloc(1, sizeof(LVarList));
  vl->var = new_lvar(name, ty);
  return vl;
}

LVarList *read_func_params(void)
{
  if (consume(")"))
    return NULL;

  LVarList *head = read_func_param();
  LVarList *cur = head;

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
  LVar *var = new_lvar(name, ty);

  if (consume(";"))
    return new_node(ND_NULL, tok);

  expect("=");
  Node *lhs = new_node_lvar(var, tok);
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
  if (tok = peek("int"))
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

Node *primary()
{
  Token *tok;

  if (consume("("))
  {
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

    LVar *lvar = find_lvar(tok);
    if (!lvar)
      error_tok(tok, "undefined variable");
    return new_node_lvar(lvar, tok);
  }

  tok = token;
  if (tok->kind != TK_NUM)
    error_tok(tok, "expected expression");
  return new_node_num(expect_number(), tok);
}
