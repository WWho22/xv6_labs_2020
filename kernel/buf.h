struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  struct spinlock time_lock;
  uint refcnt;
  uint ticks;    // LRU cache list
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

