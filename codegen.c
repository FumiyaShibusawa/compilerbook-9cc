#include "9cc.h"

int labelseq = 1;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *funcname;

void gen_lvar(Node *node)
{
  if (node->kind != ND_LVAR)
    error("左辺値が変数ではありません");
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->var->offset);
  printf("  push rax\n");
  return;
}

void gen(Node *node)
{
  switch (node->kind)
  {
  case ND_NULL:
    return;
  case ND_IF:
  {
    int seq = labelseq++;
    if (node->els)
    {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.else.%d\n", seq);
      gen(node->then);
      printf("  jmp .L.end.%d\n", seq);
      printf(".L.else.%d:\n", seq);
      gen(node->els);
      printf(".L.end.%d:\n", seq);
    }
    else
    {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  jmp .L.end.%d\n", seq);
      gen(node->then);
      printf(".L.end.%d:\n", seq);
    }
    return;
  }
  case ND_WHILE:
  {
    int seq = labelseq++;
    printf(".L.continue.%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.break.%d\n", seq);
    gen(node->then);
    printf("  jmp .L.continue.%d\n", seq);
    printf(".L.break.%d:\n", seq);
    return;
  }
  case ND_FOR:
  {
    int seq = labelseq++;
    if (node->init)
      gen(node->init);
    printf(".L.begin.%d:\n", seq);
    if (node->cond)
    {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.break.%d\n", seq);
    }
    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.break.%d:\n", seq);
    return;
  }
  case ND_BLOCK:
    for (Node *var = node->body; var; var = var->next)
      gen(var);
    return;
  case ND_FUNCALL:
  {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
    {
      gen(arg);
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--)
      printf("  pop %s\n", argreg[i]);

    // We need to align RSP to a 16 byte boundary before
    // calling a function to meet ABI requirement.
    // RAX is set to 0 for variadic function.
    int seq = labelseq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", seq);
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  jmp .L.end.%d\n", seq);
    printf(".L.call.%d:\n", seq);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n");
    printf(".L.end.%d:\n", seq);
    printf("  push rax\n");
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", funcname);
    return;
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lvar(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lvar(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
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

void codegen(Function *prog)
{
  printf(".intel_syntax noprefix\n");
  for (Function *fn = prog; fn; fn = fn->next)
  {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    funcname = fn->name;

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    // Push arguments to the stack
    int i = 0;
    for (LVarList *vl = fn->params; vl; vl = vl->next)
    {
      LVar *var = vl->var;
      printf("  mov [rbp-%d], %s\n", var->offset, argreg[i++]);
    }

    for (Node *node = fn->node; node; node = node->next)
    {
      gen(node);
    }

    // Epilogue
    printf(".L.return.%s:\n", funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}
