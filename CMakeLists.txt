cmake_minimum_required(VERSION 3.16)
project(DeskMuteOverlay VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg SvgWidgets)

include(FetchContent)
FetchContent_Declare(
        qhotkey
        GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
        GIT_TAG master
)
FetchContent_MakeAvailable(qhotkey)

include_directories(include)

add_executable(DeskMuteOverlay MACOSX_BUNDLE WIN32
        src/main.cpp
        src/DiscordClient.cpp
        src/MemberWidget.cpp
        src/OverlayWindow.cpp
        src/ConfigWindow.cpp
        include/DiscordClient.h
        include/MemberWidget.h
        include/OverlayWindow.h
        include/ConfigWindow.h
        resources/resources.qrc
)

set_target_properties(DeskMuteOverlay PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST_MAKE_LSUIELEMENT ON
)

target_link_libraries(DeskMuteOverlay PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Svg Qt6::SvgWidgets qhotkey
)