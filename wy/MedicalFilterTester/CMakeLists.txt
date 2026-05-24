cmake_minimum_required(VERSION 3.10)
project(MedicalFilterTester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(Qt6_DIR "D:/Qt/6.8.1/mingw_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
)

set(SOURCES
    src/main.cpp
    src/LoginWindow.cpp
    src/MainWindow.cpp
    src/VirtualKeyboard.cpp
    src/MessageDialog.cpp
    src/RoundedProgressBar.cpp
    src/Database/DatabaseManager.cpp
    src/Pages/ParamInputPage.cpp
    src/Pages/TestSelectionPage.cpp
    src/Pages/DiffusionFlowTestPage.cpp
    src/Pages/HistoryPage.cpp
    src/Export/ExcelExporter.cpp
    3rdParty/keyboard/frminput.cpp
)

set(HEADERS
    include/LoginWindow.h
    include/MainWindow.h
    include/VirtualKeyboard.h
    include/MessageDialog.h
    include/RoundedProgressBar.h
    include/DatabaseManager.h
    include/Pages/ParamInputPage.h
    include/Pages/TestSelectionPage.h
    include/Pages/DiffusionFlowTestPage.h
    include/Pages/HistoryPage.h
    include/Export/ExcelExporter.h
    3rdParty/keyboard/frminput.h
)

set(UI_FILES
    3rdParty/keyboard/frminput.ui
)

set(RESOURCES
    res/icon.qrc
    res/Image.qrc
    res/ChineseLib.qrc
    res/ChinesePhraseLib.qrc
    res/Font.qrc
    res/GoogleChineseLib.qrc
    res/translations.qrc
)

add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Pages
    ${CMAKE_SOURCE_DIR}/include/Export
    ${CMAKE_SOURCE_DIR}/include/Database
    ${CMAKE_SOURCE_DIR}/3rdParty/keyboard
)

set(Qt6_BIN_DIR "D:/Qt/6.8.1/mingw_64/bin")
set(Qt6_PLUGINS_DIR "D:/Qt/6.8.1/mingw_64/plugins")
set(MINGW_BIN_DIR "D:/Qt/Tools/mingw1310_64/bin")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Qt6_BIN_DIR}/Qt6Core.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Qt6_BIN_DIR}/Qt6Gui.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Qt6_BIN_DIR}/Qt6Widgets.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Qt6_BIN_DIR}/Qt6Sql.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Qt6_BIN_DIR}/Qt6Core5Compat.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        "${CMAKE_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
        "${CMAKE_BINARY_DIR}/"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${Qt6_PLUGINS_DIR}/platforms"
        "${CMAKE_BINARY_DIR}/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${Qt6_PLUGINS_DIR}/sqldrivers"
        "${CMAKE_BINARY_DIR}/sqldrivers"
)
