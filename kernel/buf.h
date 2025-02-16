struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf? // what dose this mean?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;   // reference count
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

