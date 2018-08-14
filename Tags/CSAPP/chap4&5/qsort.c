#include <stdio.h>
#define LEN 15

void swap(int *const p1, int *const p2) {
  int tmp = *p1;
  *p1 = *p2;
  *p2 = tmp;
}

void qsort(int a[], int left, int right) {
  int i, j, pivot;
  pivot = a[right]; // the last item as pivot
  i = left;
  j = right - 1;
  if (left < right) {
    for (;;) {
      for (; a[i] < pivot; i++)
        ;
      for (; a[j] > pivot; j--)
        ;
      if (i < j)
        swap(&a[i], &a[j]);
      else
        break;
    }
    swap(&a[i], &a[right]); // now i is the pivot index in the array
    qsort(a, left, i - 1);
    qsort(a, i + 1, right);
  }
}

int main(void) {
  int i;
  int arr[LEN] = {43, 423, 13,  6,   34,   64,  24, 69,
                  32, 28,  432, 641, 4365, 345, 624};
  qsort(arr, 0, LEN - 1);
  for (i = 0; i < LEN; i++)
    printf("%d ", arr[i]);
  printf("\n");
  return 0;
}
