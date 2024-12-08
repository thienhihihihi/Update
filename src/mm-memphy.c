//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
    // Validate the offset
    if (offset < 0 || offset >= mp->maxsz) {
        printf("Error: Invalid address %d\n", offset);
        return -1;
        
    }

    // Directly set the cursor using the offset
    mp->cursor = offset;

    return 0;
}


/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, int addr, BYTE value) {
    if (mp == NULL) return -1;

    if (!mp->rdmflg) return -1; // Ensure sequential mode is enabled

    MEMPHY_mv_csr(mp, addr); // Move cursor to the address
    mp->storage[mp->cursor] = value; // Write to the cursor location

    return 0;
}


/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz) {
    int numfp = mp->maxsz / pagesz; // Number of frames
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0) {
        printf("MEMPHY_format: No frames to initialize.\n");
        return -1;
    }

    // Initialize the head of the free frame list
    fst = malloc(sizeof(struct framephy_struct));
    if (!fst) {
        printf("MEMPHY_format: Memory allocation failed.\n");
        return -1;
    }

    fst->fpn = iter;
    mp->free_fp_list = fst;
    printf("Initialized frame: %d\n", iter);

    // Add remaining frames to the list
    for (iter = 1; iter < numfp; iter++) {
        newfst = malloc(sizeof(struct framephy_struct));
        if (!newfst) {
            printf("MEMPHY_format: Memory allocation failed for frame %d.\n", iter);
            return -1;
        }
        newfst->fpn = iter;
        newfst->fp_next = NULL;
        fst->fp_next = newfst;
        fst = newfst;
      //  printf("Initialized frame: %d\n", iter);
    }

    return 0;
}


int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn) {
    struct framephy_struct *fp = mp->free_fp_list;

    if (fp == NULL) {
        printf("MEMPHY_get_freefp: No free frames available.\n");
        return -1;
    }

    *retfpn = fp->fpn;
    mp->free_fp_list = fp->fp_next;
  //  printf("trang trong swpram la : %d\n",*retfpn);

    free(fp);
    return 0;
}


int MEMPHY_dump(struct memphy_struct *mp)
{
    if (!mp || !mp->storage) {
        printf("Error: MEMPHY is not initialized.\n");
        return -1;
    }

//     printf("MEMPHY content dump:\n");
//     for (int i = 0; i < mp->maxsz; i++) {
//         //if (i % 16 == 0) // Print 16 bytes per line for better readability
//             //printf("\n%04x: ", i);
//        // printf("%02x ", mp->storage[i]);
//     }
//    // printf("\n");

printf("===== PHYSICAL MEMORY DUMP =====\n");
   for (int i = 0; i < mp->maxsz; ++i)
   {
      if (mp->storage[i] != 0)
      {
         printf("BYTE %08x: %d\n", i, mp->storage[i]);
      }
   }

   printf("===== PHYSICAL MEMORY END-DUMP =====\n");
   printf("================================================================\n");
   //!
    return 0;



   // return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
    if (newnode == NULL) {
        printf("MEMPHY_put_freefp: Memory allocation failed.\n");
        return -1;
    }
   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;

   return 0;
}


/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg) {
    mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
    if (!mp->storage) {
        printf("init_memphy: Memory allocation for storage failed.\n");
        return -1;
    }

    mp->maxsz = max_size;

    if (MEMPHY_format(mp, PAGING_PAGESZ) != 0) {
        printf("init_memphy: MEMPHY_format failed.\n");
        return -1;
    }

    mp->rdmflg = (randomflg != 0) ? 1 : 0;

    if (!mp->rdmflg) // Sequential access device
        mp->cursor = 0;

    printf("init_memphy: Initialized successfully with %d bytes.\n", max_size);
    return 0;
}


//#endif
