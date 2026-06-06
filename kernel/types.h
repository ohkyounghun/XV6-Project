typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

// PA4 Slide 22: one struct page per physical frame in [KERNBASE, PHYSTOP).
// next/prev form a circular doubly-linked LRU list; pagetable/vaddr identify
// the user mapping that owns this frame.  pagetable == 0 means "not on list".
// Uses uint64 * instead of pagetable_t because types.h is included before riscv.h
// where pagetable_t (typedef uint64 *) is defined.
struct page {
  struct page *next, *prev;
  uint64      *pagetable;  // same underlying type as pagetable_t
  uint64       vaddr;
};
