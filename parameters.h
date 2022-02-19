#ifndef FLASH_PARAMTER_H
#define FLASH_PARAMETER_H

#include "pico/stdlib.h"



#define UART_ID uart0
#define UART_TX_PIN 0       // PIN1
#define UART_RX_PIN 1       // PIN2
#define UART_DIR_PIN 2      // PIN4
#define BAUD_RATE 9600
#define DATA_BITS 8
#define PARITY UART_PARITY_NONE
#define STOP_BITS 1

#define RTU_SETUP_PIN 3
#define FLASH_TARGET_OFFSET (1024*1024)

#define GPIO_ENCODER_A 10   // PIN14
#define GPIO_ENCODER_B 11   // PIN15
#define GPIO_ENCODER_Z 12   // PIN16
#define GPIO_LED 25         // 內部燈
#define GPIO_CAMARA 14      // PIN19
#define GPIO_RESET 15       // PIN20
typedef struct  
{
    uint16_t rtu_id;                 //0
    uint16_t writeFlash;             //1
    uint16_t camaraPos1;             //2
    uint16_t camaraPos2;             //3
    uint16_t camaraPos3;             //4
    uint16_t camaraPos4;             //5
    uint16_t encoderResolution;     //6
    uint16_t WheelLen;              //7
    uint16_t MPM;                   //8
    uint16_t ledDuration;           //9
    uint16_t setup;                 //a
    uint16_t encoderDiff;           //b
    uint16_t LED_warmup_us;         //c
    uint16_t LED_duration_us;       //d
    uint16_t Camara_delay_us;
    uint16_t RPM;                   //e
    uint16_t AQ0;                   //0xf
    uint16_t AQ1;

    uint16_t garbage[128];
} Parameters_t;

bool writeParam(Parameters_t* Param);
bool initFlash(Parameters_t * Param);
#endif
