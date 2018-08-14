/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS:
 * 使用的是显式分离(双向链表)的方法
 *fit in 使用的是binary search
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ferris",
    /* First member's full name */
    "ferris chan",
    /* First member's email address */
    "ferris chan@xxx.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 一些基本操作和宏定义 */
#define WSIZE 4 /* 4字节--字长,和头部,脚部长 */
#define DSIZE 8 /* 8字节--双字长 */
/* 块的最小大小,2^12 Byte= 4k ;Extend heap by this amount (bytes) */
#define CHUNKSIZE (1 << 12)

// 求最大值
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// PACK将大小和已分配位结合起来并返回
#define PACK(size, alloc) ((size) | (alloc))

// GET得到地址(指针)p的值; PUT将val 存放在地址p,等效*p = val;
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)p = (val))

/* GET_SIZE 得到已分配地址的头部或脚步p的大小,也即 xxx & ~0x7
 *  GET_ALLOC 得到该区块的最低位,看是否已经分配,也即 xxx & ~0x1
 */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & ~0x1)

/* 给出块的指针bp,计算其头部和脚部的指针;
 * HDRP 为头部指针,FTRP为脚部指针
 */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(bp) - DSIZE)

/* 给出块的指针bp,计算其下一个块和前一个块 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

// 二分查找树一些操作的宏定义
#define LEFT(bp) ((char *)(bp))
#define RIGHT(bp) ((char *)(bp) + WSIZE)
#define PARENT(bp) ((char *)(bp) + DSIZE)
#define GET_LEFT(bp) (*(void **)LEFT(bp))
#define GET_RIGHT(bp) (*(void **)RIGHT(bp))
#define GET_PARENT(bp) (*(void **)PARENT(bp))

#define SET_LEFT(bp, left) (*(void **)LEFT(bp) = left)
#define SET_RIGHT(bp, right) (*(void **)RIGHT(bp) = right)
#define SET_PARENT(bp, parent) (*(void **)PARENT(bp) = parent)
// 设置空的头部和尾部
#define SET_HEAD_FREE(bp) (GET(HDRP(bp)) &= ~0x7)
#define SET_FOOT_FREE(bp) (GET(FTRP(bp)) &= ~0x7)

static void *insert_node(void *bp);
static void delete_node(void *bp);

/*
 * 其他参数请看config.h,
 */

/* 一些static全局变量 */
static char *heap_listp = 0; // 指向第一个块
static void *root = NULL;
/* 函数声明*/

/* 扩展heap的大小*/
static void *extend_heap(size_t words);

/* 合并 */
static void *coalesce(void *bp);
/* 查找合适的block */
static void *find_fit(size_t asize);
/* 将block 选择合适放置 */
static void place(void *bp, size_t asize);

static void checkheap(int verbose);
static void checkblock(void *bp);
/*
 * mm_init - initialize the malloc package.
 * 也即创建头指针
 */
int mm_init(void) {
  if ((heap_listp = (char *)mem_sbrk(4 * WSIZE)) == (void *)-1)
    return -1;
  // 见图9-48
  PUT(heap_listp, 0);                              /* Alignment padding */
  PUT((heap_listp + (1 * WSIZE)), PACK(DSIZE, 1)); /* Prologue header 首部头 */
  PUT((heap_listp + (2 * WSIZE)), PACK(DSIZE, 1)); /* Prologue footer  首部尾 */
  PUT((heap_listp + (3 * WSIZE)), PACK(0, 1)); /* Epilogue header 尾部footer*/
  heap_listp += (2 * WSIZE);

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    return -1;
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  size_t asize; /* 调整的块大小 Adjusted block size */
  size_t extendsize; /* 扩展的块大小,需要更大的块才可以fit Amount to extend heap
                        if no fit */
  char *bp;

  /* 错误检测 */
  if (heap_listp == 0) {
    mm_init();
  }
  if (size <= 0)
    return NULL;

  /* 块的大小必须包含头部和尾部,所以必须为16个字节*/
  if (size <= DSIZE) {
    asize = 2 * DSIZE;
  } else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  } // 8字节的最小整数倍

  /* Search the free list for a fit */
  if ((bp = (char *)find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE); // line:vm:mm:growheap1
  if ((bp = (char *)extend_heap(extendsize / WSIZE)) != NULL) {
    if (NULL != (bp = find_fit(extendsize)))
      place(bp, asize);
    return bp;
  }
  return NULL;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  size_t size = GET_SIZE(HDRP(ptr));
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  insert_node(coalesce(ptr));
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

/* 声明函数的实现*/

/* 扩展heap,增加一个chunk*/
static void *extend_heap(size_t words) {
  char *bp;                                                 // 块指针
  size_t size;                                              // 实际的size
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // 偶数个Word
  if ((long)(bp = (char *)mem_sbrk(size)) == -1)
    return NULL; // brk指针移动失败

  /* 初始化首部头部和尾部 */
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  /*看是否需要合并  */
  return insert_node(coalesce(bp));
}

/* 合并空闲块 */
static void *coalesce(void *bp) {
  void *pre_bp = (void *)PREV_BLKP(bp);
  void *next_bp = (void *)NEXT_BLKP(bp);
  if (!GET_ALLOC(HDRP(next_bp))) {
    size_t total_size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_bp));
    PUT(HDRP(bp), PACK(total_size, 0));
    PUT(FTRP(next_bp), PACK(total_size, 0));
    delete_node(next_bp);
  }
  if (!GET_ALLOC(HDRP(pre_bp))) {

    size_t total_size = GET_SIZE(HDRP(pre_bp)) + GET_SIZE(HDRP(bp));
    PUT(HDRP(pre_bp), PACK(total_size, 0));
    PUT(FTRP(bp), PACK(total_size, 0));
    delete_node(pre_bp);
    return pre_bp;
  }
  return bp;
}

// 查找适合的block,利用二分查找树;双向链表,所以有parent指针
static void *find_fit(size_t asize) {
  void *node = root;
  void *fit = NULL;
  while (node != NULL) {
    if (asize <= GET_SIZE(HDRP(node))) {
      fit = node;
      node = GET_LEFT(node);
    } else {
      // fit 不能= node,因为node到最大点
      node = GET_RIGHT(node);
    }
  }
  return fit;
}

static void place(void *bp, size_t size) {
  delete_node(bp);
  size_t block_size = GET_SIZE(HDRP(bp));
  if (block_size - size < 24) // freelist node minsize
  {
    PUT((HDRP(bp)), PACK(block_size, 1));
    PUT((FTRP(bp)), PACK(block_size, 1));
    return;
  }
  PUT((HDRP(bp)), PACK(size, 1));
  PUT((FTRP(bp)), PACK(size, 1));
  PUT((HDRP(NEXT_BLKP(bp))), PACK(block_size - size, 0));
  PUT((FTRP(NEXT_BLKP(bp))), PACK(block_size - size, 0));

  insert_node(coalesce(NEXT_BLKP(bp)));
}

static void *insert_node(void *bp) {
  // 设置空的头部和尾部
  SET_HEAD_FREE(bp);
  SET_FOOT_FREE(bp);
  // PUT((HDRP(bp)), (GET(HDRP(bp)) &= ~07));
  // PUT((FTRP(bp)), (GET(FTRP(bp)) &= ~07));

  if (root == NULL) {
    root = bp;
    SET_LEFT(bp, NULL);
    SET_RIGHT(bp, NULL);
    SET_PARENT(bp, NULL);
    return bp;
  }
  void *node = root;
  void *prev = node; // parent 节点
  size_t size = GET_SIZE(HDRP(bp));
  // 先找到合适的插入点prev
  while (NULL != node) {
    if (size <= GET_SIZE(HDRP(node))) {
      prev = node;
      node = GET_LEFT(bp);
    } else {
      prev = node;
      node = GET_RIGHT(bp);
    }
  }
  // 把bp挂上prev
  if (size <= GET_SIZE(HDRP(prev)))
    SET_LEFT(prev, bp);
  else
    SET_RIGHT(prev, bp);
  SET_PARENT(bp, prev);
  //设置bp的左右节点为0
  SET_LEFT(bp, NULL);
  SET_RIGHT(bp, NULL);
  return bp;
}
static void delete_node(void *bp) {

  // 设置空的头部和尾部
  SET_HEAD_FREE(bp);
  SET_FOOT_FREE(bp);

  void *parent = GET_PARENT(bp);
  void *right = GET_RIGHT(bp);
  void *left = GET_LEFT(bp);
  if (bp == root) {
    if (right == NULL)
      root = left;
    else {
      void *temp = right;
      while (GET_LEFT(temp) != NULL) // look up the new root
        temp = GET_LEFT(temp);
      if (temp == right) // right child of bp has not left child
      {
        if (left == NULL) {
          root = right;
          SET_PARENT(right, NULL);
          return;
        } else {
          root = left;
          SET_LEFT(right, GET_RIGHT(left));

          SET_RIGHT(left, right);
          SET_PARENT(left, NULL);
          return;
        }
      }
      void *temp_parent = GET_PARENT(temp);
      void *temp_right = GET_RIGHT(temp);
      SET_LEFT(temp_parent, temp_right); // right node of temp will replace temp
      if (temp_right != NULL)
        SET_PARENT(temp_right, temp_parent);

      root = temp;
      SET_LEFT(root, left);
      SET_RIGHT(root, right);
      SET_PARENT(root, NULL);
    }
  } else {
    if (right == NULL) {
      if (bp == GET_LEFT(parent))
        SET_LEFT(parent, left);
      else
        SET_RIGHT(parent, left);
      if (left != NULL)
        SET_PARENT(left, parent);

    } else {
      void *temp = right;
      while (GET_LEFT(temp) != NULL) // look up the root
        temp = GET_LEFT(temp);
      void *temp_parent = GET_PARENT(temp);
      void *temp_right = GET_RIGHT(temp);
      SET_LEFT(temp_parent, temp_right); // right node of temp will replace temp
      if (temp_right != NULL)
        SET_PARENT(temp_right, temp_parent);
      if (bp == GET_LEFT(parent))
        SET_LEFT(parent, temp);
      else
        SET_RIGHT(parent, temp);

      // set new node that replace bp
      SET_LEFT(temp, left);
      SET_RIGHT(temp, right);
      SET_PARENT(temp, parent);
    }
  }
};

static void printblock(void *bp) {
  size_t hsize, halloc, fsize, falloc;

  checkheap(0);
  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));

  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, hsize,
         (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp) {
  if ((size_t)bp % 8)
    printf("Error: %p is not doubleword aligned\n", bp);
  if (GET(HDRP(bp)) != GET(FTRP(bp)))
    printf("Error: header does not match footer\n");
}

/*
 *
  - Minimal check of the heap for consistency
 */
void checkheap(int verbose) {
  char *bp = heap_listp;

  if (verbose)
    printf("Heap (%p):\n", heap_listp);

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
    printf("Bad prologue header\n");
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose)
      printblock(bp);
    checkblock(bp);
  }

  if (verbose)
    printblock(bp);
  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
    printf("Bad epilogue header\n");
}
