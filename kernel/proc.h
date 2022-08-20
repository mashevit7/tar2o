#include "signal.h"

// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct thread *thread;      // The thread running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

// THREADS-TODO: maybe we want to also check if the process was killed
#define THREAD_IS_KILLED(t) ((t)->killed)

// THREADS: threads transition change might be needed in files:
//   * console.c
//   * exec.c
//   * -- file.c
//   * pipe.c
//   * proc.c
//   * ?? sleeplock.c
//   * syscall.c
//   * sysproc.c
//   * trap.c

enum thread_state { T_UNUSED, T_USED, T_SLEEPING, T_RUNNABLE, T_RUNNING, T_ZOMBIE };

struct thread {
  struct proc *process; // The process this threads belongs to
  struct spinlock lock;

  // lock must be held when using these:
  enum thread_state state;     // Thread state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status (return on join)
  int tid;                     // Thread ID

  // THREAD: kstack is now void*
  void* kstack;                  // Virtual address of kernel stack
  struct trapframe *trapframe;   // data page for trampoline.S
  struct context context;        // swtch() here to run process
  char name[16];                 // Thread name (debugging)

  int waiting_on_me_count;       // how many threads are currently joining this thread, see kthread_join for notes

  // TODO: signals and multi-threading

  // The following fields were not transfered because
  // they are part of the shared state of all the threads
  // of this process.
  
  // Not transfered fields:
  //   -- uint64 sz;                  // Size of process memory (bytes)
  //   -- pagetable_t pagetable;      // User page table
  //   -- struct file *ofile[NOFILE]; // Open files
  //   -- struct inode *cwd;          // Current directory
};

enum procstate { P_UNUSED, P_USED, P_SCHEDULABLE, P_ZOMBIE };

#define NTHREAD 8

// Per-process state

#define KILLED_DFL 1
#define KILLED_SPECIAL 2
#define KILLED_XSTATUS -1

struct proc {
  struct spinlock lock;

  int next_tid;

  // lock must be held when using these:
  enum procstate state;        // Process state
                               // Only threads can sleep on something since a process doesn't execute anything,
                               // it's the collection of it's threads.
  // TODO: Collapse state
  //   Change to 'collapse_state' which is an enum
  //   stating why the process is collapsing.
  //   For now, either because it is killed or one thread is executing a exec
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // proc_tree_lock must be held when using this:
  struct proc *parent;         // Parent process


  // TODO: look for all the places where we might need to use
  // synchronization now because of multiple threads of the same processes
  // accessing a shared process resource.
  // For example 'growproc' in proc.c
  // ?? these are private to the process, so p->lock need not be held. ??
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  void *kpage_trapframes;      // A pointer to the beginning of memory page for the trapframes
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  struct thread threads[NTHREAD];
  struct thread *thread0;
  int threads_alive_count;
  
  uint pending_signals;
  uint signal_mask;         // specifies the signals which are blocked for this process
  uint signal_mask_backup;  // used for restoring the original signal mask after a custom signal handler
  void *signal_handlers[MAX_SIG];
  uint signal_handles_mask[MAX_SIG];
  struct trapframe *backup_trapframe;

  // controls whether the process was freezed by a SIGSTOP signal. let's handling SIGCONT know whether to yield or not.
  int freezed;
  
  int special_signum_handling; // determines the signum which caused to process to handle a SIGSTOP-like signal
  int in_custom_handler; // determines whether the process is in the middle of executing a custom signal handler

  // SIGNAL: modification places: allocproc, freeproc, fork, sigprocmask, sigaction, sigret, handle_proc_signals, kill

  // THREADS:
  // The following fields have been removed because these
  // are only relavent for a task that can execute code.
  // A process is a collection of threads with shared state,
  // but cannot execute code on it's own.

  // Removed fields:
  //   -- void   *chan;                // If non-zero, sleeping on chan
  //   -- uint64 kstack;               // Virtual address of kernel stack
  //   -- struct trapframe *trapframe; // data page for trampoline.S
  //   -- struct context context;      // swtch() here to run process
}; 
