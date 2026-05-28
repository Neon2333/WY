cmake_minimum_required(VERSION 3.16)
project(DiffusionFlow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 Qt 的自动工具（重要！）
set(CMAKE_AUTOMOC ON)   # 处理 Q_OBJECT 宏 - 解决当前错误
set(CMAKE_AUTOUIC ON)   # 处理 .ui 文件
set(CMAKE_AUTORCC ON)   # 处理 .qrc 资源文件

# 查找 Qt6 组件
#find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

# 如果有其他 Qt 模块，在这里添加，例如：
# find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# 链接 Qt6 库
target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::Core
    Qt6::SerialPort
    # 如果使用其他模块，也在这里添加，例如：
    # Qt5::Gui
)


if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        # Debug 时：不启用 WIN32_EXECUTABLE（弹出控制台）
        # Release 时：启用 WIN32_EXECUTABLE（不弹出控制台）
        WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
    )
endif()