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

#define BUCKETSIZE 13

char bclname[BUCKETSIZE][15];

struct {
  struct spinlock lock[BUCKETSIZE];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUCKETSIZE];
  struct spinlock big_lock;
} bcache;

int ihash(uint blockno) {
  return blockno % BUCKETSIZE;
}

void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < BUCKETSIZE; i++) {
    snprintf(bclname[i], 15, "bcache:%d", i);
    initlock(&bcache.lock[i], bclname[i]);
  }
  // initlock(&bcache.lock, "bcache");
  initlock(&bcache.big_lock, "bcache:big_lock");
  for(int i = 0; i < BUCKETSIZE; i++) {
    bcache.head[i].prev = &bcache.head[i]; // 初始化head节点
    bcache.head[i].next = &bcache.head[i];
  }
  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  int index = 0;
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.head[index].next;
    b->prev = &bcache.head[index];
    initsleeplock(&b->lock, "buffer");
    bcache.head[index].next->prev = b;
    bcache.head[index].next = b;
    index = (index + 1) % BUCKETSIZE;
  }
  #if 0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  #endif
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = ihash(blockno);

  acquire(&bcache.lock[index]);

  // Is the block already cached?
  for(b = bcache.head[index].next; b != &bcache.head[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock); // 获取sleep lock
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head[index].prev; b != &bcache.head[index]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[index]);
  // 此处需要调整锁的顺序，避免发生死锁，所以需要先放弃锁,再拿锁-->这是一个调整锁顺序的过程
  acquire(&bcache.big_lock); // 保证锁的顺序是big_lock -- > lock[index]
  acquire(&bcache.lock[index]);

  // 存在窗口期，再次循环，避免发生block被缓存了两次。
  for(b = bcache.head[index].next; b != &bcache.head[index]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      release(&bcache.big_lock);
      return b;
    }
  }

  for(b = bcache.head[index].prev; b != &bcache.head[index]; b = b->prev) {
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      release(&bcache.big_lock);
      return b;
    }
  }


  int new_index = (index + 1) % BUCKETSIZE;
  while(new_index != index) {
    acquire(&bcache.lock[new_index]);
    for(b = bcache.head[new_index].prev; b != &bcache.head[new_index]; b = b->prev) {
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        // 修复链表
        b->next->prev = b->prev; // 修复链表
        b->prev->next = b->next;

        // 插入到bcache.bucket[index]的头部
        b->next = bcache.head[index].next;
        b->prev = &bcache.head[index];
        bcache.head[index].next->prev = b;
        bcache.head[index].next = b;

        release(&bcache.lock[new_index]); // 按顺序释放锁
        acquiresleep(&b->lock);
        release(&bcache.lock[index]);
        release(&bcache.big_lock);
        return b;
      }
    }
    release(&bcache.lock[new_index]);
    new_index = (new_index + 1) % BUCKETSIZE; // 轮询
  }

  release(&bcache.lock[index]);
  release(&bcache.big_lock);
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
  int index = ihash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[index].next;
    b->prev = &bcache.head[index];
    bcache.head[index].next->prev = b;
    bcache.head[index].next = b;
  }
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = ihash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = ihash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


