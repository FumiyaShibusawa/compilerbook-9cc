#include "9cc.h"

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

// reports an error message in the following format and exit.
// foo.c:10: x = y + 1;
//               ^ <error message here>
void verror_at(char *loc, char *fmt, va_list ap)
{
  // エラー箇所の行の開始地点と終了地点を取得する
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end = '\n')
    end++;

  // エラー箇所が何行目かを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->str, fmt, ap);
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool starts_with(char *lhs, char *rhs)
{
  return memcmp(lhs, rhs, strlen(rhs)) == 0;
}

bool is_alpha(char c)
{
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         c == '_';
}

bool is_alnum(char c)
{
  return is_alpha(c) && ('0' <= c && c <= '9') || ('_' == c);
}

char *starts_with_reserved(char *p)
{
  char *kw[] = {"if", "else", "return", "while", "for", "int", "sizeof", "char"};
  int kw_size = sizeof(kw) / sizeof(kw[0]);
  for (size_t i = 0; i < kw_size; i++)
  {
    int len = strlen(kw[i]);
    if (starts_with(p, kw[i]) && !is_alnum(p[len]))
      return kw[i];
  }
  return NULL;
}

static char get_escape_char(char c)
{
  switch (c)
  {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case 'e':
    return 27;
  case '0':
    return 0;
  default:
    return c;
  }
}

static Token *read_string_literal(Token *cur, char *start)
{
  char *p = start + 1;
  char buf[1024];
  int len = 0;

  for (;;)
  {
    if (len == sizeof(buf))
      error_at(start, "string literal too large");
    if (*p == '\0')
      error_at(start, "unclosed string literal");
    if (*p == '"')
      break;

    if (*p == '\\')
    {
      p++;
      buf[len++] = get_escape_char(*p++);
    }
    else
      buf[len++] = *p++;
  }

  Token *tok = new_token(TK_STR, cur, start, p - start + 1);
  tok->contents = malloc(len + 1);
  memcpy(tok->contents, buf, len);
  tok->contents[len] = '\0';
  tok->cont_len = len + 1;
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

    // line comments
    if (starts_with(p, "//"))
    {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // block comments
    if (starts_with(p, "/*"))
    {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "unclosed block comment");
      p = q + 2;
      continue;
    }

    // string literal
    if (*p == '"')
    {
      cur = read_string_literal(cur, p);
      p += cur->len;
      continue;
    }

    // keywords
    char *kw = starts_with_reserved(p);
    if (kw)
    {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    // multi-letter punctuator
    if (starts_with(p, "==") ||
        starts_with(p, "!=") ||
        starts_with(p, ">=") ||
        starts_with(p, "<="))
    {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // single-letter punctuator
    if (ispunct(*p))
    {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // multi-letter identifier
    if (is_alpha(*p))
    {
      char *q = p++;
      while (is_alpha(*p) || isdigit(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    if (isdigit(*p))
    {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q; // 数値の部分がどこからどこまでは事前にわからないので、次のトークンの位置との差分で算出する
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
