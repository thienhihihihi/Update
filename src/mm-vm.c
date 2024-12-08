//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h> //thêm thư viện đồng bộ hóa

static pthread_mutex_t mmvm_lock;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *  
 */

int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt) {
    struct vm_rg_struct **rg_node = &mm->mmap->vm_freerg_list;

    if (rg_elmt.rg_start >= rg_elmt.rg_end) {
      
        return -1; // Invalid range
    }



    // Traverse the free list to insert or merge the region
    while (*rg_node) {
        struct vm_rg_struct *current = *rg_node;

        // Merge if the new region is adjacent to or overlaps with the current region
        if (current->rg_end >= rg_elmt.rg_start && current->rg_start <= rg_elmt.rg_end) {
            // Extend the current region to include the new region
            current->rg_start = (current->rg_start < rg_elmt.rg_start) ? current->rg_start : rg_elmt.rg_start;
            current->rg_end = (current->rg_end > rg_elmt.rg_end) ? current->rg_end : rg_elmt.rg_end;

            // Check if subsequent regions can also be merged
            struct vm_rg_struct *next = current->rg_next;
            while (next && current->rg_end >= next->rg_start) {
                current->rg_end = (current->rg_end > next->rg_end) ? current->rg_end : next->rg_end;
                current->rg_next = next->rg_next;
                free(next);
                next = current->rg_next;
            }
            return 0;
        }

        // Insert the new region in sorted order
        if (rg_elmt.rg_end < current->rg_start) {
            struct vm_rg_struct *new_node = malloc(sizeof(struct vm_rg_struct));
            if (!new_node) return -1; // Allocation failed
            *new_node = rg_elmt;
            new_node->rg_next = current;
            *rg_node = new_node;
            return 0;
        }

        rg_node = &current->rg_next;
    }

    // Append the new region to the end of the list
    struct vm_rg_struct *new_node = malloc(sizeof(struct vm_rg_struct));
    if (!new_node) return -1; // Allocation failed
    *new_node = rg_elmt;
    new_node->rg_next = NULL;
    *rg_node = new_node;
 //print_free_list(mm->mmap->vm_freerg_list);
    return 0;
}


/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
/* __alloc - allocate a region memory */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr) {

  //   printf("vmaid cua tien trinh nay la: %d\n",vmaid);
  //   struct vm_rg_struct* rrr=caller->mm->mmap->vm_freerg_list;
  //  while(rrr!=NULL) {
  //   printf("free_mem la start:%d end: %d\n",rrr->rg_start,rrr->rg_end);
  //   rrr=rrr->rg_next;
  //  }

  pthread_mutex_lock(&mmvm_lock);//? thêm mutex_lock

     struct vm_rg_struct rgnode;

      printf("__alloc: Allocating %d bytes in region ID %d, vmaid=%d\n", size, rgid, vmaid);


if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  { 
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;
    printf("Đã cấp phát cho rgid = %d từ [%d,%d]\n",rgid,rgnode.rg_start,rgnode.rg_end);
    pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

   
    return 0;
  }



   

 if(vmaid==0) {
struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
struct vm_area_struct *remain_rg = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);

int old_sbrk=cur_vma->sbrk;

int a;
inc_vma_limit(caller,vmaid,inc_sz,&a);
caller->mm->symrgtbl[rgid].rg_start=old_sbrk;
caller->mm->symrgtbl[rgid].rg_end=old_sbrk+size;
//printf("Da cap phat vung start: %d toi end: %d voi rgid la: %d\n ",caller->mm->symrgtbl[rgid].rg_start,remain_rg->sbrk,rgid);

*alloc_addr=old_sbrk;



if (old_sbrk + size < remain_rg->sbrk)
  {
    struct vm_rg_struct *rg_free = malloc(sizeof(struct vm_rg_struct));
    rg_free->rg_start = old_sbrk + size;
    rg_free->rg_end = remain_rg->sbrk;
 //   printf("Rg_free tu start: %d toi end: %d\n",rg_free->rg_start,rg_free->rg_end);
    enlist_vm_freerg_list(caller->mm, *rg_free);
  }


      pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

  return 0;


  











//      if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0){

//       cur_vma->sbrk+=size;

//       caller->mm->symrgtbl[rgid].rg_start=rgnode.rg_start;



//       caller->mm->symrgtbl[rgid].rg_end=rgnode.rg_end;
//       caller->mm->symrgtbl[rgid].vmaid=vmaid;

//       *alloc_addr=rgnode.rg_start;

//        printf("__alloc: Successfully allocated region [%d, %d] in vmaid=%d\n",

//                rgnode.rg_start, rgnode.rg_end, vmaid);

  

//         return 0; // Success

// }
 }
else if (vmaid==1){
  ;
struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
struct vm_area_struct *remain_rg = get_vma_by_num(caller->mm, vmaid);

  int inc_sz = PAGING_PAGE_ALIGNSZ(size);

int old_sbrk=cur_vma->sbrk;
int a;
inc_vma_limit(caller,vmaid,inc_sz,&a);
//printf("old_sbrk la : %d\n",old_sbrk);
caller->mm->symrgtbl[rgid].rg_start=old_sbrk-size;
caller->mm->symrgtbl[rgid].rg_end=old_sbrk;

//printf("Da cap phat vung start: %d toi end: %d voi rgid la:  %d\n",old_sbrk,remain_rg->sbrk,rgid);


*alloc_addr=caller->mm->symrgtbl[rgid].rg_start;
if (old_sbrk - size > remain_rg->sbrk)
  {
    struct vm_rg_struct *rg_free = malloc(sizeof(struct vm_rg_struct));
    rg_free->rg_end = old_sbrk - size;
    rg_free->rg_start = remain_rg->sbrk;
  //  printf("Rg_free tu start: %d toi end: %d\n",rg_free->rg_end,rg_free->rg_start);
    enlist_vm_freerg_list(caller->mm, *rg_free);
  }


      pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

  return 0;


}
}


/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
/* __free - remove a region memory */
int __free(struct pcb_t *caller, int rgid) {
    pthread_mutex_lock(&mmvm_lock); //? thêm mutex_lock

    if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
          pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

        return -1; // Invalid region ID
    }

    struct vm_rg_struct *symrg = get_symrg_byid(caller->mm, rgid);
 // printf("free data voi rgid: %d la start:%d va end la:%d ",rgid,symrg->rg_start,symrg->rg_end);
    if (!symrg) {
          pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

        return -1; // Symbol region not found
    }

    printf("Freeing region: Start=%d, End=%d\n", symrg->rg_start, symrg->rg_end);




    // Add the region back to the free list
    if (enlist_vm_freerg_list(caller->mm, *symrg) != 0) {
          pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

        return -1; // Failed to enlist region
    }






    // Clear the symbol region table entry
    symrg->rg_start = symrg->rg_end = -1;
    symrg->vmaid = -1;
    

    // Print updated free region list
    struct vm_rg_struct *list = caller->mm->mmap->vm_freerg_list;
    printf("Updated free region list:\n");
    while (list) {
        printf("  Start=%d, End=%d\n", list->rg_start, list->rg_end);
        list = list->rg_next;
    }
    pthread_mutex_unlock(&mmvm_lock); //? thêm mutex_unlock

    return 0;
}



/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index) {
    int addr;
    printf("pgalloc: Attempting to allocate %u bytes for region %u\n", size, reg_index);

    // By default using vmaid = 0
    int result = __alloc(proc, 0, reg_index, size, &addr);

    if (result == 0) {
        printf("pgalloc: Successfully allocated at address %d\n", addr);
    } else {
        printf("pgalloc: Allocation failed. Free region list:\n");

        struct vm_area_struct *vma = proc->mm->mmap;
        struct vm_rg_struct *list = vma->vm_freerg_list;

        while (list) {
            printf("  Start=%d, End=%d\n", list->rg_start, list->rg_end);
            list = list->rg_next;
        }
    }

    return result;
}


/*pgmalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify vaiable in symbole table)
 */
int pgmalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 1 */
  return __alloc(proc, 1, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
   return __free(proc, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];
 
  if (!PAGING_PTE_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn; 
    int vicfpn;
    uint32_t vicpte;

     int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable

    /* TODO: Play with your paging theory here */
    /* Find victim page */
    //find_victim_page(caller->mm, &vicpgn);
    if(find_victim_page(caller->mm, &vicpgn) == -1) return -1;

    /* Get free frame in MEMSWP */
    //MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
    if(MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1) return -1;

    vicpte = mm->pgd[vicpgn];
    vicfpn = PAGING_FPN(vicpte);

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

    /* Update page table */
    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);

    /* Update its online status of the target page */
    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    //pte_set_fpn() & mm->pgd[pgn];
    pte_set_fpn(&pte, tgtfpn); //!nếu lỗi xem lại chỗ này(bỏ)



    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }

  *fpn = PAGING_PTE_FPN(pte);

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;
  

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */


  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;


  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  MEMPHY_write(caller->mram,phyaddr, value);

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int rgid, int offset, BYTE *data)
{ 
  
  
   pthread_mutex_lock(&mmvm_lock); //?thêm mutex_lock

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  

  if(currg == NULL ||(currg->rg_start==0 && currg->rg_end==0) ){ /* Invalid memory identify */
    printf("Vùng Rgid %d không hợp lệ\n",rgid);

      pthread_mutex_unlock(&mmvm_lock); //?thêm mutex_unlock

	  return -1;
  }
 
  pg_getval(caller->mm, currg->rg_start + offset, data, caller);
      pthread_mutex_unlock(&mmvm_lock); 
      //?thêm mutex_unlock


  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;
  int val = __read(proc, source, offset, &data);

  destination = (uint32_t) data;
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int rgid, int offset, BYTE value)
{

    pthread_mutex_lock(&mmvm_lock); //?thêm mutex_lock

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  
  if(currg == NULL || (currg->rg_start==0 && currg->rg_end==0)){
    printf("Vùng Rgid %d không hợp lệ\n",rgid);
      pthread_mutex_unlock(&mmvm_lock); //?thêm mutex_unlock

    return -1;
  } /* Invalid memory identify */
	 

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  pthread_mutex_unlock(&mmvm_lock); //?thêm mutex_unlock

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return __write(proc, destination, offset, data);
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
    pthread_mutex_lock(&mmvm_lock); //?thêm mutex_lock

  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PTE_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }
  pthread_mutex_unlock(&mmvm_lock); //?thêm mutex_unlock

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
if (vmaid==0) {
newrg->rg_start=cur_vma->sbrk;
newrg->rg_end=newrg->rg_start+size;
return newrg;


}
if(vmaid==1) {
newrg->rg_start=cur_vma->sbrk-size;
newrg->rg_end=cur_vma->sbrk;
return newrg;





}
  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */

 
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
/* validate_overlap_vm_area - check for overlaps */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend) {
    struct mm_struct *mm = caller->mm;

  //  printf("validate_overlap_vm_area: Checking proposed region [%d, %d] (vmaid=%d)\n", vmastart, vmaend, vmaid);

    for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
        struct vm_rg_struct *rg = &mm->symrgtbl[i];

        // Skip unallocated regions
        if (rg->rg_start == -1 || rg->rg_end == -1) {
            continue;
        }

        // Skip regions with a different vmaid
        if (rg->vmaid != vmaid) {
            continue;
        }

      

        // Skip the region being validated
        if (rg->rg_start == vmastart && rg->rg_end == vmaend) {
          
            continue;
        }

      

        if (OVERLAP(rg->rg_start, rg->rg_end, vmastart, vmaend)) {
          
            return -1; // Overlap detected
        }
    }

   // printf("  No overlap detected\n");
    return 0;
}



/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *@inc_limit_ret: increment limit return
 *
 */
/* inc_vma_limit - increase VM area limits */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz, int* inc_limit_ret) {
    if (vmaid==0) {
 struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 int old_end = cur_vma->vm_end;
if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0){return -1; /*Overlap and failed allocation */}
    cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz; 
//printf("cur_end hien tai la :%d\n",cur_vma->vm_end);
*inc_limit_ret=cur_vma->sbrk;

//printf("area_rg la start:%d end:%d va incnumpage la:%d\n",area->rg_start,area->rg_end,incnumpage );

 if (vm_map_ram(caller, area->rg_start, area->rg_end,  old_end, incnumpage , newrg) < 0){
                      return -1;
                    }
   
return 0;



    }


    else if (vmaid==1) {
struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   int old_end = cur_vma->vm_end;
if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0){return -1; /*Overlap and failed allocation */}
cur_vma->vm_end-=inc_sz;
cur_vma->sbrk-=inc_sz;
//printf("area_rg la start:%d end:%d va incnumpage la:%d va old_end la :%d\n",area->rg_start,area->rg_end,incnumpage,old_end );

if (vm_map_ram(caller, area->rg_start, area->rg_end, cur_vma->sbrk , incnumpage , newrg) < 0){
                      return -1;
                    }
return 0;

    }
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
/* find_victim_page - find victim page */
int find_victim_page(struct mm_struct *mm, int *retpgn) {
    struct pgn_t *pg = mm->fifo_pgn;
    if (pg == NULL) {
      
        return -1; // No pages to victimize
    }
 struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  *retpgn = pg->pgn;
  prev->pg_next = NULL;


  
    return 0;
}

int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = caller->mm->mmap->vm_freerg_list;
  if (rgit == NULL)
    return -1;
if (rgit->rg_start==rgit->rg_end) {
  if(rgit->rg_next!=NULL) {
    rgit=rgit->rg_next;
  }
}

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL && rgit->vmaid == vmaid)
  {
    
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
    
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        { /*End of free list */
          rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      return 0;
    }
    else
    {
      rgit = rgit->rg_next;	// Traverse next rg
    }
  }

 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */


//#endif
