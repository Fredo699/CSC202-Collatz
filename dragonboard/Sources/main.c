// Collatz Sequence Generator
#include <hidef.h>      /* common defines and macros */
#include <mc9s12dg256.h>     /* derivative information */
#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"

#include "main_asm.h" /* interface to the assembly module */

#define MAX_LONG_VALUE 4294967295

unsigned long new_sequence(void);
unsigned long pow(int a, int b);

char command; // Command received from ESP8266
 
  /* Commands to and from the ESP8266
   * 'n' (0x6e) -> New number to compute the sequence of
   *                -Only ever sent by ESP, dragonboard gets number, and computes sequence.
   *                -Delimited by '\0' (0x00)
   * 'h' (0x68) -> Halt execution
   *                -Only ever sent by ESP, causes dragonboard to cease all execution, and go into SWI
   * 'r' (0x72) -> Ready for new number
   *                -When sent by dragonboard, indicates that the dragonboard is ready to compute the next sequence.
   *                -When sent by the ESP, indicates that the ESP is ready to receive a result.
   * 'f' (0x66) -> Finished computing sequence, have result
   *                -Only ever sent by dragonboard. Sends TLV-encoded sequence of numbers back to ESP.
   *                -Delimited by '\0' (0x00) for end of sequence, and 'c' (0x63) for next number in sequence.
   * 'e' (0x65) -> Error
   *                -Sent by either
   *                -Indicates that there was an issue processing the last command received.
   *                -Next byte indicates type of error
   *                  (0x01) -> Number exceeds MAX_LONG_VALUE
   */

unsigned long result=0; // Result after computing sequence.
int i;

void main(void) {
  PLL_init();
  lcd_init();
  SCI0_init(9600); // Channel to talk to ESP8266
  while(1){
    command = inchar0();
    switch (command) {
      case 'n':
        result = new_sequence();
    }
    outchar0(result);
  }
}

unsigned long new_sequence(void) {
  unsigned long num = 0;
  char c;
  for(i = 0; i < 10; i++) {
    c = inchar0();
    num += (c - '0') * pow(10, 9 - i);
    outchar0(c);
  }
  
  if (num > MAX_LONG_VALUE){  
    outchar0('e');            // Send error code 0x01 to ESP8266
    outchar0(0x01);
    return 0;
  }
  
  clear_lcd();
   
  while (num != 1){
      if (num >= (MAX_LONG_VALUE / 3)){
         set_lcd_addr(0);
         type_lcd("Sequence goes");
         set_lcd_addr(0x40);
         type_lcd("above 2^32!");
         return 0;
      }
      set_lcd_addr(0);
      write_long_lcd(num);
     if (num % 2 == 0)
        num /= 2;
     else
        num = (num * 3) + 1;
     ms_delay(750);
  }
  write_long_lcd(num); 
  return num;  
}

unsigned long pow(int a, int b) {
  unsigned long result = 1, i;
  for (i = 0; i < b; i++)
    result *= a;
  return result;
}