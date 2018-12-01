/*
 * C64_touch_screen_pro.ino
 *
 * Created on: 1.12.2018
 * Author: Teemu Leppänen (tjlepp@gmail.com)
 *
 * This work is licensed under Creative Commons
 * Attribution-NonCommercial-ShareAlike (CC BY-NC-SA 4.0)
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * 
 */
 
#include <SPI.h>

// MCP4151
#define SS            10
#define CS_YL         9
#define CS_YH         8
#define CS_XH         7
#define CS_XL         6
#define WIPER0        0x00
#define TCON          0x04
#define P0BCON        0x01

// NEXTION TOUCH SCREEN
#define DISPLAY       Serial
#define TOUCH         0x65
#define COORDS        0x67
#define PRESS         0x01
#define RELEASE       0x00
#define SCREEN        0x01
#define BUTTONLEFT    0x02
#define BUTTONRIGHT   0x08
#define MSGEND        0xFF
#define LBUTTON       2         // controller port pin 3
#define RBUTTON       3         // controller port pin 4
#define SCALE_FACTOR  1.1       // coordinate scaling
#define AF            0.1       // low pass filter
#define BF            0.9       // low pass filter

// PROGRAM
enum program_states { RECEIVE=1, END1, END2 };
enum program_states state = RECEIVE;

uint8_t msg[16],msgcnt=0,i=0,c=0;
uint16_t potx,poty;
float scalex,scaley,xf=0,yf=0;

// *********** 
//  setup
// *********** 
void setup() {

  // Pins - tri-state
  pinMode(LBUTTON,INPUT);
  pinMode(RBUTTON,INPUT);

  // SPI init
  pinMode(SS,OUTPUT);
  pinMode(CS_YL,OUTPUT);
  pinMode(CS_YH,OUTPUT);
  pinMode(CS_XL,OUTPUT);
  pinMode(CS_XH,OUTPUT);
  SPI.begin();
  potWrite(CS_YH,TCON,P0BCON);    
  delay(10);
  potWrite(CS_XH,TCON,P0BCON);        

  // Display init
  delay(1000);
  DISPLAY.begin(9600);
  DISPLAY.print("baud=19200");
  DISPLAY.write(0xFF);
  DISPLAY.write(0xFF);
  DISPLAY.write(0xFF);
  DISPLAY.end();
  DISPLAY.begin(19200);  
}

// *********** 
//  parseEvent
// *********** 
void parseEvent() {

  switch(msg[0]) {
    
    // Touch event: 0x65 0x00 0x01 0x01 0xFF 0xFF 0xFF
    case TOUCH:
      switch (msg[2]) {

        case BUTTONLEFT:
          if (msg[3] == PRESS) {
            pinMode(LBUTTON,OUTPUT);            
            digitalWrite(LBUTTON,LOW);
          }
          else if (msg[3] == RELEASE) {
            pinMode(LBUTTON,INPUT);            
          }          
          break;
          
        case BUTTONRIGHT: 
          if (msg[3] == PRESS) {
            pinMode(RBUTTON,OUTPUT);                        
            digitalWrite(RBUTTON,LOW);
          }
          else if (msg[3] == RELEASE) {
            pinMode(RBUTTON,INPUT);            
          }
          break;
      }
      break;
      
    // Coord event: 67 45 1 0 0 C5 0 0 0 1 0xFF 0xFF 0xFF
    case COORDS:
      if (msg[6] == 0xFF) break; // display print error?
      potx = (msg[2] << 8) + (msg[1] & 0xFF);
      poty = (msg[6] << 8) + (msg[5] & 0xFF);
      if (poty > 200) break; // ignore button area
      if (potx > 320) potx = 320;  
      scalex = 512-((potx / 320.0) * 512.0);
      scaley = 512-((poty / 200.0) * 512.0);
      // no coordinate scaling
      potx = (int)scalex;
      poty = (int)scaley;
      // coordinate scaling
      // potx = (int)(scalex*SCALE_FACTOR);
      // poty = (int)(scaley*SCALE_FACTOR);
      // low pass filter
      // xf = (AF*xf)+(BF*scalex);
      // yf = (AF*yf)+(BF*scaley);
      // potx = (int)(xf+0.5);
      // poty = (int)(yf+0.5);
      potWrite(CS_YL,WIPER0,poty / 2);
      delay(1);        
      potWrite(CS_YH,WIPER0,poty / 2);        
      delay(1);        
      potWrite(CS_XL,WIPER0,potx / 2);        
      delay(1);        
      potWrite(CS_XH,WIPER0,potx / 2);        
      delay(1);        
      break;
  }
}

// *********** 
//  potWrite
// *********** 
int potWrite(byte cs, byte addr, byte value) {
  digitalWrite(cs,LOW);
  SPI.transfer(addr);
  SPI.transfer(value);
  digitalWrite(cs,HIGH);
}

// *********** 
//  loop
// *********** 
void loop() {

  if (DISPLAY.available()) {
    
    c = DISPLAY.read();
    msg[msgcnt++] = c;

    switch (state) {
      case RECEIVE:
        if (c == MSGEND) state = END1;
        break;
      case END1:
        if (c == MSGEND) state = END2;
        else state = RECEIVE;
        break;
      case END2:
        if (c == MSGEND) {
          parseEvent();          
        }
        msgcnt = 0;
        state = RECEIVE;
        break;                              
    }
  } 
}
