/**
 * @file Pins.h
 * @author Etienne Montenegro
 * @brief File containing all the pins definition for the MisBKit project 
 
 ref : https://makeabilitylab.github.io/physcomp/esp32/esp32.html#huzzah32-pin-diagram
 
Any pin on ADC2 cant be used if wifi is started.
This affects GPIOs 2,12-15 and 25-27.

 they are still available to the other function of the pins, they just can't be internally connected to the ADC2 module and used for analog to digital conversions. 

ref : https://electronics.stackexchange.com/questions/470331/esp32-can-i-safely-use-gpio-adc2-pins-for-non-adc-use-while-using-wifi


 * ESP32: Only pins that support both input & output have integrated pull-up and pull-down resistors.
 * ref : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html?highlight=gpio%20pull%20up

*/


#ifndef PINS_DEF_H
#define PINS_DEF_H
#include <Arduino.h>


namespace pins{

   // For some reason, A12 is not defined. So, using raw literal 13 instead
    // ADC#1 includes A2/34, A3/39, A4/36, A7/32, A9/33
    // ADC#2 includes A0/26, A1/25, A5/4, A6/14, A8/15, A10/27, A11/12, and A12/13
#if defined(ARDUINO_FEATHER_ESP32)
    //pin to control direction of communication with the motors
    const uint8_t motorsControl{4}; // A10 - ADC2
    
     // battery voltage reading
    const uint8_t battery{34}; // A4/36 - ADC1

    const uint8_t led{21}; //pin to control RGB led that display kit status
    const uint8_t clear{14};//button to clear eeprom

    const uint8_t analogSensor1{39};
    const uint8_t analogSensor2{33};
    const uint8_t analogSensor3{32};

    const uint8_t digitalSensor1{12}; // A8 - ADC2
    const uint8_t digitalSensor2{27}; // A6 - ADC2
    const uint8_t digitalSensor3{15}; // Digital IO

    const uint8_t sda{SDA};
    const uint8_t scl{SCL};
    const uint8_t tx{TX};
    const uint8_t rx{RX};
 #elif defined(ARDUINO_M5Stack_StampS3)
    //pin to control direction of communication with the motors
    #ifdef stamps3A
    const uint8_t motorsControl{41}; 
    const uint8_t battery{3}; // battery voltage reading
    #else
    const uint8_t motorsControl{1}; 
    const uint8_t battery{1}; // battery voltage reading
    #endif 
    const uint8_t pwm1{39};
    const uint8_t pwm2{40};
    const uint8_t pwm3{14};
    const uint8_t pwm4{12};
    
    
    
    const uint8_t led{21}; //pin to control RGB led that display kit status
    const uint8_t clear{0};//button to clear eeprom
    


    const uint8_t analogSensor1{5};
    const uint8_t analogSensor2{7};
    const uint8_t analogSensor3{9};
    const uint8_t analogSensor4{11};

    const uint8_t digitalSensor1{4}; 
    const uint8_t digitalSensor2{6}; 
    const uint8_t digitalSensor3{8}; 
    const uint8_t digitalSensor4{10};
    
    const uint8_t sda{SDA};
    const uint8_t scl{SCL};
    const uint8_t tx{TX};
    const uint8_t rx{RX};
    
    
    
#endif

}//namespace pins
#endif