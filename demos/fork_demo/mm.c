#include "mm.h"

#define PAGEING_PAGES (63 * 1024 * 1024) / (4 * 1024)

unsigned char mem_map[PAGEING_PAGES];
#define RESERVE_PAGES 896

extern void assign_cr3_cr0(unsigned long);

void setup_paging() {
  unsigned long *pg_dir = PAGE_DIR;  // lan_os最多不能超过0x8000 64个扇区
  unsigned long *pg[PG_NUM];  //虚拟机必须给64M以上的物理内存
  pg[0] = (unsigned long)(pg_dir) + 0x1000;
  for (int i = 1; i < PG_NUM; ++i) {
    pg[i] = (unsigned long)pg[i - 1] + 0x1000;
  }
  for (int i = 0; i < PG_NUM; ++i) {
    pg_dir[i] = (unsigned long)(pg[i]) + 7;
  }
  for (int i = 0; i < 1024 * PG_NUM; ++i) {
    pg[0][i] = (i << 12) + 7;
  }
  assign_cr3_cr0(pg_dir);
  /*char *p = 0x3ffffff;
  p[0] = 1;*/
}

void init_mem_map() {
  for (int i = 0; i < PAGEING_PAGES; ++i) {
    mem_map[i] = 0;
  }
  for (int i = 0; i < RESERVE_PAGES; ++i) {
    mem_map[i] = 100;
  }
}

void mm_init() {
  init_mem_map();
  setup_paging();
}

unsigned long get_free_page() {
  for (int i = PAGEING_PAGES - 1; i >= 0; --i) {
    if (0 == mem_map[i]) {
      mem_map[i] = 1;
      return LOW_MEM + i * 4096;
    }
  }
  return 0;
}

int copy_page_tables(unsigned long from, unsigned long to, long size) {
  unsigned long *from_page_table;
  unsigned long *to_page_table;
  unsigned long this_page;
  unsigned long *from_dir, *to_dir;
  unsigned long new_page;
  unsigned long nr;

  if ((from & 0x3fffff) || (to & 0x3fffff)) {
    // todo panic
  }
  from_dir =
      (unsigned long
           *)(((from >> 20) & 0xffc) +
              PAGE_DIR); /* _pg_dir 和linux0.12定义不同，我们是从0x8000开始 */
  to_dir = (unsigned long *)(((to >> 20) & 0xffc) + PAGE_DIR);
  size = ((unsigned)(size + 0x3fffff)) >> 22;
  for (; size-- > 0; from_dir++, to_dir++) {
    from_page_table = (unsigned long *)(0xfffff000 & *from_dir);
    if (!(to_page_table = (unsigned long *)get_free_page())) {
      // todo panic
    }
    *to_dir = ((unsigned long)to_page_table) | 7;
    nr = 1024;
    for (; nr-- > 0; from_page_table++, to_page_table++) {
      this_page = *from_page_table;
      this_page &= ~2;
      *to_page_table = this_page;
      if (this_page > LOW_MEM) {
        *from_page_table = this_page;
        this_page -= LOW_MEM;
        this_page >>= 12;
        mem_map[this_page]++;
      }
    }
  }
  invalidate();
  return 0;
}