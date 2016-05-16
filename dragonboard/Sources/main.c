// Collatz Sequence Generator
#include <hidef.h>      /* common defines and macros */
#include <mc9s12dg256.h>     /* derivative information */
#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"

#include "main_asm.h" /* interface to the assembly module */
#include "queue.h"

#define MAX_LONG_VALUE 4294967295
#define BUFF_SIZE 256

int new_sequence(void);
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

int result; // Result after computing sequence.
int i, display_mode, getline_timeout, at_timeout;
char c, status;
void set_rgb_led(char r, char g, char b);
void send_string_newline_sci1(char* str);
void send_at_command_sci1(char* str);
void empty_queue(void);

void interrupt 21 comm_handler(void){
     qstore(read_SCI1_Rx());
}

void interrupt 7 async_handler(void){
     display_mode = ~PTH & 1; // Get bit 0 of PORTH
     if (getline_timeout > 0) --getline_timeout;
     if (at_timeout > 0) --at_timeout;
    
     switch (status){
      case 'i':                       // IDLE
        set_rgb_led(0x00, 0xff, 0x00);
        break;
      case 'c':                       // COMMUNICATING
        set_rgb_led(0xff, 0xff, 0x00);
        break;
      case 'w':                       // WORKING
        set_rgb_led(0x80, 0x00, 0x40);
        break;
      case 'e':                       // ERROR
        set_rgb_led(0xff, 0x00, 0x00);
        break;
      case 'b':                       // BOOTUP
        set_rgb_led(0x00, 0x00, 0xff);
        break;
      default:                        // ?
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
  status = 'b';
  
  // Populate binary search tree:

  set_lcd_addr(0);  
  send_at_command_sci1("ATE0");  // change to ATE1 for debug
  
  status = 'i';
  
  // Establish connection to server.
  send_at_command_sci1("AT+CWMODE=1");  // Set ESP to station mode
  
  send_at_command_sci1("AT+CIPMODE=0"); // Set ESP to normal transmission mode
  
  send_at_command_sci1("AT+CIPMUX=0");  // Set ESP to single-connection mode 
  
  send_at_command_sci1("AT+CWJAP=\"Freynet\",\"\""); // Connect to network
  
  send_at_command_sci1("AT+CIPSTART=\"TCP\",\"fpf3.net\",12345"); // connect to server

  
  while(1){
    command = '\0';   
    while(qempty());
    command = getq();
    
    switch (command) {
      case 'n':
        status = 'w';
        result = new_sequence();
        send_at_command_sci1("AT+CIPSTART=\"TCP\",\"fpf3.net\",12345"); // connect to server

        break;
        
    }
    outchar0(result);
  }
}

int new_sequence(void) {
  unsigned long num = 0, newnum = 0;
  char c;
  for(i = 0; i < 10; i++) {
    while(qempty());
    c = getq();
    newnum = (c - '0') * pow(10, 9 - i);
    if (num + newnum < num) { // Check for rollover
         status = 'e';
         set_lcd_addr(0);
         type_lcd("Sequence goes");
         set_lcd_addr(0x40);
         type_lcd("above 2^32!");
         return -1;
    }
    
    num += newnum; // we passed test
  }
  
  clear_lcd();
   
  while (num != 1){
      
      set_lcd_addr(0);
      write_long_lcd(num);
     if (num % 2 == 0)
        num /= 2;
     else{
        newnum = (num * 3) + 1;
        if (newnum < num){ // Check for rollover.
          status = 'e';
          set_lcd_addr(0);
          type_lcd("Sequence goes");
          set_lcd_addr(0x40);
          type_lcd("above 2^32!");
          return -1;
        }
        num = newnum; // we passed test
     }
     if (display_mode) ms_delay(750);// to make sure we see what numbers we're getting
  }
  write_long_lcd(num);
  status = 'i'; 
  return num;  
}

void empty_queue(){
  while (!qempty())
    c = getq();
  
  c = '\0';
}

void set_rgb_led(char r, char g, char b){
  motor0(r);
  motor1(g);
  motor2(b);
}

void send_string_newline_sci1(char* str) {
   int i;
   status = 'c';
   for (i = 0; str[i] != '\0'; i++) outchar1(str[i]);
   outchar1('\n');
   status = 'i';
}

void send_at_command_sci1(char* str){
  long i;
  status = 'c';
  for (i = 0; str[i] != '\0'; i++) outchar1(str[i]);
  outchar1('\n');

  at_timeout = 10 * 100; // set at_timeout to ~10 sec  
  while(at_timeout){
    while(qempty() && at_timeout);
    c = getq();
    if (c == 'O'){
      while(qempty() && at_timeout);
      c = getq();
      if (c == 'K')
        break;
      else
        continue;
    }
  }
  
  if (at_timeout)
    status = 'i';
  else{
    status = 'e';
    set_lcd_addr(0);
    type_lcd("writing timed");
    set_lcd_addr(0x40);
    type_lcd("out.");
  }
}

unsigned long pow(int a, int b) {
  unsigned long result = 1, i;
  for (i = 0; i < b; i++)
    result *= a;
  return result;
}