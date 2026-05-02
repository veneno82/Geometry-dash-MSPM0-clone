// LED.c - Runs on MSPM0G3507
// Uses LaunchPad built-in LEDs
// data bit mapping: bit0=PA0(RedLED1,neg logic) bit1=PB22(Blue) bit2=PB26(Red2) bit3=PB27(Green)
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"

#define LED_RED1_PIN   (1UL<<0)   // PA0, negative logic
#define LED_BLUE_PIN   (1UL<<22)  // PB22
#define LED_RED2_PIN   (1UL<<26)  // PB26
#define LED_GREEN_PIN  (1UL<<27)  // PB27

void LED_Init(void){
    IOMUX->SECCFG.PINCM[0]  = 0x00000081; // PA0 GPIO output
    IOMUX->SECCFG.PINCM[49] = 0x00000081; // PB22 GPIO output
    IOMUX->SECCFG.PINCM[56] = 0x00000081; // PB26 GPIO output
    IOMUX->SECCFG.PINCM[57] = 0x00000081; // PB27 GPIO output
    GPIOA->DOE31_0 |= LED_RED1_PIN;
    GPIOB->DOE31_0 |= (LED_BLUE_PIN|LED_RED2_PIN|LED_GREEN_PIN);
    GPIOA->DOUTSET31_0 = LED_RED1_PIN; // PA0 high = LED off (negative logic)
    GPIOB->DOUTCLR31_0 = (LED_BLUE_PIN|LED_RED2_PIN|LED_GREEN_PIN);
}

void LED_On(uint32_t data){
    if(data & 0x01) GPIOA->DOUTCLR31_0 = LED_RED1_PIN;  // neg logic: 0=on
    if(data & 0x02) GPIOB->DOUTSET31_0 = LED_BLUE_PIN;
    if(data & 0x04) GPIOB->DOUTSET31_0 = LED_RED2_PIN;
    if(data & 0x08) GPIOB->DOUTSET31_0 = LED_GREEN_PIN;
}

void LED_Off(uint32_t data){
    if(data & 0x01) GPIOA->DOUTSET31_0 = LED_RED1_PIN;  // neg logic: 1=off
    if(data & 0x02) GPIOB->DOUTCLR31_0 = LED_BLUE_PIN;
    if(data & 0x04) GPIOB->DOUTCLR31_0 = LED_RED2_PIN;
    if(data & 0x08) GPIOB->DOUTCLR31_0 = LED_GREEN_PIN;
}

void LED_Toggle(uint32_t data){
    if(data & 0x01) GPIOA->DOUTTGL31_0 = LED_RED1_PIN;
    if(data & 0x02) GPIOB->DOUTTGL31_0 = LED_BLUE_PIN;
    if(data & 0x04) GPIOB->DOUTTGL31_0 = LED_RED2_PIN;
    if(data & 0x08) GPIOB->DOUTTGL31_0 = LED_GREEN_PIN;
}
