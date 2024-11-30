#pragma once

#if defined(SKIPR_BOARD)
    #define SD_CS_PIN PC9
    #define SOFT_MISO_PIN PC11
    #define SOFT_MOSI_PIN PC12
    #define SOFT_SCK_PIN PC10
    #define SD_DET PC4

    #define INTERNAL_FLASH_START_ADDRESS 0x0800C000

    #ifdef BOOTDISPLAY
        #define TFT_BL   PE11            // LED back-light control pin
        #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)
        #define TFT_MISO  PA6  // Automatically assigned with ESP8266 if not defined
        #define TFT_MOSI  PA7  // Automatically assigned with ESP8266 if not defined
        #define TFT_SCLK  PA5  // Automatically assigned with ESP8266 if not defined

        #define TFT_CS    PE15  // Chip select control pin D8
        #define TFT_DC    PE7  // Data Command control pin
        #define TFT_RST   PD10  // Reset pin (could connect to NodeMCU RST, see next line)
        #define SPI_FREQUENCY  80000000
        #define SPI_READ_FREQUENCY  20000000
        #define SPI_TOUCH_FREQUENCY  2500000
        #define TOUCH_CS PD9     // Chip select pin (T_CS) of touch screen

        #ifdef BOOTDISPLAY == MKS_TS35
        #define ST7796_DRIVER
            #ifdef SHUI
                #define BASE_SCREEN_HEIGHT 480
                #define BASE_SCREEN_WIDTH 320
            #elif
                #if defined(ROTATION) && (ROTATION == 0 || ROTATION == 2)
                    #define BASE_SCREEN_HEIGHT 480
                    #define BASE_SCREEN_WIDTH 320
                #else
                    #define BASE_SCREEN_HEIGHT 320
                    #define BASE_SCREEN_WIDTH 480
                #endif
            #endif
        #endif
    #endif

    #define BEEPER PB2
    
    #ifdef DEBUG  
        #define DEBUG_RX PB11// включение отладки через serial 
        #define DEBUG_TX PB10
        #define SERIAL_BAUD 115200   
    #else
        #define NDEBUG
    #endif
#endif