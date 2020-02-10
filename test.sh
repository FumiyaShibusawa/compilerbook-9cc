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

try 0 0
try 42 42
try 2 1+1
try 51 5-21+67
try 233 '10 - 34 + 1'
try 7 '1 + 1 * (3 + 3)'
try 3 '(3 + 4)/2'
try 253 -3
try 7 +7
try 200 '-10*(-20)'

echo OK
