#include <Arduino.h>
#include <LiquidCrystal.h>

extern "C" {
    #include "pb_decode.h"
    #include "pb_encode.h"
    #include "lcdkeypad.pb.h"
}

// Select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// Read the buttons
int read_LCD_buttons() {
    // Read the value from the sensor 
    adc_key_in = analogRead(0);

    // Map buttons (v1.0 hardware)
    if (adc_key_in > 1000) return btnNONE;
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 195)  return btnUP; 
    if (adc_key_in < 380)  return btnDOWN; 
    if (adc_key_in < 555)  return btnLEFT; 
    if (adc_key_in < 790)  return btnSELECT;   

    return btnNONE;  // when all others fail, return this...
}

void setup() {
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Push the buttons");
}

void loop() {
    // Move cursor to second line "1" and 9 spaces over
    lcd.setCursor(9, 1);            

    // Display seconds elapsed since power-up
    lcd.print(millis()/1000);      

    // Move to the start of the second line
    lcd.setCursor(0, 1);         

    // Read the buttons
    lcd_key = read_LCD_buttons();

    // Print button status
    switch (lcd_key) {
        case btnRIGHT:
            lcd.print("RIGHT ");
            break;
        case btnLEFT:
            lcd.print("LEFT   ");
            break;
        case btnUP:
            lcd.print("UP    ");
            break;
        case btnDOWN:
            lcd.print("DOWN  ");
            break;
        case btnSELECT:
            lcd.print("SELECT");
            break;
        case btnNONE:
            lcd.print("NONE  ");
            break;
    }
}