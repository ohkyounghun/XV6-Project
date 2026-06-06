// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

// PA4: 0 until lruinit() has run (set in main()). Boot-time frees in kinit()
// happen before the LRU exists, so kfree() must not touch it until this is set.
// This lru_active guard was written with Claude assistance for the assignment;
// the boot-ordering hazard it solves is not covered by Xv6_part4_PageReplacement.pptx.
int lru_active = 0;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 free_pages; // PA3 (Slide 27) counter that tracks the current free-page pool size so freemem() can return it in O(1).
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // PA4: enforce the invariant "a frame on the freelist is never on the LRU".
  // uvmunmap() already unlinks ordinary user pages before freeing them, but a
  // few paths free a frame whose PTE_U was stripped after mapping (e.g. exec's
  // stack-guard page). If such a frame's LRU node lingered, lru_select_victim()
  // would later walk() a freed/reused page table through the stale node and
  // either corrupt an unrelated process's frame or panic. lru_remove() is
  // idempotent (a no-op when the frame isn't tracked), so this is safe for
  // page-table pages, kernel pages, and already-unlinked user pages alike.
  // This kfree() LRU backstop was written with Claude assistance for the
  // assignment; it is beyond the Xv6_part4_PageReplacement.pptx spec, which only
  // specifies removing a page from the LRU inside uvmunmap (Slide 14/22).
  if(lru_active)
    lru_remove((uint64)pa);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.free_pages++; // PA3 (Slide 27): increment the free-page counter so freemem() observes the page that just returned to the pool.
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.free_pages--; // PA3 (Slide 27): decrement the free-page counter as soon as a page leaves the pool so freemem() reflects the allocation.
  }
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    return (void*)r;
  }

  // PA4: freelist is empty — reclaim a frame by swapping one user page out.
  // This MUST be called AFTER releasing kmem.lock: swap_out does disk I/O via
  // swapwrite (sleeplock) and also takes lru.lock, so calling it while holding
  // kmem.lock would deadlock. swap_out returns the pa of the now-unmapped
  // frame directly, so it bypasses the freelist and serves this request.
  void *pa = swap_out();
  if(pa){
    memset((char*)pa, 5, PGSIZE); // fill with junk (swap_in overwrites it all soon)
    return pa;
  }

  // swap_out also returned 0 -> no user page to evict and no free swap slot = true OOM.
  // Part4 Slide 29: print a message when physical memory is truly exhausted.
  printf("kalloc: out of memory\n");
  return 0;
}

// AI was used (ChatGPT provided guidance on freelist traversal algorithm)
uint64
freemem(void) // freemem: The function which calculates free memory in BYTES (used by meminfo)
{
  struct run *r; // pointer for freelist
  uint64 total = 0;

  acquire(&kmem.lock); // Check spinlock.c
  for(r = kmem.freelist; r; r = r->next)
    total += PGSIZE;
  release(&kmem.lock);

  return total;
}

// This freemem_pages() helper was written with Claude assistance for the assignment.
// Slide 27 of the project spec defines freemem() as "the current number of free physical memory pages", so this returns the integer page count rather than bytes (kept separate from the legacy bytes-based freemem() above to avoid breaking sys_meminfo).
uint64
freemem_pages(void)
{
  uint64 n; // Snapshot of the counter so we can release the lock before returning.
  acquire(&kmem.lock); // Take the same lock that guards every increment/decrement in kalloc/kfree.
  n = kmem.free_pages; // Read the counter exactly once while holding the lock to avoid a torn read on multicore.
  release(&kmem.lock); // Release the lock before returning so callers do not run with it held.
  return n; // Slide 27 semantics: caller sees the count at the moment we took the snapshot.
}



