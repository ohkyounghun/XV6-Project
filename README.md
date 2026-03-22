# 🧠 xv6 Project 

This project is based on the educational operating system **xv6 (RISC-V)**.  
In Assignment 1, I implemented several system calls to understand how user programs interact with the kernel.

---

## 🚀 Implemented System Calls

- **getnice(pid)**  
  Returns the nice value of a given process.

- **setnice(pid, value)**  
  Updates the nice value with proper validation (range check and process existence).

- **ps(pid)**  
  Displays process information for all processes or a specific process.

- **meminfo()**  
  Returns the amount of free physical memory (in bytes) using xv6's freelist.

- **waitpid(pid)**  
  Waits for a specific child process to terminate.

---

## 🧩 Key Concepts

- **System Call Flow**  
  User → syscall → kernel → return value

- **Process Management**  
  - Process table traversal (`struct proc`)  
  - Parent-child relationships  
  - Process states (e.g., RUNNING, ZOMBIE)

- **Memory Management**  
  - Free memory tracking using freelist (`kalloc.c`)  
  - Page-based allocation (4096 bytes per page)

- **Synchronization**  
  - Safe access to shared data using spinlocks

---

## 🛠 Tech Stack

- Language: C  
- OS: xv6 (RISC-V)  
- Emulator: QEMU  
- Environment: macOS  

---

## 👥 Collaborator

- gunuzello

---

## 📄 License

This project is based on xv6, which is licensed under the MIT License.