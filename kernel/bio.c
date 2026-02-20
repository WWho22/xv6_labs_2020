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

struct {
  struct spinlock lock;
  struct spinlock bucket_lock[BUFHASH_NUM];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf bucket[BUFHASH_NUM];
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  char buffer[32];
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < BUFHASH_NUM; i++)
  {
    snprintf(buffer, sizeof(buffer), "bucket_lock%d", i);
    initlock(&bcache.bucket_lock[i], buffer);
    // Create linked list of buffers
    bcache.bucket[i].prev = &bcache.bucket[i];
    bcache.bucket[i].next = &bcache.bucket[i];
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->ticks = 0;
    initlock(&b->time_lock, "time_lock");
    b->next = bcache.bucket[0].next;
    b->prev = &bcache.bucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].next->prev = b;
    bcache.bucket[0].next = b;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  // Is the block already cached?
  int hash_index = blockno % BUFHASH_NUM;
  acquire(&bcache.bucket_lock[hash_index]);
  for(b = bcache.bucket[hash_index].next; b != &bcache.bucket[hash_index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      // acquire(&b->time_lock);
      b->ticks = ticks;
      // release(&b->time_lock);
      release(&bcache.bucket_lock[hash_index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached.
  struct buf *result = 0;
  int ticks_min = 0x7fffffff;
  for(int i = hash_index,cycle_count = 0; cycle_count < BUFHASH_NUM; i = (i + 1) % BUFHASH_NUM)
  {
  // for(int i = 0 ; i < BUFHASH_NUM; i++)
  // {
    ++cycle_count;
    if(i == hash_index) 
    {
    }
    else
    {
      if(!holding(&bcache.bucket_lock[i]))
      {
        acquire(&bcache.bucket_lock[i]);
      }
      else continue;
    }
    
    for(b = bcache.bucket[i].prev; b != &bcache.bucket[i]; b = b->prev)
    {
      if(b->refcnt == 0 && b->ticks < ticks_min) {
        ticks_min = b->ticks;
        result = b;
      }
    }
    if(result != 0) 
    {
      // no one is waiting for it.
      if(i != hash_index) 
      {
        result->next->prev = result->prev;
        result->prev->next = result->next;
        release(&bcache.bucket_lock[i]);
        result->next = bcache.bucket[hash_index].next;
        result->prev = &bcache.bucket[hash_index];
        bcache.bucket[hash_index].next->prev = result;
        bcache.bucket[hash_index].next = result; 
      }
      // acquire(&result->time_lock);  
      result->ticks = ticks;
      // release(&result->time_lock);
      result->dev = dev;
      result->blockno = blockno;
      result->valid = 0;
      result->refcnt = 1;  
      release(&bcache.bucket_lock[hash_index]); 
      acquiresleep(&result->lock);
      return result;
    }
    else
    {
      if(i != hash_index)
      {
        release(&bcache.bucket_lock[i]);
      }
    }
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

  int hash_index = b->blockno % BUFHASH_NUM;
  releasesleep(&b->lock);

  acquire(&bcache.bucket_lock[hash_index]);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->ticks = ticks;
  // }
  b->ticks = ticks;
  release(&bcache.bucket_lock[hash_index]);
}

void
bpin(struct buf *b) {
  int hash_index = b->blockno % BUFHASH_NUM;
  acquire(&bcache.bucket_lock[hash_index]);
  b->refcnt++;
  release(&bcache.bucket_lock[hash_index]);
}

void
bunpin(struct buf *b) {
  int hash_index = b->blockno % BUFHASH_NUM;
  acquire(&bcache.bucket_lock[hash_index]);
  b->refcnt--;
  release(&bcache.bucket_lock[hash_index]);
}


