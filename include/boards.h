#pragma once
// #ifndef BOARDS_H
// #define BOARDS_H
#define BLVER  "FWBL v0.2 BETA"
// Определение платформы и соответствующих пинов
// #define POWERHOLD

// Включаем соответствующий файл платы
#ifdef RN3_BOARD
#include "boards/RN3_BOARD.h"
#elif defined(SKIPR_BOARD)
#include "boards/SKIPR_BOARD.h"
#elif defined(GENERIC_BOARD)
#include "boards/GENERIC_BOARD.h"
#else
#error "No board defined!"
#endif

#ifdef BOOTLOADER
    // #define WIFI
    // #define DEBUG
    #define WIFI_FILE "esp8266.bin"
    #define WIFIBACKUP "wifibackup.bin"
    #define WIFI_CUR "esp8266.cur"
    #define FIRMWARE_FILE "firmware.bin"
    #define PREFLASH "preloader.bin"
    #define LOG_FILE "flashlog.txt"
    #define PREFLASH_CURRENT "preloader.CUR"
    #define FIRMWARE_CURRENT "firmware.CUR"
    #define BACKUPNAME "backup.bin"
    #define BACKUP_CURRENT "backup.CUR"

    #ifndef INTERNAL_FLASH_START_ADDRESS && !defined(USBSERDBG) 
        #define INTERNAL_FLASH_START_ADDRESS 0x0800C000 //standart MKS 48KiB offset
        #warning "looks like your config contain errors. you need to define INTERNAL_FLASH_START_ADDRESS in board config!"
    #elif !defined(INTERNAL_FLASH_START_ADDRESS) && defined(USBSERDBG)
        #define INTERNAL_FLASH_START_ADDRESS 0x08010000 //64KiB offset for USB debug
    #endif

    #if defined (USBSERDBG) && defined(INTERNAL_FLASH_START_ADDRESS)
        #undef INTERNAL_FLASH_START_ADDRESS
        #define INTERNAL_FLASH_START_ADDRESS (INTERNAL_FLASH_START_ADDRESS + 0x4000) // добавляем 16 КБ

        #undef PREFLASH
        #define PREFLASH ""

        // Создание предупреждения с новым значением
        #define STR_HELPER(x) #x
        #define STR(x) STR_HELPER(x)
        #warning New INTERNAL_FLASH_START_ADDRESS: STR(INTERNAL_FLASH_START_ADDRESS)
    #endif


    #define VERMARKER 0x0800BFF0
    #define INTERNAL_FLASH_END_ADDRESS   0x080FFFFF // Адрес конца флеш-памяти для STM32F407VET6
    // #define INTERNAL_FLASH_START_ADDRESS 0x08008000  // адрес начала памяти для прошивки после 32 КБ загрузчика
    // в режиме build также выполняется проверка размера загрузчика, по умолчанию ограничение 32кб
    // #define INTERNAL_FLASH_START_ADDRESS 0x08000000 
#else 
    #ifdef USBSERDBG
    #warning "don't try to update bootloader with stock preloader offset and usb debug! it will broke your board!!! add 0x4000 to board_upload.offset_address in pio.ini"
    #endif

    #define VERMARKER 0x0800BFF0
    #define FIRMWARE_FILE "bootloader.bin"
    #define LOG_FILE "bootup.txt"
    #define FIRMWARE_CURRENT "bootloader.cur"
    #define INTERNAL_FLASH_START_ADDRESS 0x08000000
    // #define APPLICATION_ADDRESS 0x0800C000
    #define INTERNAL_FLASH_END_ADDRESS   0x0800C000 // Адрес конца флеш-памяти для STM32F407VET6
#endif

#define FLASH_SECTOR_SIZE 16384
#define FLASH_SECTOR_SIZE_16K         16384       // Размер сектора 16 КБ
#define FLASH_SECTOR_SIZE_64K         65536       // Размер сектора 64 КБ
#define FLASH_SECTOR_SIZE_128K        131072      // Размер сектора 128 КБ
#define BUFFER_SIZE 256         // Размер буфера

#ifdef BOOTDISPLAY
#define STM32
#define USER_SETUP_INFO "User_Setup"
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
// #define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
// #define SMOOTH_FONT
#endif


#if defined(USBSERDBG) && !defined(USBD_USE_CDC)
    #error "USBSERDBG is defined, but USBD_USE_CDC is missing!"
#endif

#if defined(USBSERDBG) && !defined(USBCON)
    #error "USBSERDBG is defined, but USBCON is missing!"
#endif

#if !defined(USBSERDBG) && (defined(USBD_USE_CDC) || defined(USBCON))
    #error "USBSERDBG is not defined, but USBD_USE_CDC or USBCON is defined!"
#endif

// #endif // BOARDS_H
