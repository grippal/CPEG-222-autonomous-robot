/*==================================CPEG222=============================
 * Program:		FP.c
 * Authors: 	Luke Grippa and Mason Mostrom
 * Date: 		12/7/2018
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
#include "rgbled.h"
#include "srv.h"
#include "ssd.h"
#include "swt.h"
#include "ultr.h"
#include "utils.h"
#include "math.h"


#pragma config JTAGEN = OFF     // Turn off JTAG - required to use Pin RA0 as IO
#pragma config FNOSC = PRIPLL   //configure system clock 80 MHz
#pragma config FSOSCEN = OFF    // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT     // Primary Oscillator Configuration (XT osc mode)
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1
#pragma config FPBDIV = DIV_2  //configure peripheral bus clock to 40 MHz

#define DELAY 40000        //(40MHz = 25ns period) 25ns x 40000 = 1ms  
//156250         // 1000ms, (40MHz/256 = 6.4us period) 6.4us x 156250 = 1sec
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
int start = 0;
int backUp = 0;
int celebrate = 0;
float rangeCm[90];
float pulse;
float pulseMs;
float distCm;
int is, counters;
int turn = 0;
int right = 0;
int left = 0;
int rLine = 0;
int lLine = 0;

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
    LCD_Init(); // Setup the Liquid Crystal Display
    LED_Init(); //Setup the LEDs as digital outputs
    SSD_Init(); // Setup the Seven Segment Display
    SWT_Init(); // Setup the switches
    SRV_Init(); // Setup the servos
    RGBLED_Init();
    ULTR_Init(0, 3, 0, 4);
    PMODS_InitPin(1, 7, 1, 0, 0); // Initialize PMODB pin JB7 as an input
    PMODS_InitPin(1, 8, 1, 0, 0); // Initialize PMODB pin JB8 as an input
    PMODS_InitPin(1, 9, 1, 0, 0); // Initialize PMODB pin JB9 as an input
    PMODS_InitPin(1, 10, 1, 0, 0); // Initialize PMODB pin JB10 as an input

    OpenCoreTimer(DELAY * 100); //configure CoreTimer
    mConfigIntCoreTimer(CT_INT_ON | CT_INT_PRIOR_2);
    INTEnableSystemMultiVectoredInt();

    sprintf(strMsg, "Tm:20 LEDMcQueen");
    LCD_WriteStringAtPos(strMsg, 0, 0);

    while (1) { // main loop (continuous))
        pulse = (ULTR_MeasureDist());
        pulseMs = (pulse / 1000);
        distCm = (pulse * 0.034) / 2;
        L = PMODS_GetValue(1, 7);
        Ml = PMODS_GetValue(1, 8);
        Mr = PMODS_GetValue(1, 9);
        R = PMODS_GetValue(1, 10);

        // Ultrasonic Sensor
        sprintf(strMsg, "Range:%.2fcm   ", distCm);
        LCD_WriteStringAtPos(strMsg, 2, 0);
        if (distCm > 0 && distCm <= 5) {
            RGBLED_SetValue(0xFF, 0x00, 0x00);
        } else if (distCm > 5 && distCm <= 10) {
            RGBLED_SetValue(0xFF, 0xFF, 0x00);
        } else if (distCm > 10 && distCm <= 50) {
            RGBLED_SetValue(0x00, 0xFF, 0x00);
        } else if (distCm > 50) {
            RGBLED_SetValue(0x00, 0x00, 0xFF);
        }

        // Maneuvering Blocks
        if (mode == 1) {
            if ((start == 0 && (!L && !Ml && !Mr && !R) && distCm > 16) || backUp == 1) { // Start
                SRV_SetPulseMicroseconds0(955); // Right Servo Forward
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                stopTimer = 0; // start timer
                start = 1;
                right = 1;

            } else if (right == 1 && distCm < 15 && turn == 0) { // TURN-R) 
                for (counters = 0; counters < 35; counters++) {
                    for (is = 0; is < 300; is++) {
                        SRV_SetPulseMicroseconds0(2100); // Right Servo Reverse
                        SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                    }
                }

                SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                SRV_SetPulseMicroseconds1(1525); // Left Servo Stop
                turn = 1;
                right = 0;
                left = 1;
                finish = 1;

            } else if (left == 1 && distCm < 15 && turn == 0) { // TURN-L
                for (counters = 0; counters < 35; counters++) {
                    for (is = 0; is < 300; is++) {
                        SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                        SRV_SetPulseMicroseconds1(900); // Left Servo Reverse
                    } //software delay ~1 millisec
                }

                SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                SRV_SetPulseMicroseconds1(1525); // Left Servo Stop
                turn = 1;
                right = 1;
                left = 0;
                finish = 1;

            } else if (right == 0 && turn == 1) { // FORWARD after TURN-R
                SRV_SetPulseMicroseconds0(955); // Right Servo Forward
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                turn = 0;
                rLine = 1;

            } else if (left == 0 && turn == 1) { // FORWARD after TURN-L
                SRV_SetPulseMicroseconds0(955); // Right Servo Forward
                SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                turn = 0;
                lLine = 1;

            } else if (turn == 1 && right == 0 && rLine == 1) { // CENTER-R
                if (!L) {
                    for (counters = 0; counters < 35; counters++) {
                        for (is = 0; is < 300; is++) {
                            SRV_SetPulseMicroseconds0(900); // Right Servo Forward
                            SRV_SetPulseMicroseconds1(900); // Left Servo Reverse
                        }
                    }
                }

                if (distCm > 15) {
                    SRV_SetPulseMicroseconds0(955); // Right Servo Forward
                    SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                } else if (distCm < 15) {
                    SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                    SRV_SetPulseMicroseconds1(1525); // Left Servo Stop
                    rLine = 0;
                }
            } else if (turn == 1 && left == 0 && lLine == 1) { // CENTER-L
                if (!R) {
                    for (counters = 0; counters < 35; counters++) {
                        for (is = 0; is < 300; is++) {
                            SRV_SetPulseMicroseconds0(2100); // Right Servo Reverse
                            SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                        } //software delay ~1 millisec
                    }
                }
                if (distCm > 15) {
                    SRV_SetPulseMicroseconds0(955); // Right Servo Forward
                    SRV_SetPulseMicroseconds1(2100); // Left Servo Forward
                } else if (distCm < 15) {
                    SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                    SRV_SetPulseMicroseconds1(1525); // Left Servo Stop
                    lLine = 0;
                }
            } else if (finish == 1 && (!L && !Ml && !Mr && !R)) { // Finish Line
                SRV_SetPulseMicroseconds0(1530); // Right Servo Stop
                SRV_SetPulseMicroseconds1(1525); // Left Servo Stop
                stopTimer = 1;
                celebrate = 1;
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
                
                LED_SetGroupValue(0b00000000);
            } else if (mode == 1) {
                mode = 0;
                
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
