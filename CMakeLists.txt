# Требуемая минимальная версия CMake
cmake_minimum_required(VERSION 3.14)

# Название проекта и язык
project(LDF_TimeSeries_Cutter LANGUAGES CXX)

# Установка стандарта C++17 (необходим для std::filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Статическая линковка стандартных библиотек C/C++ ---
# Это ВАЖНО для решения проблемы "Entry Point Not Found", особенно с MSVC
# и помогает сделать исполняемый файл более самодостаточным для MinGW.
if(MSVC)
    # Для компилятора MSVC (Visual Studio):
    # Используем переменную CMake для управления типом линковки Runtime Library.
    # MultiThreaded -> /MT (статическая Release)
    # MultiThreadedDebug -> /MTd (статическая Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Примечание: Этот способ предпочтительнее ручной замены флагов CMAKE_CXX_FLAGS_...
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Для компилятора GCC (MinGW):
    # Добавляем флаги линковщику для статической линковки библиотек GCC и C++.
    # Это не всегда гарантирует полную статичность (например, winpthreads может остаться динамическим),
    # но часто решает проблемы с отсутствующими DLL стандартной библиотеки.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    message(STATUS "Attempting static linking for MinGW (-static-libgcc -static-libstdc++)")
else()
    # Для других компиляторов (например, Clang) статическая линковка может требовать других флагов.
    message(WARNING "Static linking configuration not explicitly set for this compiler.")
endif()


# --- Зависимости (исходники должны лежать в папке libs/) ---

# GLFW (для окна и OpenGL контекста)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# Добавляем как поддиректорию, CMake сам найдет его CMakeLists и скомпилирует библиотеку glfw
add_subdirectory(libs/glfw)

# Dear ImGui (библиотека GUI)
# Находим нужные исходники ImGui
# Убери imgui_demo.cpp, если ты его не используешь в своем проекте
file(GLOB IMGUI_SRC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    # libs/imgui/imgui_demo.cpp # Раскомментируй, если используешь функции из demo
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# ImGuiFileDialog (Диалоги выбора файлов/папок)
file(GLOB IMGUI_FILEDIALOG_SRC libs/ImGuiFileDialog/*.cpp)

# Добавляем папки с заголовочными файлами в пути поиска компилятора
include_directories(
    libs/imgui          # Основные заголовки ImGui
    libs/imgui/backends # Заголовки бэкендов ImGui
    libs/glfw/include   # Заголовки GLFW (нужны для imgui_impl_glfw)
    libs/ImGuiFileDialog # Заголовки ImGuiFileDialog
)

# --- Исходники твоей программы ---
# Находим все .cpp файлы в папке src/
file(GLOB APP_SRC src/*.cpp)


# --- Создание исполняемого файла ---
add_executable(${PROJECT_NAME}
    ${APP_SRC}            # Твой main.cpp
    ${IMGUI_SRC}          # Исходники ImGui
    ${IMGUI_FILEDIALOG_SRC} # Исходники ImGuiFileDialog
)


# --- Линковка библиотек ---
# Связываем наш исполняемый файл с:
# 1. Библиотекой 'glfw', которую мы собрали из поддиректории.
# 2. Системной библиотекой OpenGL. В Windows это обычно OpenGL32.lib (для MSVC)
#    или libopengl32.a (для MinGW). CMake обычно находит ее сам.
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL32)


# --- Настройки для Windows ---
if(WIN32)
    # 1. Тип приложения: оконное (WIN32_EXECUTABLE)
    #    Это убирает черное окно консоли, которое появляется при запуске GUI-приложения.
    # set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)

    # 2. Опции компилятора для поддержки UTF-8 в исходниках и строковых литералах.
    if(MSVC)
        # Флаг /utf-8 для компилятора MSVC
        target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
        message(STATUS "Using MSVC compiler flag /utf-8")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # Флаги -finput-charset=UTF-8 и -fexec-charset=UTF-8 для компилятора GCC (MinGW)
        target_compile_options(${PROJECT_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
        message(STATUS "Using MinGW/GCC compiler flags for UTF-8")
    else()
        message(WARNING "UTF-8 compiler options not explicitly set for this compiler.")
    endif()
endif()

# Сообщение в конце конфигурации
message(STATUS "CMake configuration finished for ${PROJECT_NAME}.")