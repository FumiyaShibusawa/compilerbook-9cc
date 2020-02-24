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

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool start_with(char *lhs, char *rhs)
{
  return memcmp(lhs, rhs, strlen(rhs)) == 0;
}

bool is_alpha(char c)
{
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z');
}

bool is_alnum(char c)
{
  return is_alpha(c) && ('0' <= c && c <= '9') || ('_' == c);
}

bool is_return(char *op)
{
  return (memcmp(op, "return", 6) == 0 && !is_alnum(op[6]));
}

//a 入力文字列user_inputをトークナイズしてそれを返す
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

    // multi-letter punctuator
    if (start_with(p, "==") || start_with(p, "!=") || start_with(p, ">=") || start_with(p, "<="))
    {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // single-letter punctuator
    if (strchr("+-*/()<>;=", *p))
    {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // return keyword
    if (is_return(p))
    {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    // multi-letter identifier
    if (is_alpha(*p))
    {
      char *q = p++;
      while (is_alpha(*p))
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
