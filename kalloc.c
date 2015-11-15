// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "qemu-queue.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

typedef QTAILQ_HEAD(run_list, run) list_head;
typedef QTAILQ_ENTRY(run) list_entry;

struct run {
  uint flags;
  uint size; // size in page
  //struct run *next;
  list_entry next;
  list_entry pra_link;
};

struct {
  struct spinlock lock;
  int use_lock;
  uint nfreeblock;
  //struct run *freelist;
  list_head freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  QTAILQ_INIT(&kmem.freelist);
  kmem.use_lock = 0;
  kmem.nfreeblock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
//  char *p;
//  p = (char*)PGROUNDUP((uint)vstart);
//  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
//    kfree(p);
  char *s = (char*)PGROUNDUP((uint)vstart);
  char *e = (char*)PGROUNDUP((uint)vend);
  struct run *r = (struct run*)s;
  r->size = ((uint)e-(uint)s) / PGSIZE;
  QTAILQ_INSERT_HEAD(&kmem.freelist, r, next);
  kmem.nfreeblock++;
}

void print_mem()
{
  struct run *r = QTAILQ_FIRST(&kmem.freelist);
  int count = 0;
  while (count < kmem.nfreeblock) {
    cprintf("%x\t\t\t%d\n", r, r->size);
    count ++;
    r = QTAILQ_NEXT(r, next);
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
  //struct run *r;
  //cprintf("kfree %x\n", v);
  //print_mem();
  if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  // memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  //r = (struct run*)v;
  //r->next = kmem.freelist;
  //kmem.freelist = r;
  int n = 1;
  struct run *p = (struct run*)v; // page(s) to free
  struct run *r = QTAILQ_FIRST(&kmem.freelist);
  p->size = n;
  int count = 0;
  int merged = 0;
  while (count < kmem.nfreeblock)
  {
    if ((char *)r == v + p->size * PGSIZE)
    {
      p->size += r->size;
      QTAILQ_INSERT_BEFORE(r, p, next);
      QTAILQ_REMOVE(&kmem.freelist, r, next);
      merged = 1;
      break;
    }
    else if ((char *)r + r->size * PGSIZE == v)
    {
      r->size += p->size;
      merged = 1;
      break;
    }
    count ++;
    r = QTAILQ_NEXT(r, next);
  }
  if (merged == 0)
  {
    QTAILQ_INSERT_HEAD(&kmem.freelist, p, next);
    kmem.nfreeblock ++;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  //struct run *r;
  //print_mem();
  if(kmem.use_lock)
    acquire(&kmem.lock);
  int n = 1; // n pages to alloc
  struct run *r = QTAILQ_FIRST(&kmem.freelist);
  int count = 0;
  while (count < kmem.nfreeblock) {
    if (r->size >= n) break;
    count ++;
    r = QTAILQ_NEXT(r, next);
  }
  if (count < kmem.nfreeblock){
    QTAILQ_REMOVE(&kmem.freelist, r, next);
    struct run* remain = (struct run*)((char *)r + n * PGSIZE);
    remain->size = r->size - n;
    r->size = n;
    if (remain->size > 0)
      QTAILQ_INSERT_HEAD(&kmem.freelist, remain, next);
    else
      kmem.nfreeblock--;
    if(kmem.use_lock)
    release(&kmem.lock);
    //cprintf("kalloc %x\n", r);
    return (char*)r;
  }
  else {
    //cprintf("cannot allocate\n");
    if(kmem.use_lock)
    release(&kmem.lock);
    return NULL;
  }

//  r = kmem.freelist;
//  if(r)
//    kmem.freelist = r->next;

  // will not execute anyway
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

