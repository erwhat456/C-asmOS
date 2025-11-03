/* kernel.c */
#include <stdint.h>

/* forward from drivers.c/apps.c */
void screen_clear(void);
void screen_print(const char*);
void screen_println(const char*);
void screen_putch(char);
int kb_getascii_poll(void);
uint16_t get_e820_count(void);
typedef struct { uint64_t base; uint64_t len; uint32_t type; uint32_t acpi; } e820_entry_t;
e820_entry_t* get_e820_table(void);
void itoa_dec(uint32_t v, char *out);
uint32_t bios_ticks(void);

/* apps tasks */
void task_terminal(void);
void task_counter(void);

/* simple task registration */
#define MAX_TASKS 8
typedef void (*task_fn)(void);
static task_fn tasks[MAX_TASKS];
static int tcount=0;
void register_task(task_fn f){ if(tcount<MAX_TASKS) tasks[tcount++]=f; }
void scheduler_run(void){
  while(1){
    for(int i=0;i<tcount;i++) tasks[i]();
    for(volatile uint32_t d=0; d<50000; ++d) __asm__ volatile("");
  }
}

/* CPUID helper */
void cpuid(uint32_t eax_in, uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d){
  __asm__ volatile("cpuid" : "=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d) : "a"(eax_in));
}

/* compute total usable RAM from E820 (type==1 usable) */
uint64_t compute_usable_ram_bytes(void){
  uint16_t cnt = get_e820_count();
  e820_entry_t *t = get_e820_table();
  uint64_t sum = 0;
  for(int i=0;i<cnt;i++){
    if(t[i].type == 1) sum += t[i].len;
  }
  return sum;
}

/* print hex64 */
void print_hex64(uint64_t v){
  const char hex[]="0123456789ABCDEF";
  char s[17]; for(int i=0;i<16;i++){ s[15-i]=hex[v & 0xF]; v >>=4; } s[16]=0;
  screen_print(s);
}

void kmain(void){
  screen_clear();
  screen_println("CinnaOS kernel: real hardware probe");
  /* E820 memory map */
  uint16_t cnt = get_e820_count();
  char tmp[32];
  screen_print("E820 entries: "); itoa_dec(cnt,tmp); screen_println(tmp);
  e820_entry_t *t = get_e820_table();
  for(int i=0;i<cnt;i++){
    screen_print("Entry "); itoa_dec(i,tmp); screen_print(" base=0x"); print_hex64(t[i].base); screen_print(" len=0x"); print_hex64(t[i].len);
    screen_print(" type="); itoa_dec(t[i].type,tmp); screen_println(tmp);
  }
  uint64_t ram = compute_usable_ram_bytes();
  screen_print("Usable RAM bytes: 0x"); print_hex64(ram); screen_println("");
  /* also print in MB */
  uint32_t mb = (uint32_t)(ram / (1024ULL*1024ULL));
  screen_print("Usable RAM (MB): "); itoa_dec(mb,tmp); screen_println(tmp);

  /* CPUID vendor */
  uint32_t a,b,c,d;
  cpuid(0,&a,&b,&c,&d);
  char vendor[13];
  *(uint32_t*)&vendor[0] = b; *(uint32_t*)&vendor[4] = d; *(uint32_t*)&vendor[8] = c; vendor[12]=0;
  screen_print("CPU vendor: "); screen_println(vendor);
  cpuid(1,&a,&b,&c,&d);
  uint8_t family = ((a>>8)&0xF) + ((a>>20)&0xFF);
  screen_print("CPU family/model: ");
  itoa_dec(family,tmp); screen_println(tmp);

  /* register tasks and run scheduler (cooperative) */
  register_task(task_terminal);
  register_task(task_counter);
  screen_println("Starting tasks (cooperative)...");
  scheduler_run();

  while(1) __asm__ volatile("hlt");
}
