#include "stdio.h"

int Fibonacci(int n) {
  if (n == 0)
    return 0;
  if (n == 1)
    return 1;
  return Fibonacci(n - 1) + Fibonacci(n - 2);
}

int main(int argc, char const *argv[]) {
  int i = 20;
  int res;
  res = Fibonacci(i);
  printf("%d\n", res);
  return 0;
}
