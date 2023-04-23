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

    if (GET_SIZE(HDRP(start)) == GET_ALLOC(HDRP(start)))
    {

      exit(0);
    }
    if (start == adj_heap_max)
    {

      return;
    }
    start = NEXT_BLKP(start);
  }
}
void *find_fit(size_t needed)
{

  void *start = (char *)heap_start + (4 * WSIZE); // bp to the first block

  while (!(GET_ALLOC(HDRP(start)) == 0 && (GET_SIZE(HDRP(start)) >= needed)))
  {

    if (start == adj_heap_max)
    {
      return NULL;
    }
    start = NEXT_BLKP(start);
  }

  return start;
}

void place(void *bp, size_t needed)
{

  size_t blk_size = GET_SIZE(HDRP(bp));

  PUT((char *)HDRP(bp), PACK(needed, 1));
  PUT((char *)FTRP(bp), PACK(needed, 1));

  if (blk_size > needed)
  {
    PUT((char *)HDRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
    PUT((char *)FTRP(NEXT_BLKP(bp)), PACK(blk_size - needed, 0));
  }
}

void *coalesce(void *bp)
{

  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc)
  {

    return bp;
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
    bp = PREV_BLKP(bp);
  }
  else
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
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
inline void myfree(void *bp)
{
  if (bp == NULL)
  {

    return;
  }

  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
}

void *myrealloc(void *oldptrv, size_t newsz)
{

  size_t oldsz;

  // if (oldptrv == NULL)
  // {
  //   return mymalloc(newsz);
  // }

  // if (newsz == 0)
  // {
  //   myfree(oldptrv);
  //   return NULL;
  // }
  oldsz = GET_SIZE(HDRP(oldptrv));
  // alignment and boundary tag padding
  size_t adj_newsz;
  if (newsz <= DSIZE)
    adj_newsz = 2 * DSIZE;
  else
    adj_newsz = DSIZE * ((newsz + (DSIZE) + (DSIZE - 1)) / DSIZE);

  if (adj_newsz == oldsz)
  {

    return oldptrv;
  }
  else if (adj_newsz < oldsz)
  {
    place(oldptrv, adj_newsz);

    return oldptrv;
  }
  else
  {
    void *coalesced_bp = coalesce(oldptrv);
    if (GET_SIZE(HDRP(coalesced_bp)) >= adj_newsz)
    {
      if (oldptrv != coalesced_bp)
      {
        memmove(coalesced_bp, oldptrv, oldsz - DSIZE);
      }
      place(coalesced_bp, adj_newsz);

      return coalesced_bp;
    }

    void *newptr = mymalloc(newsz);
    if (!newptr)
      return NULL;
    memmove(newptr, oldptrv, oldsz - DSIZE);

    myfree(coalesced_bp);

    return newptr;
  }
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
