# 🧠 xv6 OS Project

This project extends the educational operating system **xv6 (RISC-V)**
with custom system calls and a priority-based CPU scheduler.

---

## 🚀 System Calls

- **getnice(pid)** — Returns the nice value of a process
- **setnice(pid, value)** — Updates the nice value with validation
- **ps(pid)** — Displays process information
- **meminfo()** — Returns free physical memory in bytes
- **waitpid(pid)** — Waits for a specific child process

---

## ⚙️ CPU Scheduler (EEVDF)

Replaced xv6's default Round-Robin scheduler with **EEVDF
(Earliest Eligible Virtual Deadline First)** to support
priority-based CPU time allocation using nice values.

---

## 🧩 Key Concepts

- System call flow: User → syscall → kernel → return value
- Process management: table traversal, parent-child, states
- Memory management: freelist, page-based allocation
- Synchronization: spinlocks for shared data

---

## 🛠 Tech Stack

- Language: C
- OS: xv6 (RISC-V) / QEMU / macOS

---

## 👥 Collaborators

- gunuzello

---

## 📄 License

- Based on xv6, licensed under the [MIT License](LICENSE).
