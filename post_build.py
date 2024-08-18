import os
import shutil
import zipfile

def rename_and_zip(working_directory):
    # Путь к папке сборки
    build_dir = os.path.join(working_directory, ".pio", "build")
    
    # Путь к файлам
    firmware_path = os.path.join(build_dir, "firmware.bin")
    bootloader_path = os.path.join(build_dir, "bootloader.bin")
    preloader_path = os.path.join(build_dir, "preloader.bin")
    
    # Отладочные сообщения
    print(f"Working directory: {working_directory}")
    print(f"Build directory: {build_dir}")
    print(f"Firmware path: {firmware_path}")

    if os.path.exists(firmware_path):
        # Переименовать firmware.bin в bootloader.bin и preloader.bin
        shutil.copy(firmware_path, bootloader_path)
        shutil.copy(firmware_path, preloader_path)

        # Создание архива FWBL.zip
        fwbl_zip_path = os.path.join(working_directory, "FWBL.zip")
        with zipfile.ZipFile(fwbl_zip_path, 'w') as zipf:
            zipf.write(firmware_path, 'firmware.bin')
        print(f"Created FWBL.zip with firmware.bin")

        # Временное переименование preloader.bin в Robin_nano_v3.bin
        robin_nano_path = os.path.join(build_dir, "Robin_nano_v3.bin")
        shutil.copy(preloader_path, robin_nano_path)

        # Создание архива updater.zip
        updater_zip_path = os.path.join(working_directory, "updater.zip")
        with zipfile.ZipFile(updater_zip_path, 'w') as zipf:
            zipf.write(firmware_path, 'firmware.bin')
            zipf.write(robin_nano_path, 'Robin_nano_v3.bin')
        print(f"Created updater.zip with firmware.bin and Robin_nano_v3.bin")

        # Удаление временных файлов
        os.remove(robin_nano_path)
        print(f"Removed temporary file: {robin_nano_path}")
    else:
        print(f"Firmware file not found: {firmware_path}")

if __name__ == "__main__":
    rename_and_zip(os.getcwd())
