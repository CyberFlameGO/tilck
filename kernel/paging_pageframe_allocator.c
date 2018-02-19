
#include <common_defs.h>
#include <paging.h>
#include <string_util.h>
#include <utils.h>

/*
 * By mapping 4096 KB (one page) in 1 bit, a single 32-bit integer maps 128 KB.
 * Mapping 1 MB requires 8 integers.
 */
#define FULL_128KB_AREA (0xFFFFFFFFU)


static u32 bitfield[MB_RESERVED_FOR_PAGING * 8];

void init_paging_pageframe_allocator(void)
{
#ifdef KERNEL_TEST
   bzero((void *)bitfield, sizeof(bitfield));
#endif
}


/*
 * Paging needs its custom page frame allocator for (kernel) page tables.
 */

uptr paging_alloc_pageframe()
{
   u32 idx = 0;
   bool found = false;

   for (u32 i = 0; i < ARRAY_SIZE(bitfield); i++) {

      if (bitfield[idx] != FULL_128KB_AREA) {
         found = true;
         break;
      }

      idx = (idx + 1) % ARRAY_SIZE(bitfield);
   }

#ifndef KERNEL_TEST
   VERIFY(found);
#else
   if (!found)
      return 0;
#endif

   u32 free_index = get_first_zero_bit_index(bitfield[idx]);
   bitfield[idx] |= (1 << free_index);

   uptr ret = (idx << 17) + (free_index << PAGE_SHIFT);
   return ret + (INITIAL_MB_RESERVED << 20);
}


void paging_free_pageframe(uptr address) {

   uptr naddr = (address & PAGE_MASK) - (INITIAL_MB_RESERVED << 20);
   u32 bitIndex = (naddr >> PAGE_SHIFT) & 31;
   u32 majorIndex = (naddr & 0xFFFE0000U) >> 17;

   // Asserts that the page was allocated.
   ASSERT(bitfield[majorIndex] & (1 << bitIndex));

   bitfield[majorIndex] &= ~(1 << bitIndex);
}
