#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

// This PA3 constant block was written with Claude assistance for the assignment.
// Slide 19 of the project spec requires these macros to live in kernel/param.h so the kernel can identify mmap protection/flags and bound the mmap region.
#define PROT_NONE     0x0         // Define a zero protection value so we can defensively reject a NULL-prot request before touching page tables.
#define PROT_READ     0x1         // Slide 19 protection flag value, matched by user space when requesting a readable mapping.
#define PROT_WRITE    0x2         // Slide 19 protection flag value, matched by user space when requesting a writable mapping.
#define MAP_ANONYMOUS 0x1         // Slide 12/19 flag bit indicating the mapping is NOT backed by a file (fd must be -1).
#define MAP_POPULATE  0x2         // Slide 12/19 flag bit asking the kernel to allocate physical pages and install PTEs immediately (eager path).
#define MMAPBASE      0x40000000L // Slide 19 base virtual address that separates the mmap region from ordinary user memory below it.
#define MAXMMAP       64          // Slide 19/20 cap on how many mmap_area entries one process can hold simultaneously.

