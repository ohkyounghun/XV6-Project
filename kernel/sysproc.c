#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// AI was used (ChatGPT explained syscall wrapper structure)
// sys_meminfo: system call handler for meminfo()
// This function is invoked after a trap (ecall) from user space,
// and returns the current free memory by calling freemem().
uint64
sys_meminfo(void)
{
  return freemem();
}

// This sys_getnice() wrapper was written with Codex assistance for the assignment.
// It reads the pid argument from user space and forwards it to the kernel helper.
uint64
sys_getnice(void)
{
  int pid; // Store the pid argument fetched from the user register set.
  argint(0, &pid); // Read the first syscall argument, which is the target pid.
  return getnice(pid); // Call the kernel helper and return its result to user space.
}

// This sys_setnice() wrapper was written with Codex assistance for the assignment.
// It reads the pid and value arguments from user space and forwards them to the kernel helper.
uint64
sys_setnice(void)
{
  int pid; // Store the pid argument fetched from the user register set.
  int value; // Store the requested new nice value fetched from the user register set.
  argint(0, &pid); // Read the first syscall argument, which is the target pid.
  argint(1, &value); // Read the second syscall argument, which is the new nice value to apply.
  return setnice(pid, value); // Call the kernel helper and return success or failure back to user space.
}

// This sys_ps() wrapper was written with Codex assistance for the assignment.
// It reads the pid argument from user space and forwards it to the kernel helper that prints process information.
uint64
sys_ps(void)
{
  int pid; // Store the pid argument fetched from the user register set.
  argint(0, &pid); // Read the first syscall argument, which selects one process or all processes when it is zero.
  return ps(pid); // Call the kernel helper and return its success code back to user space.
}

// AI was used (ChatGPT explained syscall wrapper structure)
// sys_waitpid: System call handler for waitpid()
uint64
sys_waitpid(void)
{
  int pid;
  argint(0, &pid);
  return waitpid(pid);
}

// This sys_mmap() wrapper was written with Claude assistance for the assignment.
// Slide 11 of the project spec defines a 6-argument prototype, so we fetch each user-space argument with the matching helper before delegating to mmap() in kernel/mmap.c.
uint64
sys_mmap(void)
{
  uint64 addr;   // Slide 11 first argument: start address relative to MMAPBASE, fetched as a 64-bit value.
  int length;    // Slide 11 second argument: requested mapping length in bytes, fetched as a 32-bit signed int.
  int prot;      // Slide 11 third argument: combination of PROT_READ / PROT_WRITE bits requested by the user.
  int flags;     // Slide 11 fourth argument: combination of MAP_ANONYMOUS / MAP_POPULATE flags selected by the user.
  int fd;        // Slide 11 fifth argument: file descriptor for file-backed mappings or -1 for anonymous mappings.
  int offset;    // Slide 11 sixth argument: starting file offset for file-backed mappings or 0 for anonymous mappings.
  argaddr(0, &addr);   // Read the addr argument as a virtual address so values up to 64 bits survive the user-to-kernel transition.
  argint(1, &length);  // Read the length argument as a 32-bit int because xv6's argint helper handles signed integers safely.
  argint(2, &prot);    // Read the prot bitmask the user requested.
  argint(3, &flags);   // Read the flags bitmask that selects eager/lazy and file/anonymous behavior.
  argint(4, &fd);      // Read the fd argument so the kernel can locate p->ofile[fd] later when needed.
  argint(5, &offset);  // Read the file offset for use by either eager loading or Part B's lazy fault handler.
  return mmap(addr, length, prot, flags, fd, offset); // Slide 11 return contract: 0 on failure, otherwise MMAPBASE+addr from mmap().
}

// This sys_munmap() wrapper was written with Claude assistance for the assignment.
// Slide 26 of the project spec defines munmap() as taking a single virtual address argument; Part B owns the body so this wrapper only marshals the argument and forwards.
uint64
sys_munmap(void)
{
  uint64 addr;       // Slide 26 argument: start virtual address of the mapping to remove.
  argaddr(0, &addr); // Fetch the addr argument as a 64-bit virtual address from the user trap frame.
  return munmap(addr); // Slide 26 return contract: 1 on success, -1 on failure (Part B implements the actual logic).
}

// This sys_freemem() wrapper was written with Claude assistance for the assignment.
// Slide 27 of the project spec asks for the current number of free physical memory pages, so we forward to freemem_pages() which keeps an O(1) counter.
uint64
sys_freemem(void)
{
  return freemem_pages(); // Slide 27 semantics: number of free physical pages (not bytes) reflected at the moment of the call.
}
