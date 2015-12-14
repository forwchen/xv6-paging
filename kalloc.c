// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "queue.h"

#define SLABSIZE 32

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

typedef Q_HEAD(run_list, run) q_head;
typedef Q_ENTRY(run) q_entry;

struct run {
  uint size; // size in bytes
  q_entry link;
};

struct {
  struct spinlock lock;
  int use_lock;
  uint nfreeblock;
  q_head freelist;
} kmem;

struct {
  struct spinlock lock;
  int use_lock;
  uint nfreeblock;
  q_head freelist;
} slab;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  initlock(&slab.lock, "slab");
  Q_INIT(&kmem.freelist);
  kmem.use_lock = 0;
  kmem.nfreeblock = 0;
  Q_INIT(&slab.freelist);
  slab.use_lock = 0;
  slab.nfreeblock = 0;
  freerange(vstart, vend);
}

void
slabinit()
{
  char *p = kalloc();
  char *start = p;
  for (; p + SLABSIZE < start + PGSIZE; p += SLABSIZE)
    free_slab(p);
}

void
kinit2(void *vstart, void *vend)
{
  //freerange(vstart, vend);
  kmem.use_lock = 1;
  slab.use_lock = 1;
  slabinit();
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}

void print_mem()
{
  struct run *r = Q_FIRST(&kmem.freelist);
  int count = 0;
  while (count < kmem.nfreeblock) {
    cprintf("%x\t\t\t%d\n", r, r->size);
    count ++;
    r = Q_NEXT(r, link);
  }
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  // memset(v, 1, PGSIZE);
//  cprintf("free %x\n", v2p(v));
  if(kmem.use_lock)
    acquire(&kmem.lock);
  struct run *p = (struct run*)v; // page to free
  p->size = PGSIZE;
  Q_INSERT_HEAD(&kmem.freelist, p, link);
  kmem.nfreeblock ++;

  if(kmem.use_lock)
    release(&kmem.lock);
}

void
free_slab(char *v)
{
  if (slab.use_lock)
      acquire(&slab.lock);
  struct run *p = (struct run*)v;
  p->size = SLABSIZE;
  Q_INSERT_HEAD(&slab.freelist, p, link);
  slab.nfreeblock ++;

  if (slab.use_lock)
      release(&slab.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  if (kmem.nfreeblock == 0)
      page_out();
  if(kmem.use_lock)
    acquire(&kmem.lock);

  struct run *r = Q_FIRST(&kmem.freelist);
  Q_REMOVE(&kmem.freelist, r, link);
  kmem.nfreeblock--;
//  cprintf("alloc %x\n", v2p(r));
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

char*
alloc_slab(void)
{
  if (slab.nfreeblock == 0) slabinit();
  if (slab.use_lock)
    acquire(&slab.lock);
  struct run *r = Q_FIRST(&slab.freelist);
  Q_REMOVE(&slab.freelist, r, link);
  slab.nfreeblock--;

  if (slab.use_lock)
    release(&slab.lock);
  return (char*)r;
}
