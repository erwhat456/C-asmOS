/* drivers.c */
#include <stdint.h>

/* VGA text */
volatile uint16_t *VGA = (volatile uint16_t*)0xB8000;
static uint32_t vpos = 0;
static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v; }

void screen_clear(void){
  for(int i=0;i<80*25;i++) VGA[i]=(uint16_t)(' ' | (0x07<<8));
  vpos=0;
}
void screen_putch(char c){
  if(c=='\n'){ uint32_t r=vpos/80; vpos=(r+1)*80; return; }
  VGA[vpos++]=(uint16_t)c | (0x07<<8);
  if(vpos>=80*25) vpos=0;
}
void screen_print(const char *s){ while(*s) screen_putch(*s++); }
void screen_println(const char *s){ screen_print(s); screen_putch('\n'); }

/* keyboard polled ASCII */
int kb_getascii_poll(void){
  uint8_t st=inb(0x64);
  if(!(st & 1)) return -1;
  uint8_t sc=inb(0x60);
  static const char map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z',
    'x','c','v','b','n','m',',','.','/',0,0,0,0,' '
  };
  if(sc < 128 && map[sc]) return map[sc];
  return -1;
}

/* ATA PIO single-sector read (LBA28) */
int ata_read_sector_lba28(uint32_t lba, void *buffer){
  if(lba > 0x0FFFFFFF) return -1;
  outb(0x1F6, 0xE0 | ((lba>>24)&0x0F));
  outb(0x1F2, 1);
  outb(0x1F3, (uint8_t)(lba & 0xFF));
  outb(0x1F4, (uint8_t)((lba>>8)&0xFF));
  outb(0x1F5, (uint8_t)((lba>>16)&0xFF));
  outb(0x1F7, 0x20);
  for(int i=0;i<100000;i++){
    uint8_t s=inb(0x1F7);
    if(!(s & 0x80) && (s & 0x08)){
      uint16_t *p = (uint16_t*)buffer;
      for(int w=0; w<256; ++w){ uint16_t val; __asm__ volatile("inw %1,%0":"=a"(val):"Nd"(0x1F0)); p[w]=val; }
      return 0;
    }
  }
  return -1;
}

/* BIOS tick counter at 0x046C (low 32-bit) */
uint32_t bios_ticks(void){ volatile uint32_t *p = (volatile uint32_t*)0x046C; return *p; }

/* E820 memory map access: pointer to entries written by bootloader at 0x7000
   each entry is 24 bytes: base(64), length(64), type(32), acpi(32)
   count stored at 0x6FF0 (word)
*/
typedef struct {
  uint64_t base;
  uint64_t len;
  uint32_t type;
  uint32_t acpi;
} e820_entry_t;

uint16_t get_e820_count(void){
  return *(volatile uint16_t*)0x6FF0;
}
e820_entry_t* get_e820_table(void){
  return (e820_entry_t*)0x7000;
}

/* tiny uint->decimal */
void itoa_dec(uint32_t v, char *out){
  char buf[12]; int i=0;
  if(v==0){ out[0]='0'; out[1]=0; return; }
  while(v){ buf[i++]= '0' + (v%10); v/=10; }
  int j=0; while(i--) out[j++]=buf[i]; out[j]=0;
}
