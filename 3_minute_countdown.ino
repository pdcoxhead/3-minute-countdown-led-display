    /*--------------------------------------------------------------------------------------

 dmd_test.cpp 
   Demo and example project for the Freetronics DMD, a 512 LED matrix display
   panel arranged in a 32 x 16 layout.

 Copyright (C) 2011 Marc Alexander (info <at> freetronics <dot> com)

 See http://www.freetronics.com/dmd for resources and a getting started guide.

 Note that the DMD library uses the SPI port for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD is being written to.

 USAGE NOTES
 -----------

 - Place the DMD library folder into the "arduino/libraries/" folder of your Arduino installation.
 - Get the TimerOne library from here: http://code.google.com/p/arduino-timerone/downloads/list
   or download the local copy from the DMD library page (which may be older but was used for this creation)
   and place the TimerOne library folder into the "arduino/libraries/" folder of your Arduino installation.
 - Restart the IDE.
 - In the Arduino IDE, you can open File > Examples > DMD > dmd_demo, or dmd_clock_readout, and get it
   running straight away!

 * The DMD comes with a pre-made data cable and DMDCON connector board so you can plug-and-play straight
   into any regular size Arduino Board (Uno, Freetronics Eleven, EtherTen, USBDroid, etc)
  
 * Please note that the Mega boards have SPI on different pins, so this library does not currently support
   the DMDCON connector board for direct connection to Mega's, please jumper the DMDCON pins to the
   matching SPI pins on the other header on the Mega boards.

 This example code is in the public domain.
 The DMD library is open source (GPL), for more see DMD.cpp and DMD.h

--------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"
#include "Arial_black_16.h"
#include "Arial_Black_16_ISO_8859_1.h"
#include "Arial14.h"
#include <Time.h>  

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
#define TIMER 180
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
char serialBuffer[256];
char topMessage[256];

/*--------------------------------------------------------------------------------------
  Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
  called at the period set in Timer1.initialize();
--------------------------------------------------------------------------------------*/
void ScanDMD()
{ 
  dmd.scanDisplayBySPI();
}

class CountDownAction {
public:
  CountDownAction(int _countDownStartValue, void (*_toAction)(char*, int)) {
   toAction = _toAction;
   countDownStartValue = _countDownStartValue;
   reset();
  }
  void checkAction(void) {
    if ((countDown--) == 0) {
      reset();
      if (toAction != NULL) {
        (*toAction)(NULL, 0);
      }
    }
  }
  void reset(void) {
    countDown = countDownStartValue;
  }
private:
  int countDownStartValue;
  int countDown;
  void (*toAction)(char*, int); 

};
void mymemset(char* data, const byte value, const int byteArrayLength) {
  for (int i = 0; i < byteArrayLength; i++) {
    data[i] = value; 
  } 
} 


CountDownAction actions[] = {   CountDownAction(300, updateDisplay), 
                                CountDownAction(100, readSerial   )  };

void checkActions() {
  for (int i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
   actions[i].checkAction();
  }
}

int piezo = 3;
/*--------------------------------------------------------------------------------------
  setup
  Called by the Arduino architecture before the main loop begins
--------------------------------------------------------------------------------------*/
void setup(void)
{
  Serial.begin(9600);
  mymemset(serialBuffer, 0x0, sizeof(serialBuffer));
  mymemset(topMessage, 0x0, sizeof(topMessage));
   //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
   Timer1.initialize( 5000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

   //clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
   sprintf(topMessage, "...");
   
   pinMode(piezo, OUTPUT);

}

/*--------------------------------------------------------------------------------------
  loop
  Arduino architecture main loop
--------------------------------------------------------------------------------------*/
void loop(void)
{
  delay(1);
  checkActions();
}

void updateDisplay(char* data, int dataSize) {
   // 10 x 14 font clock, including demo of OR and NOR modes for pixels so that the flashing colon can be overlayed
   dmd.clearScreen( true );
//   dmd.selectFont(SystemFont5x7);
//   dmd.selectFont( Arial_Black_16_ISO_8859_1 );
//dmd.selectFont(Arial_14);
dmd.selectFont(Arial_Black_16);
                  //         1         2         3
                  //1234567890123456789012345678901234567890
   //dmd.drawMarquee(topMessage,strlen(topMessage),(32*DISPLAYS_ACROSS)-1,0);
   long start=millis();
   long timer=start;
   boolean ret=false;
   boolean sound = false;
   int sirenValue = 0;
   int countDown = TIMER;
   int lastSecond = 0;
   while(!ret){
       char myTimerString[32];
       if ((timer / 1000) > lastSecond) {
        countDown--;
        lastSecond = timer / 1000;
       }
       
       int minutes = countDown / 60;
       int seconds = countDown - (minutes * 60);
       
       if (countDown > 1 && countDown < TIMER - 1) {
          sound = false; 
          sirenValue=0;
          siren(sirenValue);
        }
        else if (countDown == 0) {
          countDown = TIMER;
        }
       else {
          sound = true;
          sirenValue = 200;
          siren((int)sirenValue);
       }
       sprintf(myTimerString, "%01d:%02d  ", minutes, seconds);
       dmd.drawString(0,1, myTimerString, 5, GRAPHICS_NORMAL);
       timer=millis();
   }

}

void siren(int value) {
  analogWrite(piezo, value);
}

void readSerial(char* data, int dataSize) {
  int i = 0;
  while (Serial.available()) {
    serialBuffer[i++] = Serial.read();
  }
  serialBuffer[i] = NULL;
  if (i != 0) 
    strncpy(topMessage, serialBuffer, sizeof(topMessage) - 1);
}

