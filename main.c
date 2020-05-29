#include "9cc.h"

int align_to(int n, int align)
{
  return (n + align - 1) & ~(align - 1);
}

int main(int argc, char **argv)
{
  if (argc != 2)
    error("%s: 引数の個数が正しくありません\n", argv[0]);

  user_input = argv[1];
  // トークナイズする
  token = tokenize();
  // デバッグ用
  // for (Token *var = token; var; var = var->next)
  //   printf("token->kind: %d token->str: %s token->len: %d\n", var->kind, var->str, var->len);
  Program *prog = program();

  // 関数ごとのローカル変数用のメモリサイズを計算する
  for (Function *fn = prog->fns; fn; fn = fn->next)
  {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next)
    {
      offset += vl->var->ty->size;
      vl->var->offset = offset;
    }
    fn->stack_size = align_to(offset, 8);
  };

  codegen(prog);

  return 0;
}
