cmake_minimum_required(VERSION 3.10)
project(MyOpenGLProject)

# Установка стандарта C++ (например, C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Включение директорий с заголовочными файлами
include_directories(include)
include_directories(include/GLFW)
include_directories(include/KHR)
include_directories(include/imgui)
include_directories(include/implot)
include_directories(src)
# Добавление исходных файлов вашего проекта
set(SOURCES
    src/main.cpp
    src/glad.c

    src/imgui/imgui_demo.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_impl_glfw.cpp
    src/imgui/imgui_impl_opengl3.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui.cpp

    src/implot/implot.cpp
    src/implot/implot_items.cpp

)

# Добавление исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка OpenGL (если GLFW не линкует его автоматически)
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

# Линковка GLFW (используем предварительно скомпилированную библиотеку)
target_link_libraries(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/libs/glfw/glfw3.lib
)

# Линковка GLM (заголовочная библиотека, линковка не требуется)
# Просто убедитесь, что include_directories указан правильно

# Линковка GLAD (уже включен в исходники)
# Ничего дополнительного не требуется, так как glad.c уже добавлен в SOURCES