#!/bin/bash

try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 0 '0;'
try 42 '42;'
try 2 '1+1;'
try 51 '5-21+67;'
try 233 '10 - 34 + 1;'
try 7 '1 + 1 * (3 + 3);'
try 3 '(3 + 4)/2;'
try 253 '-3;'
try 7 '+7;'
try 200 '-10*(-20);'

try 0 '0 == 1;'
try 1 '1 == 1;'
try 0 '5 != 5;'
try 1 '5 != 4;'
try 0 '40 < 1;'
try 1 '3 < 10;'
try 0 '5 > 53;'
try 1 '7 > 3;'
try 0 '7 <= 3;'
try 1 '3 <= 3;'
try 1 '3 <= 7;'
try 0 '3 >= 5;'
try 1 '3 >= 3;'
try 1 '5 >= 3;'

try 29 'a=3+4;b=5*6-8;a+b;'
try 8 'foo=3;bar=5;foo+bar;'
try 58 'foo = 3 * 20 - (6 / 2); bar = 45 >= 10; foo + bar;'
try 5 'foo = 5;return foo;'

try 1 'if (10 >= 5) { return 1; }'
try 1 'if (10 >= 5) { return 1; } else { return 2; }'
try 1 'if (10 >= 5) { 1; } else { 2; }'
try 2 'foo = 10; if (foo < 5) { bar = 1; } else { bar = 2; } return bar;'

try 10 'count = 0; while (count < 10) { count = count + 1; }; count;'

try 43 'count = 0; for (i = 0; i < 43; i = i + 1) { count = count + 1; }; count;'

echo OK
