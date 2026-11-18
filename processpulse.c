/**
 * Project: ProcessPulse – Linux Process Manager & Monitor
 * Author: Ashish Raikwar
 * Student ID: B24CS010
 * Course: Operating Systems (OS)
 * Repository: https://github.com/araikwar745/ProcessPulse
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_WORKERS 32
#define MSG_SIZE 256

typedef struct { pid_t pid; int pipe_fd; int active; } Worker;
static Worker workers[MAX_WORKERS];
static int worker_count = 0;

static void reap_children(void) {
    for (int i = 0; i < worker_count; ++i) {
        if (!workers[i].active) continue;
        int status;
        pid_t r = waitpid(workers[i].pid, &status, WNOHANG);
        if (r == workers[i].pid || (r < 0 && errno == ECHILD)) {
            workers[i].active = 0;
            if (workers[i].pipe_fd >= 0) close(workers[i].pipe_fd);
            workers[i].pipe_fd = -1;
        }
    }
}

static Worker *find_worker(pid_t pid) {
    for (int i = 0; i < worker_count; ++i)
        if (workers[i].pid == pid && workers[i].active) return &workers[i];
    return NULL;
}

static void child_worker(int read_fd) {
    close(STDIN_FILENO);
    printf("[child %d] Started. Waiting for parent messages...\n", getpid());
    fflush(stdout);
    struct pollfd pfd = { .fd = read_fd, .events = POLLIN | POLLHUP };
    char buf[MSG_SIZE];
    for (;;) {
        int ready = poll(&pfd, 1, 3000);
        if (ready < 0) { if (errno == EINTR) continue; break; }
        if (ready == 0) {
            printf("[child %d] heartbeat: still running\n", getpid());
            fflush(stdout);
            continue;
        }
        if (pfd.revents & POLLIN) {
            ssize_t n = read(read_fd, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            buf[n] = '\0';
            printf("[child %d] IPC message received: %s\n", getpid(), buf);
            fflush(stdout);
        }
        if (pfd.revents & POLLHUP) break;
    }
    close(read_fd);
    _exit(0);
}

static void spawn_worker(void) {
    if (worker_count >= MAX_WORKERS) { puts("Worker table is full."); return; }
    int p[2];
    if (pipe(p) < 0) { perror("pipe"); return; }
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(p[0]); close(p[1]); return; }
    if (pid == 0) {
        close(p[1]);
        /* Do not keep write ends belonging to older children. */
        for (int i = 0; i < worker_count; ++i)
            if (workers[i].pipe_fd >= 0) close(workers[i].pipe_fd);
        child_worker(p[0]);
    }
    close(p[0]);
    workers[worker_count++] = (Worker){ .pid = pid, .pipe_fd = p[1], .active = 1 };
    printf("Created child process successfully. PID = %d\n", pid);
}

static void show_process_info(pid_t pid) {
    char path[128], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) { printf("PID %d: process information unavailable (may have exited).\n", pid); return; }
    char name[128] = "?", state[128] = "?", vmrss[128] = "?", threads[128] = "?";
    while (fgets(line, sizeof(line), f)) {
        char *dest = NULL;
        if (strncmp(line, "Name:", 5) == 0) dest = name;
        else if (strncmp(line, "State:", 6) == 0) dest = state;
        else if (strncmp(line, "VmRSS:", 6) == 0) dest = vmrss;
        else if (strncmp(line, "Threads:", 8) == 0) dest = threads;
        if (dest) {
            char *value = strchr(line, ':') + 1;
            while (*value == ' ' || *value == '\t') value++;
            value[strcspn(value, "\r\n")] = '\0';
            snprintf(dest, 128, "%s", value);
        }
    }
    fclose(f);
    printf("PID: %-7d Name:%s | State:%s | RSS:%s | Threads:%s\n", pid, name, state, vmrss, threads);
}

static void list_workers(void) {
    reap_children();
    puts("\n--- ProcessPulse: managed processes ---");
    int found = 0;
    for (int i = 0; i < worker_count; ++i) {
        if (workers[i].active) { show_process_info(workers[i].pid); found = 1; }
    }
    if (!found) puts("No active managed processes. Create one using option 1.");
    puts("---------------------------------------");
}

static int read_pid(pid_t *pid) {
    printf("Enter PID: ");
    fflush(stdout);
    if (scanf("%d", pid) != 1) { int c; while ((c = getchar()) != '\n' && c != EOF) {} return 0; }
    return 1;
}

static void control_worker(int sig, const char *action) {
    pid_t pid;
    if (!read_pid(&pid)) { puts("Invalid PID."); return; }
    Worker *w = find_worker(pid);
    if (!w) { puts("PID is not an active ProcessPulse child."); return; }
    if (sig == SIGTERM) kill(pid, SIGCONT); /* A stopped child must be resumed to handle termination. */
    if (kill(pid, sig) == 0) printf("Sent %s signal to PID %d.\n", action, pid);
    else perror("kill");
    if (sig == SIGTERM) {
        int status;
        if (waitpid(pid, &status, 0) == pid) {
            w->active = 0; close(w->pipe_fd); w->pipe_fd = -1;
            printf("PID %d terminated and reaped.\n", pid);
        } else if (errno != ECHILD) perror("waitpid");
    }
}

static void send_message(void) {
    pid_t pid;
    if (!read_pid(&pid)) { puts("Invalid PID."); return; }
    Worker *w = find_worker(pid);
    if (!w) { puts("PID is not an active ProcessPulse child."); return; }
    char msg[MSG_SIZE];
    printf("Message (max 200 characters): ");
    fflush(stdout);
    int c; while ((c = getchar()) != '\n' && c != EOF) {}
    if (!fgets(msg, sizeof(msg), stdin)) return;
    msg[strcspn(msg, "\n")] = '\0';
    if (write(w->pipe_fd, msg, strlen(msg) + 1) < 0) perror("write(pipe)");
    else puts("Message sent to child through an IPC pipe.");
}

static void cleanup(void) {
    for (int i = 0; i < worker_count; ++i) {
        if (workers[i].active) {
            kill(workers[i].pid, SIGCONT);
            kill(workers[i].pid, SIGTERM);
            waitpid(workers[i].pid, NULL, 0);
            workers[i].active = 0;
        }
        if (workers[i].pipe_fd >= 0) close(workers[i].pipe_fd);
    }
}

int main(void) {
    int choice;
    puts("\n==============================================");
    puts("       ProcessPulse - Linux Process Manager");
    puts("==============================================");
    for (;;) {
        reap_children();
        puts("\n1. Create worker process (fork)");
        puts("2. List and monitor managed processes (/proc)");
        puts("3. Pause a process (SIGSTOP)");
        puts("4. Resume a process (SIGCONT)");
        puts("5. Terminate a process (SIGTERM + waitpid)");
        puts("6. Send IPC message to a process (pipe)");
        puts("0. Exit");
        printf("Select an option: ");
        fflush(stdout);
        if (scanf("%d", &choice) != 1) {
            if (feof(stdin)) break;
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            puts("Please enter a number."); continue;
        }
        switch (choice) {
            case 1: spawn_worker(); break;
            case 2: list_workers(); break;
            case 3: control_worker(SIGSTOP, "SIGSTOP"); break;
            case 4: control_worker(SIGCONT, "SIGCONT"); break;
            case 5: control_worker(SIGTERM, "SIGTERM"); break;
            case 6: send_message(); break;
            case 0: cleanup(); puts("All managed children cleaned up. Goodbye!"); return 0;
            default: puts("Invalid option. Choose 0-6.");
        }
    }
    cleanup();
    return 0;
}
