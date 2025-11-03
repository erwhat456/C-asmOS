/* apps.c */
#include <stdint.h>
void screen_print(const char*);
void screen_putch(char);
int kb_getascii_poll(void);
uint32_t bios_ticks(void);
void itoa_dec(uint32_t v, char *out);

void task_terminal(void){
  int c = kb_getascii_poll();
  if(c > 0){
    if(c=='\r') screen_putch('\n'); else screen_putch((char)c);
  }
}

/* prints BIOS tick counter every tick change */
void task_counter(void){
  static uint32_t last = 0;
  uint32_t t = bios_ticks();
  if(t != last){
    last = t;
    char s[16]; itoa_dec(t,s);
    screen_print("[TICKS] "); screen_println(s);
  }
}
