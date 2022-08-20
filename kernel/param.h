#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name

#define STACK_SIZE     4000
#define MAX_STACK_SIZE 4000
#define MAX_BSEM       128

#define ARR_LEN(a) (sizeof((a)) / sizeof((a)[0]))
#define ARR_END(a) (&((a)[ARR_LEN(a)]))
#define INDEX_OF(i, a) ((i) - (a))
#define FOR_EACH(i, a) for (i = (a); i < ARR_END(a); i++)