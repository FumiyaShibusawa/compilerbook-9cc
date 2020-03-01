#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* tokenize.c */

// NOTE: enum は定数を一括で宣言できる型
typedef enum
{
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
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
  int len;        // トークンの長さ ※ 数値の長さではない
};

Token *token;
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool start_with(char *lhs, char *rhs);
bool is_alpha(char c);
Token *tokenize();

/* parse.c */

typedef enum
{
  ND_ASSIGN, // =
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LE,     // <=
  ND_LT,     // <
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_LVAR,   // ローカル変数
  ND_NUM,    // Integer
  ND_RETURN, // return keyword
  ND_IF,     // if keyword
  ND_WHILE   // while keyword
} NodeKind;

typedef struct Node Node;
struct Node
{
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  Node *cond;
  Node *then;
  Node *els;
  int val;    // ND_NUMの時のみ使う
  int offset; // ND_LVARの時のみ使い、ベースポインタからどのくらい離れているかを示す
};

bool consume(char *op);
Token *consume_ident(void);
void expect(char *op);
int expect_number(void);
bool at_eof(void);
Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// NODE: 四則演算, 比較表現, 変数は以下で表現される。これをC関数に落とし込む。
//       program = stmt*
//       stmt    = expr ";"
//                 | return expr ";"
//                 | "if" "(" expr ")" stmt ("else" stmt)?
//                 | "while" "(" expr ")" stmt
//       expr    = assign
//       assign  = equality ("=" assign)?
//       equality = relational ("==" relational | "!=" relational)*
//       relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//       add    = mul ("+" mul | "-" mul)*
//       mul     = unary ("*" unary | "/" unary)*
//       unary   = ("+" | "-")? primary
//       primary = num | ident | "(" expr ")"
Node *code[100];
void *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

typedef struct LVar LVar;

struct LVar
{
  LVar *next;
  char *name;
  int len;
  int offset;
};

LVar *locals;
LVar *find_lvar(Token *token);

/* codegen.c */

void gen(Node *node);

/* main.c */

char *user_input;

int main(int argc, char **argv);
