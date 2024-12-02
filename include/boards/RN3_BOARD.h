#pragma once

#ifdef RN3_BOARD

    #ifdef BOOTLOADER
        #define INTERNAL_FLASH_START_ADDRESS 0x0800C000
    #endif

    #ifdef WIFI
    #define SERIAL_PORT 2
    #define RST_PIN PE9
    #define GPIO0_PIN PC13
    #define WIFIRX PA10
    #define WIFITX PA9
    #define WIFIBAUD 115200
    #define FLASH_SIZE 1048576  // Размер флеш-памяти ESP8266 (например, 1 МБ)
    #define READ_BLOCK_SIZE 256 // Размер блока чтения за один раз
    #endif

    #define SDLOG
    #define SD_CS_PIN PC9
        #define SOFT_MISO_PIN PC11
        #define SOFT_MOSI_PIN PC12
        #define SOFT_SCK_PIN PC10
        #define SD_DET PD12

        #ifdef BOOTDISPLAY
            #define TFT_BL   PD13            // LED back-light control pin
            #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)
            #define TFT_MISO  PA6  // Automatically assigned with ESP8266 if not defined
            #define TFT_MOSI  PA7  // Automatically assigned with ESP8266 if not defined
            #define TFT_SCLK  PA5  // Automatically assigned with ESP8266 if not defined
            #define TFT_CS    PD11  // Chip select control pin D8
            #define TFT_DC    PD10  // Data Command control pin
            #define TFT_RST   PC6  // Reset pin (could connect to NodeMCU RST, see next line)
            #define SPI_FREQUENCY  80000000
            #define SPI_READ_FREQUENCY  20000000
            #define SPI_TOUCH_FREQUENCY  2500000
            #define TOUCH_CS PE14     // Chip select pin (T_CS) of touch screen

            #if BOOTDISPLAY == MKS_TS35
                #define ST7796_DRIVER
                #ifdef SHUI
                    #define setaddr 0
                    #define BASE_SCREEN_HEIGHT 480
                    #define BASE_SCREEN_WIDTH 320
                #else
                    #if defined(ROTATION) && (ROTATION == 0 || ROTATION == 2)
                        #define BASE_SCREEN_HEIGHT 480
                        #define BASE_SCREEN_WIDTH 320
                    #else
                        #define BASE_SCREEN_HEIGHT 320
                        #define BASE_SCREEN_WIDTH 480
                    #endif
                #endif
                        #define BASE_ROTATION 1
                        // Определяем отступы
                        #define TOP_MARGIN    10
                        #define BOTTOM_MARGIN 50
                        #define LEFT_MARGIN   10
                        #define LINE_HEIGHT   10 // Определяем высоту строки
            #endif
            
        #endif

        #ifdef SPIFLASH
            #define CUSTOM_CS   PB12
            #define csPin PB12
            #define clkPin PB13
            #define diPin PC3
            #define doPin PC2
        #endif

        #define BEEPER PC5

        #ifdef BEEPER
            #define BASE_BEEPSTATE 1
        #endif

        #ifdef POWERHOLD
        #define HOLD_PIN PB2
        #endif

        #ifdef DEBUG  
            #define DEBUG_RX PB11// включение отладки через serial 
            #define DEBUG_TX PB10
            #define SERIAL_BAUD 115200   
        #else
            // #define NDEBUG
        #endif
#endif