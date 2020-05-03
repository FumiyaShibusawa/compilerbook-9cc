#include "9cc.h"

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
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
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
  char *ident = calloc(1, sizeof(token->len));
  strncpy(ident, token->str, token->len);
  ident[token->len] = '\0';
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

Node *new_node_num(int val, Token *tok)
{
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

LVar *find_lvar(Token *token)
{
  for (LVar *var = locals; var; var = var->next)
    if (token->len == var->len &&
        !memcmp(token->str, var->name, var->len))
      return var;
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

Function *function(void)
{
  locals = NULL;
  char *name = expect_ident();
  expect("(");
  expect(")");
  expect("{");
  Node head = {};
  Node *cur = &head;
  while (!consume("}"))
  {
    cur->next = stmt();
    cur = cur->next;
  }

  Function *fn = calloc(1, sizeof(Function));
  fn->name = name;
  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

Node *stmt()
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
  if (consume(";"))
  {
    node = new_node(ND_NULL, tok);
    return node;
  }

  node = expr();
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

// add = mul ("+" mul | "-" mul)*
Node *add()
{
  Node *node = mul();
  Token *tok;
  for (;;)
  {
    if (tok = consume("+"))
      node = new_binary(ND_ADD, node, mul(), tok);
    else if (tok = consume("-"))
      node = new_binary(ND_SUB, node, mul(), tok);
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
    return primary();
  else if (tok = consume("-"))
    return new_binary(ND_SUB, new_node_num(0, tok), primary(), tok);
  else
    return primary();
}

Node *func_args()
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

Node *primary()
{
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }
  Token *tok;
  if (tok = consume_ident())
  {
    if (consume("("))
    {
      Node *node = new_node(ND_FUNCALL, tok);
      node->funcname = calloc(1, sizeof(tok->len));
      strncpy(node->funcname, tok->str, tok->len);
      node->funcname[tok->len] = '\0';
      node->args = func_args();
      return node;
    }
    LVar *lvar = find_lvar(tok);
    if (!lvar)
    {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals; // 連結リストの現在の先頭を自身の次のリストにする
      lvar->name = tok->str;
      lvar->len = tok->len;
      locals = lvar; // 連結リストの新しい先頭を自身にする
    }
    Node *node = new_node(ND_LVAR, tok);
    node->var = lvar;
    return node;
  }
  else
  {
    tok = token;
    if (tok->kind != TK_NUM)
      error_tok(tok, "expected expression");
    return new_node_num(expect_number(), tok);
  }
}
