/*==================================CPEG222=============================
 * Program:		P5.c
 * Authors: 	Luke Grippa and Mason Mostrom
 * Date: 		11/15/2018
 * Description: 
 * This program reads the PIR sensor connected to 5V tolerant PMOD pin B1.
 * The output goes high (3.3V)when the PIR senses motion. The program then
 * flashes the 8 LEDS on Basys MX3 if the PIR output is high. Timer1 (w/o
 * interupts is used for timing. Version B uses the UART to send message.
========================================================================*/
#ifndef _SUPPRESS_PLIB_WARNING  //turn off MPLAB PLIB warnings
#define _SUPPRESS_PLIB_WARNING
#endif

#include <xc.h>             //include Microchip XC compiler header
#include <plib.h>           //include Microchip peripheral library
#include <stdio.h>
#include <stdlib.h>
#include <dsplib_dsp.h> 
#include <fftc.h>
#include "config.h"         //include Digilent Basys MX3 configuration file
#include "adc.h"
#include "led.h"            //include Digilent LED library
#include "pmods.h"          //include Digilent PMOD library
#include "btn.h"
#include "lcd.h"
#include "srv.h"
#include "ssd.h"
#include "swt.h"
#include "tone.h"
#include "utils.h"
#include "math.h"
#include "IrDA.h"


#pragma config JTAGEN = OFF     // Turn off JTAG - required to use Pin RA0 as IO
#pragma config FNOSC = PRIPLL   //configure system clock 80 MHz
#pragma config FSOSCEN = OFF    // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT     // Primary Oscillator Configuration (XT osc mode)
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1
#pragma config FPBDIV = DIV_2  //configure peripheral bus clock to 40 MHz

#define DELAY   15625           // 1000ms, (40MHz/256 = 64.0us period) 64.0us x 39062 = 1sec
#define PIR_PIN     prt_PMODS_JB1   // PIR digital input pin

/* ------------- Forward Declarations -------------------------------------- */
void delay_ms(int ms);
void update_SSD(float value);
char strMsg[80];
char strMsg1[80];
int stopTimer = 1;
int ssdTimer;
int mode = 0;
int L;
int R;
int Ml;
int Mr;
int finish = 0;
int clap = 0;
int backUp = 0;
int celebrate = 0;

void delay_ms(int ms) {
    int i, counter;
    for (counter = 0; counter < ms; counter++) {
        for (i = 0; i < 300; i++) {
        } //software delay ~1 millisec
    }
}

main(void) {
    int buttonLock;
    char strMsg[80]; //String used to display on the LCD
    ADC_Init();
    IRDA_Init(9600);
    LCD_Init(); // Setup the Liquid Crystal Display
    LED_Init(); //Setup the LEDs as digital outputs
    SSD_Init(); // Setup the Seven Segment Display
    SWT_Init(); // Setup the switches
    SRV_Init(); // Setup the servos
    MIC_Init(); // Setup the microphone
    TONE_Init(); // Setup the tone library
    PMODS_InitPin(1, 7, 1, 0, 0); // Initialize PMODB pin JB7 as an input
    PMODS_InitPin(1, 8, 1, 0, 0); // Initialize PMODB pin JB8 as an input
    PMODS_InitPin(1, 9, 1, 0, 0); // Initialize PMODB pin JB9 as an input
    PMODS_InitPin(1, 10, 1, 0, 0); // Initialize PMODB pin JB10 as an input

    OpenCoreTimer(DELAY * 200); //configure CoreTimer
    mConfigIntCoreTimer(CT_INT_ON | CT_INT_PRIOR_2);
    INTEnableSystemMultiVectoredInt();

    sprintf(strMsg, "Tm:20  Mode:TEST");
    LCD_WriteStringAtPos(strMsg, 0, 0);

    while (1) { // main loop (continuous))
        L = PMODS_GetValue(1, 7);
        Ml = PMODS_GetValue(1, 8);
        Mr = PMODS_GetValue(1, 9);
        R = PMODS_GetValue(1, 10);

        // Test Mode Mic Input Level 
        if (mode == 0) {
            if (MIC_Val() < 520) {
                LED_SetGroupValue(0b00000001);
            } else if (MIC_Val() > 519 && MIC_Val() < 540) {
                LED_SetGroupValue(0b00000011);
            } else if (MIC_Val() > 539 && MIC_Val() < 560) {
                LED_SetGroupValue(0b00000111);
            } else if (MIC_Val() > 559 && MIC_Val() < 580) {
                LED_SetGroupValue(0b00001111);
            } else if (MIC_Val() > 579 && MIC_Val() < 600) {
                LED_SetGroupValue(0b00011111);
            } else if (MIC_Val() > 599 && MIC_Val() < 620) {
                LED_SetGroupValue(0b00111111);
            } else if (MIC_Val() > 619 && MIC_Val() < 640) {
                LED_SetGroupValue(0b01111111);
            } else if (MIC_Val() > 639) {
                LED_SetGroupValue(0b11111111);
            }
        }

        // Clap Counter
        if (clap == 0 && mode == 1 && MIC_Val() > 599) {
            clap = 1;
            delay_ms(100);

        } else if (clap == 1 && mode == 1) {
            int i, counter;
            for (counter = 0; counter < 2000; counter++) {
                for (i = 0; i < 300; i++) {
                    if (MIC_Val() > 599) {
                        clap = 2;
                        break;
                    }
                } //software delay ~1 millisec
                if (clap == 2) {
                    clap = 3;
                    break;
                }
            }
        }


        // Infared if Statements controlling Servo Motors
        if (clap == 3 || backUp == 1) {
            if (!L && !Ml && !Mr && !R) { // 0b0000
                if (finish == 0) {
                    sprintf(strMsg, "FWD");
                    LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                    LCD_WriteStringAtPos(strMsg, 1, 0); // Left Serbo LCD
                    LED_SetValue(0, 0); // Right Servo LED's
                    LED_SetValue(1, 0);
                    LED_SetValue(2, 1);
                    LED_SetValue(3, 1);
                    LED_SetValue(4, 1); // Left Servo LED's
                    LED_SetValue(5, 1);
                    LED_SetValue(6, 0);
                    LED_SetValue(7, 0);
                    SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                    SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                    stopTimer = 0;
                } else if (finish == 1) {
                    sprintf(strMsg, "STP");
                    LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                    LCD_WriteStringAtPos(strMsg, 1, 0); // Left Serbo LCD
                    LED_SetValue(0, 0); // Right Servo LED's
                    LED_SetValue(1, 0);
                    LED_SetValue(2, 0);
                    LED_SetValue(3, 0);
                    LED_SetValue(4, 0); // Left Servo LED's
                    LED_SetValue(5, 0);
                    LED_SetValue(6, 0);
                    LED_SetValue(7, 0);
                    SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                    SRV_SetPulseMicroseconds1(1530); // Left Servo Stop
                    stopTimer = 1;
                    celebrate = 1;

                }

            } else if (L && !Ml && !Mr && R) { // 0b1001
                sprintf(strMsg, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 0); // Right Servo LED's
                LED_SetValue(1, 0);
                LED_SetValue(2, 1);
                LED_SetValue(3, 1);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                finish = 1;

            } else if (!L && Ml && Mr && R) { // 0b0111
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "STP");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 0); // Right Servo LED's
                LED_SetValue(1, 0);
                LED_SetValue(2, 1);
                LED_SetValue(3, 1);
                LED_SetValue(4, 0); // Left Servo LED's
                LED_SetValue(5, 0);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                SRV_SetPulseMicroseconds1(1530); // Left Servo Stop

            } else if (L && Ml && Mr && !R) { // 0b1110
                sprintf(strMsg, "STP");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 0); // Right Servo LED's
                LED_SetValue(1, 0);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward

            } else if (!L && !Ml && Mr && R) { // 0b0011
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "REV");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 0); // Right Servo LED's
                LED_SetValue(1, 0);
                LED_SetValue(2, 1);
                LED_SetValue(3, 1);
                LED_SetValue(4, 0); // Left Servo LED's
                LED_SetValue(5, 0);
                LED_SetValue(6, 1);
                LED_SetValue(7, 1);
                SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                SRV_SetPulseMicroseconds1(900); // Left Servo Reverse

            } else if (L && Ml && !Mr && !R) { // 0b1100
                sprintf(strMsg, "REV");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(2100); // Right Servo Reverse
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward

            } else if (L && !Ml && !Mr && !R) { // 0b1000
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(1300); // Right Servo Forward Slow
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward

            } else if (!L && !Ml && !Mr && R) { // 0b0001
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                SRV_SetPulseMicroseconds1(1700); // Left Servo Forward Slow

            } else if (L && !Ml && Mr && R) { // 0b1011
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                SRV_SetPulseMicroseconds1(1600); // Left Servo Forward Slow

            } else if (L && Ml && !Mr && R) { // 0b1101
                sprintf(strMsg, "FWD");
                sprintf(strMsg1, "FWD");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg1, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 1); // Left Servo LED's
                LED_SetValue(5, 1);
                LED_SetValue(6, 0);
                LED_SetValue(7, 0);
                SRV_SetPulseMicroseconds0(1400); // Right Servo Forward Slow
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward

            } else if (!L && !Ml && Mr && !R) { // 0b0010

            } else if (!L && Ml && !Mr && !R) { // 0b0100

            } else if (!L && Ml && !Mr && R) { // 0b0101

            } else if (L && !Ml && Mr && !R) { // 0b1010

            } else if (L && Ml && Mr && R) { // 0b1111
                sprintf(strMsg, "REV");
                LCD_WriteStringAtPos(strMsg, 1, 13); // Right Servo LCD
                LCD_WriteStringAtPos(strMsg, 1, 0); // Left Serbo LCD
                LED_SetValue(0, 1); // Right Servo LED's
                LED_SetValue(1, 1);
                LED_SetValue(2, 0);
                LED_SetValue(3, 0);
                LED_SetValue(4, 0); // Left Servo LED's
                LED_SetValue(5, 0);
                LED_SetValue(6, 1);
                LED_SetValue(7, 1);
                SRV_SetPulseMicroseconds0(2100); // Right Servo Reverse
                SRV_SetPulseMicroseconds1(900); // Left Servo Reverse

            }
        }

        // Celebration
        if (stopTimer == 1 && celebrate == 1) {
            delay_ms(10000);
            int counters;
            SRV_SetPulseMicroseconds0(2100); // Right Servo Reverse
            SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
            for (counters = 0; counters < 20; counters++) {
                LED_SetGroupValue(0b10101010);
                delay_ms(400);
                LED_SetGroupValue(0b01010101);
                delay_ms(400);

            }
            SRV_SetPulseMicroseconds0(900); // Right Servo Forward
            SRV_SetPulseMicroseconds1(900); // Left Servo Reverse
            for (counters = 0; counters < 20; counters++) {
                LED_SetGroupValue(0b10101010);
                delay_ms(400);
                LED_SetGroupValue(0b01010101);
                delay_ms(400);

            }
            SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
            SRV_SetPulseMicroseconds1(1530); // Left Servo Stop
            LED_SetGroupValue(0b00000000);
            break;
        }

        // Ready and Test Modes with button C
        if (BTN_GetValue('C') && !buttonLock) {//If BTNC change mode
            if (mode == 0) {
                mode = 1;
                sprintf(strMsg, "Tm:20 Mode:READY");
                LCD_WriteStringAtPos(strMsg, 0, 0);
                LED_SetGroupValue(0b00000000);
            } else if (mode == 1) {
                mode = 0;
                sprintf(strMsg, "Tm:20  Mode:TEST");
                LCD_WriteStringAtPos(strMsg, 0, 0);
            }
            buttonLock = 1;
        } else if (BTN_GetValue('U') && !buttonLock) { //If BTNU is pressed robot starts course 
            if (mode == 1) {
                backUp = 1;
            } else {
                backUp = 0;
            }
            buttonLock = 1;
        } else if (buttonLock && !BTN_GetValue('C') && !BTN_GetValue('U')) {
            delay_ms(100);
            buttonLock = 0;
        }
    }
}

void __ISR(_CORE_TIMER_VECTOR, IPL2SOFT) _CoreTimerHandler(void) {
    UpdateCoreTimer(DELAY); //set Timer1 delay time

    if (stopTimer == 0) {
        ssdTimer += 1;
    } else if (stopTimer == 1) {
        ssdTimer = 0;
    }

    update_SSD(ssdTimer);

    UpdateCoreTimer(4000000L);
    mCTClearIntFlag(); // Macro function to clear the interrupt flag
}

void update_SSD(float value) {
    float thou, hunds, tens, ones;
    if (value < 10) {
        char SSD1 = 0b0000000; //SSD setting for 1st SSD (LSD)
        char SSD2 = 0b0000000; //SSD setting for 2nd SSD
        char SSD3 = 0b0000000; //SSD setting for 3rd SSD 
        char SSD4 = 0b0000000; //SSD setting for 4th SSD (MSD)

        ones = floor(value);
        if (ones > 0)
            SSD1 = ones; //SSD4 = display_char[thous];
        else
            SSD1 = 0; //blank display

        SSD_WriteDigits(SSD1, 0, 17, 17, 0, 1, 0, 0);
    } else if (value >= 10 && value < 100) {
        char SSD1 = 0b0000000; //SSD setting for 1st SSD (LSD)
        char SSD2 = 0b0000000; //SSD setting for 2nd SSD
        char SSD3 = 0b0000000; //SSD setting for 3rd SSD 
        char SSD4 = 0b0000000; //SSD setting for 4th SSD (MSD)

        tens = floor(value / 10);
        if (tens > 0)
            SSD2 = tens; //SSD4 = display_char[thous];
        else
            SSD2 = 0; //blank display
        ones = floor(fmod(value, 10));
        if (ones > 0)
            SSD1 = ones; //SSD4 = display_char[thous];
        else
            SSD1 = 0; //blank display
        SSD_WriteDigits(SSD1, SSD2, 17, 17, 0, 1, 0, 0);
    } else if (value >= 100 && value < 1000) {
        char SSD1 = 0b0000000; //SSD setting for 1st SSD (LSD)
        char SSD2 = 0b0000000; //SSD setting for 2nd SSD
        char SSD3 = 0b0000000; //SSD setting for 3rd SSD 
        char SSD4 = 0b0000000; //SSD setting for 4th SSD (MSD)

        hunds = floor(value / 100);
        if (hunds > 0)
            SSD3 = hunds; //SSD4 = display_char[thous];
        else
            SSD3 = 0; //blank display
        tens = floor(fmod(value, 100) / 10);
        if (tens > 0)
            SSD2 = tens; //SSD4 = display_char[thous];
        else
            SSD2 = 0; //blank display
        ones = floor(fmod(value, 10));
        if (ones > 0)
            SSD1 = ones; //SSD4 = display_char[thous];
        else
            SSD1 = 0; //blank display
        SSD_WriteDigits(SSD1, SSD2, SSD3, 17, 0, 1, 0, 0);
    } else if (value >= 1000) {
        char SSD1 = 0b0000000; //SSD setting for 1st SSD (LSD)
        char SSD2 = 0b0000000; //SSD setting for 2nd SSD
        char SSD3 = 0b0000000; //SSD setting for 3rd SSD 
        char SSD4 = 0b0000000; //SSD setting for 4th SSD (MSD)

        thou = floor(value / 1000);
        if (thou > 0)
            SSD4 = thou; //SSD4 = display_char[thou];
        else
            SSD4 = 0; //blank display
        hunds = floor(fmod(value, 1000) / 100);
        if (hunds > 0)
            SSD3 = hunds; //SSD4 = display_char[thous];
        else
            SSD3 = 0; //blank display
        tens = floor(fmod(value, 100) / 10);
        if (tens > 0)
            SSD2 = tens; //SSD4 = display_char[thous];
        else
            SSD2 = 0; //blank display
        ones = floor(fmod(value, 10));
        if (ones > 0)
            SSD1 = ones; //SSD4 = display_char[thous];
        else
            SSD1 = 0; //blank display
        SSD_WriteDigits(SSD1, SSD2, SSD3, SSD4, 0, 1, 0, 0);
    }
}