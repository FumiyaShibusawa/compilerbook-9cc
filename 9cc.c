#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOTE: enum は定数を一括で宣言できる型
typedef enum
{
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF       // 入力の終わりを表すトークン
} TokenKind;

/*
  NOTE: typedef で struct Token を通常の名前空間で変数宣言できるようにする
        struct Token { ... };
        struct Token token; => OK
        Token token; => Error
        typedef struct Token token_t;
        token_t token; => OK
*/
typedef struct Token Token;

struct Token
{
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
};

char *user_input;

void error_at(char *loc, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 現在着目しているトークン
Token *token;

// エラーを報告するための関数
// printfと同じ引数を取る
// NOTE: ... は引数がいくつあってもよいという意味。
//       int main() === int main(...) != int main(void)
void error(char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char op)
{
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op)
{
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(void)
{
  if (token->kind != TK_NUM)
    error_at(token->str, "数値ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof(void)
{
  return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列user_inputをトークナイズしてそれを返す
Token *tokenize()
{
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p)
  {
    // 空文字はスキップ
    if (isspace(*p))
    {
      p++;
      continue;
    }

    if (strchr("+-*/()", *p))
    {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p))
    {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

// parser

typedef enum
{
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM  // Integer
} NodeKind;

typedef struct Node Node;
struct Node
{
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};

Node *new_node(NodeKind kind)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
};

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
};

Node *new_num(int val)
{
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// NODE: 四則演算は以下の3つの文法で表現できる。これをC関数に落とし込む。
//       expr    = mul ("+" mul | "-" mul)*
//       mul     = primary ("*" primary | "/" primary)*
//       primary = num | "(" expr ")"
Node *expr();
Node *mul();
Node *primary();

Node *expr()
{
  Node *node = mul();
  for (;;)
  {
    if (consume('+'))
      node = new_binary(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul()
{
  Node *node = primary();
  for (;;)
  {
    if (consume('*'))
      node = new_binary(ND_MUL, node, primary());
    else if (consume('/'))
      node = new_binary(ND_DIV, node, primary());
    else
      return node;
  }
}

Node *primary()
{
  if (consume('('))
  {
    Node *node = expr();
    expect(')');
    return node;
  }

  return new_num(expect_number());
}

// Assembly code generator
void gen(Node *node)
{
  if (node->kind == ND_NUM)
  {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  default:
    error("パースできません: node->kind: %d, node->str: %s\n", node->kind, node->val);
  }

  printf("  push rax\n");
}

int main(int argc, char **argv)
{
  if (argc != 2)
    error("%s: 引数の個数が正しくありません\n", argv[0]);

  user_input = argv[1];
  // トークナイズする
  token = tokenize();
  Node *node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(node);
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
