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

static void *heap_start, *heap_max;

////////////////////////////////////////
size_t roundup(size_t sz, size_t mult)
{
  return (sz + mult - 1) & ~(mult - 1);
}

void *find_fit(size_t needed)
{
  // first fit
  void *start = (char *)heap_start + (4 * WSIZE); // bp to the first block

  while (!(GET_ALLOC(HDRP(start)) == 0 && (GET_SIZE(HDRP(start)) >= needed)))
  {
    // printf("hdr:%u bytes ,status %d",GET_SIZE(HDRP(start)),GET_ALLOC(HDRP(start)))
    if (start == heap_max)
    {
      return NULL;
    }
    start = NEXT_BLKP(start);
  }
  return start;
}

void place(void *bp, size_t needed)
{
  size_t blk_size = GET_SIZE((char *)bp - WSIZE);
  printf("\nsz:%lu\n", needed);

  PUT((char *)HDRP(bp), PACK(needed, 1));
  PUT((char *)FTRP(bp), PACK(needed, 1));
  PUT((char *)HDRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
  PUT((char *)FTRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
}

bool myinit(void *, size_t);
inline bool myinit(void *reqstart, size_t reqsz)
{

  heap_start = reqstart;
  heap_max = (char *)reqstart + reqsz;

  PUT(reqstart, 0);                                                  // alignment padding
  PUT((char *)reqstart + WSIZE, PACK(DSIZE, 1));                     // prolog header
  PUT((char *)reqstart + (WSIZE * 2), PACK(DSIZE, 1));               // prolog footer
  PUT((char *)reqstart + (WSIZE * 3), PACK(reqsz - (WSIZE * 4), 0)); // storage block hdr

  PUT(FTRP((char *)reqstart + (WSIZE * 4)), PACK(reqsz - (WSIZE * 4), 0)); // storage block ftr
  PUT((char *)heap_max - WSIZE, PACK(0, 1));                               // epilogue
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

  // find fit and place the block
  if ((bp = find_fit(needed)) != NULL)
  {
    place(bp, needed);
    return bp;
  }
  return NULL;
}

void myfree(void *);
inline void myfree(void *ptrv)
{
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
