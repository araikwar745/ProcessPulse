# ProcessPulse – Linux Process Manager & Monitor

**Name:** Ashish Raikwar  
**Student ID:** B24CS010  
**Course:** Operating Systems (OS)  
**GitHub Repository:** [https://github.com/araikwar745/ProcessPulse](https://github.com/araikwar745/ProcessPulse)  

A practical, hands-on Operating Systems project written in C (C11). It demonstrates real Linux process-management mechanisms rather than only simulating them in memory.

## 🚀 Key Features
- **Process Creation (`fork`)**: Spawn multiple child processes with isolated execution context.
- **Kernel Introspection (`/proc`)**: Parse `/proc/<pid>/status` for real-time process metadata (Name, State, Resident Memory `VmRSS`, Thread count).
- **Process Control (`signals`)**:
  - `SIGSTOP` to pause/freeze process execution.
  - `SIGCONT` to resume suspended processes.
  - `SIGTERM` with `waitpid()` for clean termination.
- **Inter-Process Communication (`IPC Pipes`)**: Parent-to-child message communication through unidirectional POSIX pipes.
- **I/O Multiplexing (`poll`)**: Event-driven worker loop with heartbeat timeout (0% idle CPU usage).
- **Zombie Process Prevention**: Automatic child reaping using non-blocking `waitpid(..., WNOHANG)`.
- **Graceful Cleanup**: Automatic shutdown handler ensuring zero orphan or zombie processes on exit.

---

## 🛠️ Requirements & Environment
- **OS**: Linux / Ubuntu / WSL2 (Windows Subsystem for Linux)
- **Compiler**: GCC (`-std=c11`)
- **Build tool**: GNU Make

---

## ⚡ Quick Start

```bash
# 1. Compile the project
make clean && make

# 2. Run ProcessPulse
./processpulse
```

Or compile manually without Make:
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -o processpulse processpulse.c
./processpulse
```

---

## 📸 Output Screenshots
Verified output screenshots of every scenario are available in the [`screenshots/`](screenshots/) directory:
1. `01_compilation_and_menu.png` – GCC build with flags and main interactive menu
2. `02_process_creation.png` – Spawning workers with `fork()` and checking `/proc`
3. `03_ipc_pipe_communication.png` – Parent sending messages to child through pipe
4. `04_signal_sigstop_pause.png` – Suspending process and observing `State: T (stopped)`
5. `05_signal_sigcont_resume.png` – Resuming process and observing `State: S (sleeping)`
6. `06_termination_and_reap.png` – Terminating process and synchronized reaping via `waitpid`
7. `07_graceful_exit_cleanup.png` – Clean exit and verification of zero orphan processes

---

## 📚 OS Concepts Mapping
| OS Concept | Kernel / POSIX Primitive | Location in Code |
|---|---|---|
| Process Creation | `fork()` | `spawn_worker()` |
| Inter-Process Communication | `pipe()`, `read()`, `write()` | `spawn_worker()`, `send_message()`, `child_worker()` |
| I/O Multiplexing & Heartbeat | `poll()` | `child_worker()` |
| Process Suspension | `kill(pid, SIGSTOP)` | `control_worker()` |
| Process Resumption | `kill(pid, SIGCONT)` | `control_worker()` |
| Child Reaping (Non-blocking) | `waitpid(..., WNOHANG)` | `reap_children()` |
| Synchronous Termination | `kill(..., SIGTERM)` + `waitpid()` | `control_worker()` |
| Kernel Introspection | `/proc/<pid>/status` parsing | `show_process_info()` |

---

## 📄 Complete Project Report
For the complete academic project report, architecture diagrams, and Viva Voce Q&A cheat sheet, refer to [PROJECT_REPORT.md](PROJECT_REPORT.md).
