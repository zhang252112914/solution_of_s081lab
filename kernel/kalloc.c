// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

// according to a block, the ref would be better to be placed at the keme, because it also need a lock, and it also has a tight relationship with the phsical memory 
struct {
  struct spinlock lock;
  struct run *freelist;
  int ref_count[(PHYSTOP - KERNBASE) / PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    acquire(&kmem.lock);
    kmem.ref_count[((uint64)p - KERNBASE) / PGSIZE] = 1;  //because in kfree would call decre_ref to each one, so this should be initialized as 1
    release(&kmem.lock);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.

  acquire(&kmem.lock);
  if(--kmem.ref_count[((uint64)pa - KERNBASE) / PGSIZE] == 0){
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
    
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.ref_count[((uint64)r - KERNBASE) / PGSIZE] = 1;
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

//expect pa is the start of a page
void
incre_ref(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.ref_count[(pa - KERNBASE) / PGSIZE]++;
  release(&kmem.lock);
}

void
decre_ref(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.ref_count[(pa - KERNBASE) / PGSIZE]--;
  if(kmem.ref_count[(pa - KERNBASE) / PGSIZE] < 0) panic("wrong kfree cause ref decresed to negative");
  release(&kmem.lock);
}

int
get_ref(uint64 pa){
  int ref;
  acquire(&kmem.lock);
  ref = kmem.ref_count[(pa - KERNBASE) / PGSIZE];
  release(&kmem.lock);
  return ref;
}