import os
import re

def extract_define_value(filename, define_name):
    with open(filename, 'r') as file:
        content = file.read()
    
    # Отладочная информация: вывод содержимого файла
    # print("File content:")
    # print(content)
    
    # Используем регулярное выражение для поиска строки с определением
    match = re.search(r'#define\s+' + re.escape(define_name) + r'\s+0x([0-9A-Fa-f]+)', content)
    if match:
        # Преобразуем найденное значение из hex в integer
        return int(match.group(1), 16)
    else:
        raise ValueError(f"Define {define_name} not found in {filename}")

def after_build(source, target, env):
    print("Running after_build script...")
    
    firmware_path = str(source[0].path if hasattr(source[0], 'path') else source[0])
    
    if not os.path.isfile(firmware_path):
        print(f"Error: Firmware file not found at {firmware_path}")
        env.Exit(1)
    
    # Путь к файлу main.cpp в подпапке src, предполагая текущий рабочий каталог
    project_dir = os.getcwd()  # Получаем текущий рабочий каталог
    main_cpp_path = os.path.join(project_dir, 'src', 'main.cpp')
    
    print(f"Looking for main.cpp at {main_cpp_path}")
    
    try:
        # Извлекаем значения из main.cpp
        start_address = 0x08000000  # начальный адрес (жестко задан)
        internal_flash_start_address = extract_define_value(main_cpp_path, 'INTERNAL_FLASH_START_ADDRESS')
        
        max_size = internal_flash_start_address - start_address
        
        firmware_size = os.path.getsize(firmware_path)

        print(f"Firmware size: {firmware_size} bytes")
        print(f"Maximum allowed size: {max_size} bytes")

        if firmware_size > max_size:
            print(f"Error: Firmware size {firmware_size} exceeds maximum allowed size of {max_size} bytes. disable functions that exceed the maximum size, or increase the value of INTERNAL_FLASH_START_ADDRESS")
            env.Exit(1)

    except Exception as e:
        print(f"Error: {e}")
        env.Exit(1)

Import("env")

def check_bootdisplay(source, target, env):
    if "BOOTDISPLAY" not in env['BUILD_FLAGS']:
        env.Append(
            LIB_IGNORE=[
                "TFT_eSPI"
            ]
        )

env.AddPostAction("buildprog", check_bootdisplay)
env.AddPostAction("buildprog", after_build)
