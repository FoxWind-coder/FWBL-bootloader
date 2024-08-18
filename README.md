# FWBL Bootloader
universal open source bootloader for 3d printer motherboards based on STM32F4xx
it is possible to fully customize by adding new boards to the boards.h file, as well as assembling a preloader that can be used to update the main bootloader
. The customization system allows you to run this loader on any motherboards based on stm32f407, as well as, if modified, on other stm32 processors
, this loader has a standard offset of 48 kb (0x0800C000)
by changing the compiler flags in platform.ini, you can choose which board to build for and in which mode

имеется возможность полноценной кастомизации путем добавления новых плат в файл boards.h, а также сборки прелоадера, который можно использовать для обновления основного бутлоадера
система кастомизации позволяет запустить данный загрузчик на любых материнских платах, основанных на stm32f407, а также, при доработке, на других процессорах stm32
данный загрузчик имеет стандартный offset 48 kb (0x0800C000)
путем изменения флагов компилятора в platform.ini вы можете выбрать, для какой платы и в каком режиме выполнять сборку
