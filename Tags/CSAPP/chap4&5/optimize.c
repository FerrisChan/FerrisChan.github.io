#include <ctype.h>
#include <stdio.h>

// OP 的操作不同，比较加法和乘法的时间

// 求和
#define IDENT (0)
#define OP (+)

/*
// 求积
#define OP (*)
#define IDENT (１)
*/
typedef long data_t；
    /* data structure for vectors */
    // 一个包括数组的长度和元素的结构
    typedef struct {
  size_t len;
  data_t *data;
} vec， *vec_ptr;

/* retrieve vector element　and store at val */
// 检测边界和得到数组的元素
int get_vec_element(*vec v, size_t idx, data_t *val) {
  if (idx >= v->len)
    return 0;
  *val = v->data[idx];
  return 1;
}
// 原始版本１，计算数组的元素积或总和
// Compute sum or product of vector elements
void combine1(vec_ptr v, data_t *dest) {
  long int i;
  *dest = IDENT;
  for (i = 0; i < vec_length(v); i++) {
    data_t val;
    get_vec_element(v, i, &val);
    *dest = *dest OP val;
  }
}

void combine2(vec_ptr v, data_t *dest) {
  long i;
  long length = vec_length(v); // 优化二

  for (i = 0; i < length; i++) {
    data_t val;
    get_vec_element(v, i, &val);
    *dest = *dest OP val;
  }
}

void combine３(vec_ptr v, data_t *dest) {
  long i;
  long length = vec_length(v);
  data_t *d = get_vec_start(v);

  *dest = IDENT;

  for (i = 0; i < length; i++)
    *dest = *dest OP data[i];
  　 // 优化３
}

void combine4(vec_ptr v, data_t *dest) {
  long i;
  long length = vec_length(v);
  data_t *d = get_vec_start(v);
  data_t t = IDENT; //中间变量，缓存

  for (i = 0; i < length; i++)
    t = t OP d[i];
  *dest = t;
  　 // 优化４
}

// 优化５
void unroll2a_combine(vec_ptr v, data_t *dest) {
  long length = vec_length(v);
  long limit = length - 1;
  　　　 // 注意length不总是偶数，所以length-1为检测
      data_t *d = get_vec_start(v);
  data_t x = IDENT;
  long i;
  /* Combine 2 elements at a time */
  for (i = 0; i < limit; i += 2) {
    x = (x OP d[i])OP d[i + 1];
    　　　 // 注意这里的写法
  }
  /* Finish any remaining elements */
  for (; i < length; i++) {
    x = x OP d[i];
  }
  *dest = x;
}

void unroll2a_combine(vec_ptr v, data_t *dest) {
  long length = vec_length(v);
  long limit = length - 1;
  data_t *d = get_vec_start(v);
  data_t x0 = IDENT;
  data_t x1 = IDENT;
  long i;
  /* Combine 2 elements at a time */
  for (i = 0; i < limit; i += 2) {
    // 注意这里的写法
    x0 = x0 OP d[i];
    x1 = x1 OP d[i + 1];
  }
  /* Finish any remaining elements */
  for (; i < length; i++) {
    x0 = x0 OP d[i];
  }
  *dest = x0 OP x1;
}

void combine7(vec_ptr v, data_t *dest) {
  long length = vec_length(v);
  long limit = length - 1;
  　　　 // 注意length不总是偶数，所以length-1为检测
      data_t *d = get_vec_start(v);
  data_t x = IDENT;
  long i;
  /* Combine 2 elements at a time */
  for (i = 0; i < limit; i += 2) {
    x = x OP(d[i] OP d[i + 1]);
    　　　 // 注意这里的写法
  }
  /* Finish any remaining elements */
  for (; i < length; i++) {
    x = x OP d[i];
  }
  *dest = x;
}
