#include "allocator.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WSIZE 4
#define DSIZE 8

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define ALIGNMENT 8

static void *heap_start, *heap_max, *adj_heap_max;

////////////////////////////////////////
size_t roundup(size_t sz, size_t mult)
{
  return (sz + mult - 1) & ~(mult - 1);
}
void traverseFreelist()
{
  void *start = (char *)heap_start + (4 * WSIZE);
  while (1)
  {
    printf("[%uB,%d]", GET_SIZE(HDRP(start)), GET_ALLOC(HDRP(start)));
    if (GET_SIZE(HDRP(start)) == GET_ALLOC(HDRP(start)))
    {
      puts("problem while traversing!");
      exit(0);
    }
    if (start == adj_heap_max)
    {
      puts("\n");
      return;
    }
    start = NEXT_BLKP(start);
  }
}
void *find_fit(size_t needed)
{
  // first fit
  void *start = (char *)heap_start + (4 * WSIZE); // bp to the first block
  // puts("findfit---------------\n");
  while (!(GET_ALLOC(HDRP(start)) == 0 && (GET_SIZE(HDRP(start)) >= needed)))
  {
    // printf("hdr:%u bytes ,status %d\n", GET_SIZE(HDRP(start)), GET_ALLOC(HDRP(start)));
    if (GET_SIZE(HDRP(start)) == GET_ALLOC(HDRP(start)))
    {
      puts("problem while findin fit!");

      exit(0);
    }
    if (start == adj_heap_max)
    {
      return NULL;
    }
    start = NEXT_BLKP(start);
  }
  // printf("hdr:%u bytes ,status %d\n", GET_SIZE(HDRP(start)), GET_ALLOC(HDRP(start)));

  // puts("--------------------\n\n");

  return start;
}

void place(void *bp, size_t needed)
{
  size_t blk_size = GET_SIZE((char *)bp - WSIZE);
  // printf("\nsz:%lu\n", needed);

  PUT((char *)HDRP(bp), PACK(needed, 1));
  PUT((char *)FTRP(bp), PACK(needed, 1));
  if (blk_size > needed)
  {
    PUT((char *)HDRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
    PUT((char *)FTRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
  }
}

void coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc)
  {
    return;
  }
  else if (prev_alloc && !next_alloc)
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  else if (!prev_alloc && next_alloc)
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
  }
  else
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
  }
  if (GET_SIZE(HDRP(bp)) == GET_ALLOC(HDRP(bp)))
  {
    puts("problem while coalescing!");
    exit(0);
  }
}

bool myinit(void *, size_t);
inline bool myinit(void *reqstart, size_t reqsz)
{
  size_t rounded_reqsz = roundup(reqsz, DSIZE) - 8;
  heap_start = reqstart;
  heap_max = (char *)reqstart + reqsz;
  adj_heap_max = (char *)reqstart + rounded_reqsz;

  PUT(reqstart, 0);                                                          // alignment padding
  PUT((char *)reqstart + WSIZE, PACK(DSIZE, 1));                             // prolog header
  PUT((char *)reqstart + (WSIZE * 2), PACK(DSIZE, 1));                       // prolog footer
  PUT((char *)reqstart + (WSIZE * 3), PACK(rounded_reqsz - (WSIZE * 4), 0)); // storage block hdr

  PUT(FTRP((char *)reqstart + (WSIZE * 4)), PACK(rounded_reqsz - (WSIZE * 4), 0)); // storage block ftr

  PUT(HDRP(NEXT_BLKP((char *)reqstart + (WSIZE * 4))), PACK(0, 1));
  // printf("Myinit:Demand %luB rounded:%lu\n", reqsz,rounded_reqsz);
  // printf("strgBP%p nextBP%u\n",(char *)reqstart + (WSIZE * 4), GET_SIZE(FTRP((char *)reqstart + (WSIZE * 4)))); // epilogue
  // printf("reqstart%p heapmax%p expectedheapmax%p\n", reqstart, heap_max, NEXT_BLKP((char *)reqstart + (WSIZE * 4)));  // epilogue
  // printf("Myinit:NEXT BLK TO storage hdr: %uB\n", GET_SIZE(HDRP(NEXT_BLKP((char *)reqstart + (WSIZE * 4)))));
  // printf("Myinit:ftr %uB\n", GET_SIZE(FTRP(NEXT_BLKP((char *)reqstart + (WSIZE * 4)))));

  traverseFreelist();
  return true;
}

void *mymalloc(size_t sz)
{

  size_t needed;
  void *bp;

  // ignore spurious requests
  if (sz == 0)
  {
    return NULL;
  }

  // roundup to the required alignment
  if (sz <= DSIZE)
    needed = 2 * DSIZE;
  else
    needed = DSIZE * ((sz + (DSIZE) + (DSIZE - 1)) / DSIZE);

  printf("malloc %lu bytes| asked %lu\n", needed, sz);
  // find fit and place the block
  if ((bp = find_fit(needed)) != NULL)
  {
    place(bp, needed);
    traverseFreelist();
    return bp;
  }
  return NULL;
}

void myfree(void *);
inline void myfree(void *bp)
{
  printf("free %u bytes\n", GET_SIZE(HDRP(bp)));

  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
  traverseFreelist();
}

void *myrealloc(void *oldptrv, size_t newsz)
{
  return NULL;
}

bool validate_heap()
{
  // if (next_avail > heap_max)
  // {
  //   printf("Oops! Next available spot is beyond heap end!!\n");
  //   return false;
  // }
  return true;
}
