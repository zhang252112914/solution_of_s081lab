// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define HASH(blockno) ((blockno) % NBUCKET)
extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf bucket_head[NBUCKET];   //use two-way linked list to maintain the LRU order
  struct spinlock bucket_lock[NBUCKET];
  int size; // record the number of used blocks, when we need a free block, we first check this value, if there is no free block, we will conduct eviction
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  bcache.size = 0;
  // Create linked list of buffers
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucket_lock[i], "bcache_bucket");
  }
  //because the use of buffer cache is dynamic, so we don't need to separate cache blocks to each bucket equally
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){   
    initsleeplock(&b->lock, "buffer");
  }
}

struct buf*
lookup_LRU(int idx)
{
  struct buf *pre = &bcache.bucket_head[idx];
  struct buf *cur = pre->next;
  struct buf *min_buf = 0;
  struct buf *min_pre = 0;
  int minitime = ticks;

  while(cur) {
    if(cur->refcnt == 0 && cur->time < minitime){
        minitime = cur->time;
        min_buf = cur;
        min_pre = pre;
    }
    pre = cur;
    cur = cur->next;
  }

  if(min_buf){
    min_pre->next = min_buf->next;
    return min_buf;
  }
  
  return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = HASH(blockno);

  struct buf *min_buf = 0;
  int minitime;

  // Is the block already cached?
  // First, search in the bucket (in parallel search potential LRU block)
  acquire(&bcache.bucket_lock[idx]);
  minitime = ticks;
  for(b = bcache.bucket_head[idx].next; b ; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
    if(b->refcnt == 0 && b->time < minitime){
      minitime = b->time;
      min_buf = b;
    }
  }

  // Not cached.
  // Second, try to allocate a never-used block
  acquire(&bcache.lock);
  if(bcache.size < NBUF){
    b = &bcache.buf[bcache.size++];
    release(&bcache.lock);
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->next = bcache.bucket_head[idx].next;
    bcache.bucket_head[idx].next = b;
    release(&bcache.bucket_lock[idx]);
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.lock);

  //Third, try to recycle a LRU block in the same bucket
  if(min_buf) {
    b = min_buf;
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.bucket_lock[idx]);
    acquiresleep(&b->lock);
    return b;
  }

  release(&bcache.bucket_lock[idx]);

  // Fourth, try to recycle a LRU block in other bucket
  for(int i = 0; i < NBUCKET;) {
    //printf("i: %d\n", i);
    acquire(&bcache.bucket_lock[i]);
    b = lookup_LRU(i);
    release(&bcache.bucket_lock[i]);
    if(b) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      acquire(&bcache.bucket_lock[idx]);
      b->next = bcache.bucket_head[idx].next;
      bcache.bucket_head[idx].next = b;
      release(&bcache.bucket_lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
    if(++i == NBUCKET) {
      i = 0;
    }
    else i++;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket_id = HASH(b->blockno);
  acquire(&bcache.bucket_lock[bucket_id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->time = ticks;
  }
  
  release(&bcache.bucket_lock[bucket_id]);
}

void
bpin(struct buf *b) {
  int bucket_id = HASH(b->blockno);
  acquire(&bcache.bucket_lock[bucket_id]);
  b->refcnt++;
  release(&bcache.bucket_lock[bucket_id]);
}

void
bunpin(struct buf *b) {
  int bucket_id = HASH(b->blockno);
  acquire(&bcache.bucket_lock[bucket_id]);
  b->refcnt--;
  release(&bcache.bucket_lock[bucket_id]);
}


