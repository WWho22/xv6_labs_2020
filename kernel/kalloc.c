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

struct ref_count
{
  struct spinlock lock;
  int COW_INDEX[32768]; // Array to keep track of reference counts for COW pages
}ref_count;


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  initlock(&ref_count.lock, "ref_count");
  memset(ref_count.COW_INDEX, 0, sizeof(ref_count.COW_INDEX));
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

  acquire(&ref_count.lock);
  
  if (ref_count.COW_INDEX[((uint64)pa - KERNBASE) / PGSIZE] > 1)
  {
    ref_count.COW_INDEX[((uint64)pa - KERNBASE) / PGSIZE]--;
    release(&ref_count.lock);
    return;
  }

    //只有当引用计数为0时才真正释放该物理内存页
    ref_count.COW_INDEX[((uint64)pa - KERNBASE) / PGSIZE] = 0;
    release(&ref_count.lock);
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

  acquire(&ref_count.lock);
  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    ref_count.COW_INDEX[((uint64)r - KERNBASE) / PGSIZE] = 1;
  }
  release(&ref_count.lock);
  // printf("kalloc %p\n", r);
  return (void*)r;
}

void increase_ref_count(uint64 pa)
{
  uint64 pa_deal = PGROUNDDOWN(pa);
  acquire(&ref_count.lock);
  ref_count.COW_INDEX[(pa_deal - KERNBASE) / PGSIZE]++;
  release(&ref_count.lock);
}

void decrease_ref_count(uint64 pa)
{
  uint64 pa_deal = PGROUNDDOWN(pa);
  acquire(&ref_count.lock);
  ref_count.COW_INDEX[(pa_deal - KERNBASE) / PGSIZE]--;
  release(&ref_count.lock);
}

int it_is_cow_page(pagetable_t pagetable,uint64 va)
{
  pte_t* pte;

  if (va >= MAXVA)
  {
    return -1;
  }
  //获取该虚拟地址对应的叶子页表项地址
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  
  // //获取原来页表页的权限  
  // int flags = PTE_FLAGS(*pte);

  if ((*pte & PTE_COW) != 0)
  {
    //是COW_PAGE
    return 0;
  }
  else
  {
    //不是COW_PAGE
    return -1;
  }

}

void* cowpage_alloc(pagetable_t pagetable,uint64 va)
{
  char* mem;
  int flags ;
  pte_t *pte;
  uint64 pa;

  pa = walkaddr(pagetable, va);
  if (pa == 0)
  {
    // printf("usertrap(): walkaddr failed\n");
    return 0;
  }

  pte = walk(pagetable, va, 0);
  if (pte == 0)
  {
    return 0;
  }
  flags = PTE_FLAGS(*pte);
  acquire(&ref_count.lock);
  if (ref_count.COW_INDEX[(pa - KERNBASE) / PGSIZE] > 1)
  {
    release(&ref_count.lock);
    //如果引用计数大于1，则需要分配一个新的物理内存页
    if ((mem = kalloc()) == 0)
    {
      return 0;
    }
    else 
    {
      //把物理内存内容复制到新分配的内存中
      memmove(mem, (char*)pa, PGSIZE);
      //取消虚拟地址和物理地址的映射
      uvmunmap(pagetable, PGROUNDDOWN(va), 1, 1);
      //去掉写时复制标记
      flags &= ~PTE_COW; 
      //把新分配的物理内存映射到该虚拟地址
      if (mappages(pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)mem, flags | PTE_W) < 0)
      {
        panic("cowpage_alloc: mappages failed");
        printf("cowpage_alloc(): mappages failed");
        kfree(mem);
        return 0; 
      }
      return (void*)mem;
    }
  }
  else
  {
    //如果引用计数等于1，则直接修改权限为可写
    // *pte |= PTE_W; //修改权限为可写
    // *pte &= ~PTE_COW; //去掉写时复制标记
    if (ref_count.COW_INDEX[(pa - KERNBASE) / PGSIZE] == 1)
    {
      if ((*pte & PTE_COW) != 0)
      {
        //修改权限为可写
        *pte |= PTE_W;
        //并且还要去掉写时复制的标记
        *pte &= ~PTE_COW;
      }
    }
    release(&ref_count.lock);
    return (void*)pa;
  }
  
}

