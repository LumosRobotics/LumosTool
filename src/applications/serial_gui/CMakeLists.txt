# serial_gui – Qt6 firmware downloader / serial terminal
# Included by the root CMakeLists.txt only when Qt6 is found.

set(SERIAL_GUI_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    flashworker.cpp
    flashworker.h
    serialmonitor.cpp
    serialmonitor.h
)

add_executable(serial_gui ${SERIAL_GUI_SOURCES})

# Enable Qt's code-generation tools for this target only
set_target_properties(serial_gui PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# macOS: produce a proper .app bundle
if(APPLE)
    set_target_properties(serial_gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lumos Firmware Downloader"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.lumos.serial-gui"
    )
endif()

target_link_libraries(serial_gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    lumos_serial
)
