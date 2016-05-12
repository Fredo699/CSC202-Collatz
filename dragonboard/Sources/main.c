// Collatz Sequence Generator
#include <hidef.h>      /* common defines and macros */
#include <mc9s12dg256.h>     /* derivative information */
#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"

#include "main_asm.h" /* interface to the assembly module */
#include "queue.h"
#include "btree.h"

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

unsigned long result; // Result after computing sequence.
int i, display_mode;
treeNode *searchtab_root;
char status;           // Current status of dragonboard

void set_rgb_led(char r, char g, char b);

void interrupt 21 comm_handler(){
     qstore(read_SCI1_Rx());
}

void interrupt 7 async_handler(){
     display_mode = ~PTH & 1; // Get bit 0 of PORTH
     
     switch (status){
      case 'i':
        set_rgb_led(0x00, 0xff, 0x00);
        break;
      case 'w':
        set_rgb_led(0x80, 0x00, 0x40);
        break;
      case 'e':
        set_rgb_led(0xff, 0x00, 0x00);
        break;
      default:
        set_rgb_led(0xff, 0xff, 0xff);
        break;
     }
     
     clear_RTI_flag();
}

void main(void) {
  PLL_init();
  lcd_init();
  SCI0_init(9600); 
  SCI1_int_init(9600); // Channel to talk to ESP8266
  motor0_init(); // These functions actually control PWM outputs
  motor1_init(); // We use them to run the RGB LED.
  motor2_init();
  RTI_init();
  SW_enable();
  
  initq();
  
  DDRH = 0; // PORTH is an input.
  result = 0;
  status = 'i';
  while(1){
    command = '\0';   
    while(qempty());
    command = getq();
    
    switch (command) {
      case 'n':
        status = 'w';
        result = new_sequence();
        
    }
    outchar0(result);
  }
}

unsigned long new_sequence(void) {
  unsigned long num = 0;
  char c;
  for(i = 0; i < 10; i++) {
    while(qempty());
    c = getq();
    num += (c - '0') * pow(10, 9 - i);
  }
  
  clear_lcd();
   
  while (num != 1){
      // the only way we'll go over MAX_LONG_VALUE is if we have an odd number
      // that is greater than 1/3 the max long value.
      
      if (num >= (MAX_LONG_VALUE / 3) && num % 2 == 1){
         status = 'e';
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
     if (display_mode) ms_delay(750); // to make sure we see what numbers we're getting
  }
  write_long_lcd(num);
  status = 'i'; 
  return num;  
}

void set_rgb_led(char r, char g, char b){
  motor0(r);
  motor1(g);
  motor2(b);
}

void send_string_newline_sci1(char* str) {
   int i; for (i = 0; i < sizeof str; i++) outchar1(str[i]);
   outchar1('\n');
}

unsigned long pow(int a, int b) {
  unsigned long result = 1, i;
  for (i = 0; i < b; i++)
    result *= a;
  return result;
}