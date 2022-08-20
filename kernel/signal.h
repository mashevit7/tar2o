#define SIG_DFL 0 /* default signal handling */
#define SIG_IGN 1 /* ignore signal */
#define SIGKILL 9
#define SIGSTOP 17

// Can be blocked, ignored or set to a custom handler.
// However, it will always make the process continue if it was sent SIGSTOP (even if ignored or blocked).
// Also, if the user specified a custom handler and the signal is blocked,
// the handler will not be executed (as usual for signal handlers).
// see https://www.gnu.org/software/libc/manual/html_node/Job-Control-Signals.html
// for further details.
#define SIGCONT 19

#define MAX_SIG 32
