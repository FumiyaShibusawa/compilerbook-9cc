#include "9cc.h"

static char *read_file(char *path)
{
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s\n", path, strerror(errno));

  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  char *buf = calloc(1, size + 2);
  int res = fread(buf, size, 1, fp);

  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

int align_to(int n, int align)
{
  return (n + align - 1) & ~(align - 1);
}

int main(int argc, char **argv)
{
  if (argc != 2)
    error("%s: 引数の個数が正しくありません\n", argv[0]);

  filename = argv[1];
  user_input = read_file(filename);
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
