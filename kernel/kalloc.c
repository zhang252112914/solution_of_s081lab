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

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmem_array[NCPU];

void
kinit()
{
  for(int i = 0; i < NCPU; i++){
    initlock(&kmem_array[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off(); // disable interrupts when acquire the cpu_id
  int current_cpu = cpuid();
  struct kmem *current_kmem = &kmem_array[current_cpu];  //需要深复制
  acquire(&current_kmem->lock);
  r->next = current_kmem->freelist;  //头插法
  current_kmem->freelist = r;
  release(&current_kmem->lock);
  pop_off();
}

// Steal one page from other CPUs.
struct run*
ksteal(int cpuid)
{
  struct run *r = 0;
  for(int i = 0; i < NCPU; i++){
    if(i == cpuid) continue;
    struct kmem *current_kmem = &kmem_array[i];
    acquire(&current_kmem->lock);
    r = current_kmem->freelist;
    if(r){
      current_kmem->freelist = r->next;
      release(&current_kmem->lock);
      return r;
    }
    release(&current_kmem->lock);
  }
  return 0;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off(); // disable interrupts when acquire the cpu_id
  int current_cpu = cpuid();
  struct kmem *current_kmem = &kmem_array[current_cpu];

  acquire(&current_kmem->lock);
  r = current_kmem->freelist;
  if(r){
    current_kmem->freelist = r->next;
  }
  else{
    r = ksteal(current_cpu); // input cpuid to avoid deadlock
  }

  release(&current_kmem->lock);
  pop_off();
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
