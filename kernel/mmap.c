// This Project 3 (Virtual Memory) source file was written with Claude assistance for the assignment.
// It implements Slide 10~22 mmap() behavior, the Slide 20 free-slot helpers, and a Slide 26/27 defensive cleanup hook so the kernel stays stable even before Part B's munmap is complete.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "proc.h"
#include "defs.h"

// This is_free_mmap_slot() predicate was written with Claude assistance for the assignment.
// Slide 20 stores active mappings in an array, and Part A/B agreed to use length == 0 as the free-slot marker so callers never need a separate occupied flag.
int
is_free_mmap_slot(struct mmap_area *m)
{
  return m->length == 0; // Slide 20 free-slot convention: an entry whose length is zero is considered available.
}

// This find_mmap_area() helper was written with Claude assistance for the assignment.
// Slides 23~25 describe lazy page-fault handling that must map a faulting virtual address back to its owning region; this helper performs that lookup and Part B will call it from the page-fault handler.
struct mmap_area*
find_mmap_area(struct proc *p, uint64 va)
{
  for (int i = 0; i < MAXMMAP; i++) {           // Slide 20: scan every slot in the fixed-size mmap table because we have no ordering invariant.
    struct mmap_area *m = &p->mmap_areas[i];    // Grab a pointer for clarity since each branch needs both addr and length.
    if (is_free_mmap_slot(m)) continue;         // Skip free slots so we never match a dangling addr/length pair from a previous mapping.
    if (m->addr <= va && va < m->addr + (uint64)m->length)
      return m;                                 // Slide 23~25 matching rule: fault address is inside [addr, addr+length).
  }
  return 0;                                     // No active mapping covers va, so the caller should treat this as an invalid access.
}

// This ranges_overlap() helper was written with Claude assistance for the assignment.
// Slide 13 lists "Mapping area overlaps an existing mapping" as a failure case, so we centralize the predicate here for both the mmap() check and any future caller.
static int
ranges_overlap(uint64 a_start, uint64 a_len, uint64 b_start, uint64 b_len)
{
  uint64 a_end = a_start + a_len; // Compute the exclusive end of range A for an easy two-condition test.
  uint64 b_end = b_start + b_len; // Compute the exclusive end of range B for an easy two-condition test.
  return !(a_end <= b_start || b_end <= a_start); // Slide 13 overlap test: ranges share at least one byte when neither sits entirely before the other.
}

// This clear_mmap_slot() helper was written with Claude assistance for the assignment.
// Slide 20 lists exactly seven fields per entry; resetting all of them here keeps the free convention (length == 0) consistent with allocproc's initialization.
static void
clear_mmap_slot(struct mmap_area *m)
{
  m->f      = 0; // Drop the file pointer so later cleanup never sees a stale reference once the slot is recycled.
  m->addr   = 0; // Reset the start virtual address so find_mmap_area cannot accidentally match the released range.
  m->length = 0; // Slide 20 free-slot convention: zeroing length is what actually marks the slot as available.
  m->offset = 0; // Reset the file offset since the next mmap() that takes this slot will overwrite it anyway.
  m->prot   = 0; // Reset protection bits so stale flags never influence later permission checks.
  m->flags  = 0; // Reset MAP_* flags so eager/lazy decisions are recomputed fresh for the next caller.
  m->p      = 0; // Reset the owner pointer so back-references from helpers do not survive past the slot's release.
}

// This handle_mmap_fault() helper was written with Codex assistance for the assignment.
// It handles exactly one lazy mmap page at a time: validate the faulting address, enforce permissions, allocate/load one page, and install the user PTE.
int
handle_mmap_fault(struct proc *p, uint64 va, int is_read)
{
  struct mmap_area *m = find_mmap_area(p, va); // Look up which recorded mmap region owns the faulting address, if any.
  uint64 page_va; // Keep the aligned virtual page address separate from the raw fault address reported by the trap hardware.
  uint64 file_off; // Track the file offset for this page so file-backed lazy mappings load the correct bytes.
  uint64 pa; // Hold the newly allocated physical page address once kalloc succeeds.
  int perm = PTE_U; // Every mmap page is user-accessible, so start from the user bit and add R/W bits from prot below.
  int n; // Capture the number of bytes read from a file-backed mapping so short reads can be zero-filled safely.

  if(m == 0)
    return 0; // Returning 0 tells usertrap that this fault was not for mmap, so it may still belong to lazy sbrk/vmfault.
  if(!is_read && (m->prot & PROT_WRITE) == 0)
    return -1; // A write fault on a read-only mapping is invalid and should kill the process.

  page_va = PGROUNDDOWN(va); // Lazy mmap is page-granular, so we only materialize the single faulting page.
  if(page_va < m->addr || page_va >= m->addr + (uint64)m->length)
    return -1; // The aligned page must still lie inside the mmap region; otherwise the access is invalid.
  if(walkaddr(p->pagetable, page_va) != 0)
    return 1; // If the page is already mapped, report "handled" so usertrap does not treat the fault as an unknown exception.

  pa = (uint64)kalloc(); // Allocate one physical page now because this is the first touch of a lazy mmap page.
  if(pa == 0)
    return -1; // Allocation failure means we cannot satisfy the user fault safely.
  memset((void *)pa, 0, PGSIZE); // Start from a zero-filled page so anonymous mappings and short file reads both get correct trailing zeros.

  if((m->flags & MAP_ANONYMOUS) == 0) { // File-backed lazy mappings must pull data from the backing file when the page is first touched.
    file_off = (uint64)m->offset + (page_va - m->addr); // Convert the page's virtual offset inside the mapping into the matching file offset.
    ilock(m->f->ip); // Hold the inode lock while reading file contents into the freshly allocated page.
    n = readi(m->f->ip, 0, pa, file_off, PGSIZE); // Read at most one page because the project handles page faults one page at a time.
    iunlock(m->f->ip); // Release the inode lock immediately after the read completes.
    if(n < 0) {
      kfree((void *)pa); // Return the page before failing so the free-page count stays accurate.
      return -1; // A file read error means the fault cannot be satisfied.
    }
    if(n < PGSIZE)
      memset((void *)(pa + n), 0, PGSIZE - n); // Zero-fill the unread tail when the file is shorter than the mapped page span.
  }

  if(m->prot & PROT_READ)
    perm |= PTE_R; // Add read permission when the mapping was created as readable.
  if(m->prot & PROT_WRITE)
    perm |= PTE_W; // Add write permission when the mapping was created as writable.

  if(mappages(p->pagetable, page_va, PGSIZE, pa, perm) != 0) {
    kfree((void *)pa); // If installing the PTE fails, free the page so we do not leak physical memory.
    return -1; // Report failure so usertrap can kill the process rather than returning to a still-unmapped address.
  }
  return 1; // Returning 1 tells usertrap that the lazy mmap page was successfully materialized and execution may resume.
}

// This mmap() implementation was written with Claude assistance for the assignment.
// Slide 11 fixes the prototype, Slide 12 explains the MAP_POPULATE/MAP_ANONYMOUS flag matrix, Slide 13 enumerates every failure return, and Slides 14~18 walk through eager vs lazy behavior — each block below matches one of those slide pieces.
uint64
mmap(uint64 addr, int length, int prot, int flags, int fd, int offset)
{
  struct proc *p = myproc();                          // Slide 11 mmap() runs in the context of the current process, so we capture it once.
  struct file *f = 0;                                 // Slide 14~17 file-backed paths need this pointer; anonymous paths leave it null.
  int anonymous = (flags & MAP_ANONYMOUS) ? 1 : 0;    // Slide 12 distinction: anonymous mappings come from zero pages, not from a file.
  int populate  = (flags & MAP_POPULATE)  ? 1 : 0;    // Slide 12 distinction: MAP_POPULATE means eager allocation, otherwise lazy.

  // Slide 13 failure cases — argument validation.
  if (length <= 0)                                       return 0; // Slide 13: a non-positive length is never valid.
  if (addr   % PGSIZE != 0)                              return 0; // Slide 11/13: addr must be page-aligned because mappings are page granularity.
  if (length % PGSIZE != 0)                              return 0; // Slide 11/13: length must be a multiple of page size for the same reason.
  if (prot != PROT_READ && prot != (PROT_READ|PROT_WRITE)) return 0; // Slide 11/13: only PROT_READ or PROT_READ|PROT_WRITE are accepted.

  uint64 va_start = MMAPBASE + addr; // Slide 11/19: the actual mapping start is offset from the dedicated MMAPBASE region.
  uint64 va_end   = va_start + (uint64)length; // Compute the exclusive end address once for later range checks.
  if (va_end <= va_start)  return 0; // Defensive overflow guard so a huge length cannot wrap around the address space.
  if (va_end > TRAPFRAME)  return 0; // Slide 4 memory layout: refuse to touch the trapframe/trampoline area at the top of the user VA space.

  // Slide 13 failure cases — file/anonymous consistency.
  if (anonymous) {
    if (fd != -1) return 0;                       // Slide 12/13: anonymous mappings require fd == -1.
  } else {
    if (fd < 0 || fd >= NOFILE)         return 0; // Slide 13: file-backed mapping needs a valid fd index.
    f = p->ofile[fd];                             // Slide 45 hint: file structures live in proc->ofile[fd].
    if (f == 0)                         return 0; // Slide 13: a closed slot means no file backs this mapping.
    if (f->type != FD_INODE)            return 0; // We only support real inode-backed files (no pipes/devices for mmap).
    if ((prot & PROT_READ)  && !f->readable) return 0; // Slide 13: prot must be consistent with the file's open mode (read).
    if ((prot & PROT_WRITE) && !f->writable) return 0; // Slide 13: prot must be consistent with the file's open mode (write).
    if (offset < 0)                     return 0; // A negative file offset is never valid.
  }

  // Slide 13 failure case — overlap with an existing mmap_area belonging to this process.
  for (int i = 0; i < MAXMMAP; i++) {
    struct mmap_area *m = &p->mmap_areas[i];
    if (is_free_mmap_slot(m)) continue;                                                  // Skip empty slots before doing range math.
    if (ranges_overlap(va_start, (uint64)length, m->addr, (uint64)m->length))
      return 0;                                                                          // Slide 13: refuse mappings that intersect an existing region.
  }

  // Slide 13 failure case — no free slot left in the table.
  int slot = -1; // Sentinel meaning "no slot found yet"; if it stays -1 we report failure.
  for (int i = 0; i < MAXMMAP; i++) {
    if (is_free_mmap_slot(&p->mmap_areas[i])) { slot = i; break; } // Slide 20: take the first free slot we find.
  }
  if (slot < 0) return 0; // Slide 13: hitting MAXMMAP active entries is an explicit failure.

  // Slide 20 mmap_area recording — populate every field exactly as the spec defines.
  struct mmap_area *m = &p->mmap_areas[slot];
  m->f      = anonymous ? 0 : filedup(f); // Slide 14~17: keep our own ref via filedup so munmap/exit can safely fileclose even if user closes fd first.
  m->addr   = va_start;                   // Slide 20 field: record the virtual start so find_mmap_area can match faults later.
  m->length = length;                     // Slide 20 field: non-zero length both stores the size and marks the slot as occupied.
  m->offset = offset;                     // Slide 20 field: store the original file offset so the lazy fault handler can read the correct bytes.
  m->prot   = prot;                       // Slide 20 field: remember protection bits for fault-time permission checks.
  m->flags  = flags;                      // Slide 20 field: remember MAP_* flags so the fault path knows whether to zero-fill or read from the file.
  m->p      = p;                          // Slide 20 field: pointer back to the owner so global helpers can filter by current process.

  // Slide 12/16/17: without MAP_POPULATE we only record metadata and let Part B handle the first-touch page fault.
  if (!populate)
    return va_start; // Slide 11 return contract: success returns MMAPBASE+addr regardless of eager vs lazy path.

  // Slides 14~15/18 (MAP_POPULATE) — perform the eager allocation + content load + PTE install.
  int perm = PTE_U;                       // Slide 4: user mappings always need PTE_U so user mode can access them.
  if (prot & PROT_READ)  perm |= PTE_R;   // Slide 11: PROT_READ maps to PTE_R in the RISC-V page table.
  if (prot & PROT_WRITE) perm |= PTE_W;   // Slide 11: PROT_WRITE maps to PTE_W in the RISC-V page table.

  uint64 npages = (uint64)length / PGSIZE; // Slide 14~18: eager path handles each 4 KB page individually.
  for (uint64 i = 0; i < npages; i++) {
    char *mem = kalloc();                  // Slide 14 step 2/Slide 18: allocate one physical page from the free pool.
    if (!mem) goto fail;                   // Slide 13: kalloc failure aborts the whole mmap and triggers cleanup of partial work.

    if (anonymous) {
      memset(mem, 0, PGSIZE);              // Slide 18: anonymous pages must be zero-filled before user access.
    } else {
      ilock(f->ip);                                                       // Slide 14 step 3: hold the inode lock while reading file data.
      int n = readi(f->ip, 0, (uint64)mem, offset + i * PGSIZE, PGSIZE);  // Slide 14 step 3: copy PGSIZE bytes starting from the recorded file offset.
      iunlock(f->ip);                                                     // Release the inode lock once the read completes.
      if (n < 0) { kfree(mem); goto fail; }                               // Slide 13: treat a read error as an allocation failure and clean up.
      if (n < PGSIZE) memset(mem + n, 0, PGSIZE - n);                     // Slide 42 note: short reads (file shorter than mapping) zero-fill the tail safely.
    }

    if (mappages(p->pagetable, va_start + i * PGSIZE, PGSIZE,
                 (uint64)mem, perm) != 0) {                               // Slide 15 step 4: install PTEs so the user can touch the eager pages without faulting.
      kfree(mem);                                                         // mappages failed, so release the page we just kalloc'd before unwinding.
      goto fail;                                                          // Slide 13: aborting mmap also tears down any earlier pages we mapped.
    }
  }

  return va_start;                          // Slide 11 return contract: hand back MMAPBASE+addr to user space on success.

fail:
  // Slide 13 cleanup path — undo every partial mapping and free the slot so the user sees a clean failure (0).
  for (uint64 j = 0; j < npages; j++) {
    uint64 va = va_start + j * PGSIZE;
    pte_t *pte = walk(p->pagetable, va, 0);   // Slide 15 step 4: walk down to the leaf PTE we might already have installed.
    if (pte && (*pte & PTE_V)) {              // Only roll back pages we actually mapped; lazy pages were never touched.
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);                       // Slide 14 cleanup: return the physical page to the free pool so freemem() balances out.
      *pte = 0;                               // Clear the PTE so freewalk() does not later panic over a stale leaf.
    }
  }
  if (!anonymous && m->f) fileclose(m->f);    // Drop the filedup'd reference we took above so the file's refcount stays accurate.
  clear_mmap_slot(m);                         // Slide 20: reset the slot so the next mmap() can reuse it.
  return 0;                                   // Slide 11/13 return contract: 0 indicates failure to the user.
}

// This cleanup_proc_mmap() helper was written with Claude assistance for the assignment.
// Slide 26 requires munmap to free mapped pages, but freeproc() may run before Part B's munmap is implemented — without this defensive sweep, freewalk() would panic on leftover leaf PTEs at process exit.
void
cleanup_proc_mmap(struct proc *p)
{
  if (!p || !p->pagetable) return; // Skip slots that have no proc/page table (e.g., early allocation failures).

  for (int i = 0; i < MAXMMAP; i++) {           // Slide 20: walk every slot because mappings can appear in any order.
    struct mmap_area *m = &p->mmap_areas[i];    // Take a pointer for readability when we touch multiple fields.
    if (is_free_mmap_slot(m)) continue;         // Slide 20 free-slot convention: ignore entries that already report length == 0.

    uint64 npages = (uint64)m->length / PGSIZE; // Slide 18/26: each entry can cover multiple 4 KB pages, so we iterate per page.
    for (uint64 j = 0; j < npages; j++) {
      uint64 va = m->addr + j * PGSIZE;
      pte_t *pte = walk(p->pagetable, va, 0);   // Slide 23~25: lazy pages may have no PTE at all, so probe without allocating intermediate levels.
      if (pte && (*pte & PTE_V)) {              // Slide 26 rule: free only pages that were actually mapped.
        uint64 pa = PTE2PA(*pte);
        kfree((void*)pa);                       // Slide 27: returning the page bumps the free-page counter so freemem() reflects the cleanup.
        *pte = 0;                               // Clear the leaf so the subsequent freewalk() sees a clean table.
      }
    }
    if (m->f) fileclose(m->f);                  // Slide 14~17: release the filedup'd reference for file-backed mappings.
    clear_mmap_slot(m);                         // Slide 20: mark the slot free again so any later code sees a consistent state.
  }
}

// This munmap() implementation was written with Codex assistance for the assignment.
// Slide 26 requires removing the mapping that starts at addr, freeing any allocated pages, and dropping the file reference; lazy pages that were never faulted in are simply skipped.
int
munmap(uint64 addr)
{
  struct proc *p = myproc(); // munmap() always operates on the current process's mapping table.
  struct mmap_area *m = 0; // Keep the matched slot pointer so the page teardown and slot cleanup use the same metadata entry.
  uint64 npages; // Cache the mapping length in pages because both the PTE cleanup loop and return path need it.

  if(addr % PGSIZE != 0)
    return -1; // The project requires the unmap start address itself to be page-aligned.

  for(int i = 0; i < MAXMMAP; i++) { // Search the process-local mmap table for the region whose start address matches the request exactly.
    if(!is_free_mmap_slot(&p->mmap_areas[i]) && p->mmap_areas[i].addr == addr) {
      m = &p->mmap_areas[i]; // Remember the matching slot so the teardown loop can use its length/file metadata.
      break; // There can be at most one slot with a given start address because mmap() already rejects overlaps.
    }
  }
  if(m == 0)
    return -1; // If no recorded mapping begins at addr, the call fails exactly as the spec says.

  npages = (uint64)m->length / PGSIZE; // munmap() must visit each page so lazy and eager pages can be handled uniformly.
  for(uint64 i = 0; i < npages; i++) {
    uint64 va = m->addr + i * PGSIZE; // Compute the virtual address of the current page inside the mapping.
    pte_t *pte = walk(p->pagetable, va, 0); // Probe the leaf PTE without allocating intermediate tables because lazy pages may still be absent.
    if(pte == 0)
      continue; // Missing page-table levels mean this page was never faulted in, so there is nothing to free.
    if((*pte & PTE_V) == 0)
      continue; // Invalid leaf entries are also fine for lazy pages that were recorded but never allocated.
    kfree((void *)PTE2PA(*pte)); // Free only the physical pages that were actually allocated so freemem() rises by the correct amount.
    *pte = 0; // Clear the leaf entry so later walks and freewalk() see the page as unmapped.
  }

  if(m->f)
    fileclose(m->f); // Drop the file reference taken by mmap() once the mapping is fully removed.
  clear_mmap_slot(m); // Mark the slot free again so later mmap() calls may reuse it.
  return 1; // munmap() returns 1 on success per the project specification.
}
