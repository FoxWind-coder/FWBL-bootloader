#pragma once

#ifdef GENERIC_BOARD
    #define SD_CS_PIN PC11
    #define SOFT_MISO_PIN PC8 //пины карты памяти
    #define SOFT_MOSI_PIN PD2
    #define SOFT_SCK_PIN PC12
    //#define SD_DET PC4
    // SD_DET не определен для generic
    #ifdef DEBUG  
        #define DEBUG_RX PB11// включение отладки через serial 
        #define DEBUG_TX PB10
        #define SERIAL_BAUD 115200   
    #else
        #define NDEBUG
    #endif
    #ifdef BOOTDISPLAY
        #warning "displays not configured. you need do it yourself"
    #endif
#endif