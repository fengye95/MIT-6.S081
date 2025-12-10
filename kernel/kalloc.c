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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int refcnt[(PHYSTOP - KERNBASE)/PGSIZE];
} page;

void pageinit(void)
{
  initlock(&page.lock, "page");
  for(int i = 0; i < (PHYSTOP - KERNBASE)/PGSIZE; i++) {
    page.refcnt[i] = 0;
  }
}

int pa_in_ram(uint64 pa) {
  return KERNBASE <= pa && pa < PHYSTOP;
}

void acquire_page_lock() {
  acquire(&page.lock);
}

void release_page_lock() {
  release(&page.lock);
}

void inc_refcnt(uint64 pa) {
  if (!pa_in_ram(pa)) {
    panic("inc_refcnt: pa not in ram");
  }
  int index = (pa - KERNBASE) / PGSIZE;
  page.refcnt[index]++;
}

void dec_refcnt(uint64 pa) {
  if (!pa_in_ram(pa)) {
    panic("dec_refcnt: pa not in ram");
  }
  int index = (pa - KERNBASE) / PGSIZE;
  page.refcnt[index]--;
}

int get_refcnt(uint64 pa) {
  if (!pa_in_ram(pa)) {
    panic("get_refcnt: pa not in ram");
  }
  int index = (pa - KERNBASE) / PGSIZE;
  return page.refcnt[index];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  pageinit();
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

  acquire_page_lock();
  dec_refcnt((uint64)pa);
  if (get_refcnt((uint64)pa) > 0) {
    release_page_lock();
    return;
  }
  release_page_lock();

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) 
    page.refcnt[((uint64)r - KERNBASE) / PGSIZE] = 1;

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
