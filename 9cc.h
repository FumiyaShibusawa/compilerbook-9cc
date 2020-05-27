#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

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
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool start_with(char *lhs, char *rhs);
bool is_alpha(char c);
Token *tokenize();

/* parse.c */

typedef enum
{
  ND_ASSIGN,   // =
  ND_EQ,       // ==
  ND_NE,       // !=
  ND_LE,       // <=
  ND_LT,       // <
  ND_ADD,      // num + num
  ND_PTR_ADD,  // ptr + num || num + ptr
  ND_SUB,      // num - num
  ND_PTR_SUB,  // ptr - num || num - ptr
  ND_PTR_DIFF, // ptr - ptr
  ND_MUL,      // *
  ND_DIV,      // /
  ND_ADDR,     // unary &
  ND_DEREF,    // unary *
  ND_VAR,      // 変数
  ND_NUM,      // Integer
  ND_RETURN,   // return keyword
  ND_IF,       // if keyword
  ND_WHILE,    // while keyword
  ND_FOR,      // for keyword
  ND_BLOCK,    // block
  ND_NULL,     // empty statement
  ND_FUNCALL,  // function call
  ND_EXPR_STMT // expression statement
} NodeKind;

typedef struct Var Var;

struct Var
{
  char *name;
  Type *ty;
  int len;
  bool is_local; // local or global

  // local variable
  int offset; // offset from RBP
};

typedef struct VarList VarList;

struct VarList
{
  VarList *next;
  Var *var;
};

typedef struct Node Node;
struct Node
{
  NodeKind kind;
  Node *lhs;
  Node *rhs;

  // for "while", "if", "for"
  Node *init;
  Node *cond;
  Node *then;
  Node *els;
  Node *inc;

  Node *body; // statements in block
  Node *next; // next node
  Type *ty;   // Type, e.g. int, pointer to int
  Token *tok;

  // function call
  char *funcname;
  Node *args;

  int val;  // ND_NUMの時のみ使う
  Var *var; // ND_VARの時のみ使い、変数に関する情報を格納する
};

Token *peek(char *op);
Token *consume(char *op);
Token *consume_ident(void);
void expect(char *op);
int expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Node *new_node(NodeKind kind, Token *tok);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_unary(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);
Node *new_node_lvar(Var *lvar, Token *tok);
Var *new_var(char *name, Type *ty, bool is_local);
Var *find_var(Token *tok);

// NODE: 四則演算, 比較表現, 変数は以下で表現される。これをC関数に落とし込む。
//       program    = stmt*
//       stmt       = expr ";"
//                    | return expr ";"
//                    | "if" "(" expr ")" stmt ("else" stmt)?
//                    | "while" "(" expr ")" stmt
//                    | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                    | "{" stmt* "}"
//       expr       = assign
//       assign     = equality ("=" assign)?
//       equality   = relational ("==" relational | "!=" relational)*
//       relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//       add        = mul ("+" mul | "-" mul)*
//       mul        = unary ("*" unary | "/" unary)*
//       unary      = ("+" | "-" | "*" | "&")? unary
//       primary    = num | ident args? | "(" expr ")" | "sizeof" unary
//       args       = "(" ")"

typedef struct Function Function;

struct Function
{
  Function *next;
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

typedef struct
{
  VarList *globals;
  Function *fns;
} Program;

Program *program(void);
void codegen(Program *prog);
Function *function(void);
Node *declaration(void);
Node *stmt(void);
Node *stmt2(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *postfix(void);
Node *primary(void);

/* type.c */

typedef enum
{
  TY_INT,
  TY_PTR,
  TY_ARRAY
} TypeKind;

struct Type
{
  TypeKind kind;
  size_t size;
  Type *base;
  size_t array_len;
};

extern Type *int_type;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, size_t size);
void add_type(Node *node);

/* codegen.c */

void gen(Node *node);

/* main.c */

char *user_input;

int main(int argc, char **argv);
