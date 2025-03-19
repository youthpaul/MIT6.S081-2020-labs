// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

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
// the number of physical pages
#define PGNUM (PHYSTOP - KERNBASE) / PGSIZE  

  uint ref[PGNUM]; // the reference of each page
  struct spinlock lock;
}pgref;

// set the reference of pa number to 1
// void setpgref(uint64 pa){
  // int id = PA2PGID(pa);
  // acquire(&pgref.lock);
  // pgref.ref[id] = 1;
  // release(&pgref.lock);
// }

// increase the reference number of pa
void incpgref(uint64 pa){
  int id = PA2PGID(pa);
  acquire(&pgref.lock);
  ++pgref.ref[id];
  release(&pgref.lock);
}

// decrease the reference number of pa
void decpgref(uint64 pa){
  int id = PA2PGID(pa);
  acquire(&pgref.lock);
  --pgref.ref[id];
  release(&pgref.lock);
}

// get the reference number of pa
int getpgref(uint64 pa){
  int id = PA2PGID(pa);
  acquire(&pgref.lock);
  int ret = pgref.ref[id];
  release(&pgref.lock);
  return ret;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pgref.lock, "pgref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    pgref.ref[PA2PGID(p)] = 1;
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
  
  // decpgref((uint64)pa); // decrease the ref
  // if(getpgref((uint64)pa) > 0) return; // other pgtbl still point to it
  // if(getpgref((uint64)pa) < 0)
  //   panic("kfree ref negative");
  acquire(&pgref.lock);
  int cnt = --pgref.ref[PA2PGID(pa)];
  if(cnt > 0){
    release(&pgref.lock);
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);

  release(&pgref.lock);
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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    if(getpgref((uint64)r) != 0){
      printf("\n---> %d\n", getpgref((uint64)r));
      panic("kalloc ref");
    }
    incpgref((uint64)r);
  }
  return (void*)r;
}

// alloc a new pyhsical page mappded into va
// return 0 if succeed, -1 otherwise
int cowalloc(uint64 va){
  va = PGROUNDDOWN(va);
  struct proc* p = myproc();
  if(va >= p -> sz) return -1;

  pte_t* pte = walk(p -> pagetable, va, 0);
  if(pte == 0) return -1;
  uint64 flag = PTE_FLAGS(*pte);
  if((flag & PTE_COW) == 0 || (flag & PTE_U) == 0 || (flag & PTE_V) == 0) return -1;
  
  uint64 pa = PTE2PA(*pte);
  uint64 npa = (uint64)kalloc();
  if(npa == 0) return -1;
  memmove((void*)npa, (char*)pa, PGSIZE);

  flag = (flag & ~PTE_COW) | PTE_W;
  uvmunmap(p -> pagetable, va, 1, 1); // decrease the ref in kfree()
  if(mappages(p -> pagetable, va, PGSIZE, npa, flag) < 0){
    kfree((void*)npa);
    return -1;
  }

  // if the ref only leave one, then it's not COW page
  // if(getpgref(pa) == 1) *pte = (*pte & ~PTE_COW) | PTE_W;
  // don't need, the last page will be free, and copy its content to a new page

  return 0;
}
