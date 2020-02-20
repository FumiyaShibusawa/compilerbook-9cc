#include "9cc.h"

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
