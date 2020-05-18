#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}
EOF


try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 3 'int main() { return ret3(); }'
try 5 'int main() { return ret5(); }'

try 4 'int main() { return add(1, 3); }'
try 6 'int main() { return sub(10, 4); }'
try 21 'int main() { return add6(1, 2, 3, 4, 5, 6); }'

try 0 'int main() { return 0; }'
try 42 'int main() { return 42; }'
try 2 'int main() { return 1+1; }'
try 51 'int main() { return 5-21+67; }'
try 233 'int main() { return 10 - 34 + 1; }'
try 7 'int main() { return 1 + 1 * (3 + 3); }'
try 3 'int main() { return (3 + 4)/2; }'
try 253 'int main() { return -3; }'
try 7 'int main() { return +7; }'
try 200 'int main() { return -10*(-20); }'

try 0 'int main() { 0 == 1; }'
try 1 'int main() { 1 == 1; }'
try 0 'int main() { 5 != 5; }'
try 1 'int main() { 5 != 4; }'
try 0 'int main() { 40 < 1; }'
try 1 'int main() { 3 < 10; }'
try 0 'int main() { 5 > 53; }'
try 1 'int main() { 7 > 3; }'
try 0 'int main() { 7 <= 3; }'
try 1 'int main() { 3 <= 3; }'
try 1 'int main() { 3 <= 7; }'
try 0 'int main() { 3 >= 5; }'
try 1 'int main() { 3 >= 3; }'
try 1 'int main() { 5 >= 3; }'

try 29 'int main() { int a = 3 + 4; int b = 5 * 6 - 8; return a + b; }'
try 8 'int main() { int foo = 3; int bar = 5; return foo+bar; }'
try 58 'int main() { int foo = 3 * 20 - (6 / 2); int bar = 45 >= 10; return foo + bar; }'
try 5 'int main() { int foo = 5; return foo; }'

try 1 'int main() { if (10 >= 5) { return 1; } }'
try 1 'int main() { if (10 >= 5) { return 1; } else { return 2; } }'
try 1 'int main() { if (10 >= 5) { 1; } else { 2; } }'
try 2 'int main() { int foo = 10; if (foo < 5) { int bar = 1; } else { int bar = 2; } return bar; }'

try 10 'int main() { int count = 0; while (count < 10) { count = count + 1; } return count; }'

try 43 'int main() { int count = 0; int i = 0; for (i = 0; i < 43; i = i + 1) { count = count + 1; } return count; }'
try 6 'int main() { { 1 + 5; } }'
try 20 'int main() { { 1 + 5; 4 * 5; } }'

try 3 'int main() { int x = 3; return *&x; }'
try 5 'int main() { int x = 5; int y = &x; int **z = &y; return **z; }'
try 7 'int main() { int x = 4; int y = 7; return *(&x + 1); }'
try 3 'int main() { int x = 3; int y = 5; return *(&y - 1); }'
try 5 'int main() { int x = 3; int *y = &x; *y = 5; return x; }'
try 7 'int main() { int x = 3; int y = 5; *(&x + 1) = 7; return y; }'
try 7 'int main() { int x = 3; int y = 5; *(&y - 1) = 7; return x; }'
try 2 'int main() { int x = 3; return (&x + 2) - &x; }'

try 7 'int main() { return add2(3, 4); } int add2(int x, int y) { return x + y; }'
try 1 'int main() { return sub2(4, 3); } int sub2(int x, int y) { return x - y; }'
try 55 'int main() { return fib(9); } int fib(int x) { if (x <= 1) return 1; return fib(x - 1) + fib(x - 2); }'

try 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

try 3 'int main() { int x[3]; *x = 3; *(x + 1) = 4; *(x + 2) = 5; return *x; }'
try 4 'int main() { int x[3]; *x = 3; *(x + 1) = 4; *(x + 2) = 5; return *(x + 1); }'
try 5 'int main() { int x[3]; *x = 3; *(x + 1) = 4; *(x + 2) = 5; return *(x + 2); }'

try 0 'int main() { int x[2][3]; int *y = x; *y = 0; return **x; }'
try 1 'int main() { int x[2][3]; int *y = x; *(y + 1) = 1; return *(*x + 1); }'
try 2 'int main() { int x[2][3]; int *y = x; *(y + 2) = 2; return *(*x + 2); }'
try 3 'int main() { int x[2][3]; int *y = x; *(y + 3) = 3; return **(x + 1); }'
try 4 'int main() { int x[2][3]; int *y = x; *(y + 4) = 4; return *(*(x + 1) + 1); }'
try 5 'int main() { int x[2][3]; int *y = x; *(y + 5) = 5; return *(*(x + 1) + 2); }'
try 6 'int main() { int x[2][3]; int *y = x; *(y + 6) = 6; return **(x + 2); }'
try 10 'int main() { int x[2][2][2]; int *y = x; *(y + 1) = 10; return *(**x + 1); }'

echo OK
