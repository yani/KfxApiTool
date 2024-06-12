cmake_minimum_required(VERSION 3.5)

project(KfxApiTool VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        main.cpp
        kfx_api_tool.cpp
        kfx_api_tool.h
        kfx_api_tool.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(KfxApiTool
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        about_dialog.h about_dialog.cpp about_dialog.ui
        connect_dialog.h connect_dialog.cpp connect_dialog.ui
        subscribe_variable_dialog.h subscribe_variable_dialog.cpp subscribe_variable_dialog.ui
        subscribed_variable_widget.h subscribed_variable_widget.cpp
        q_kfx_variable_completer.h
        subscribe_event_dialog.h subscribe_event_dialog.cpp subscribe_event_dialog.ui
        subscribed_event_widget.h subscribed_event_widget.cpp
        add_command_dialog.h add_command_dialog.cpp add_command_dialog.ui
        command_widget.h command_widget.cpp
        edit_command_dialog.h edit_command_dialog.cpp edit_command_dialog.ui
        set_variable_dialog.h set_variable_dialog.cpp set_variable_dialog.ui
        q_kfx_set_variable_completer.h

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET KfxApiTool APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(KfxApiTool SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(KfxApiTool
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(KfxApiTool PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.KfxApiTool)
endif()
set_target_properties(KfxApiTool PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS KfxApiTool
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(KfxApiTool)
endif()
