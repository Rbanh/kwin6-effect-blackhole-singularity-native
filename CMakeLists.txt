cmake_minimum_required(VERSION 3.20)
project(blackhole_singularity_native VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH "${ECM_MODULE_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6 REQUIRED COMPONENTS CoreAddons Config KCMUtils)
find_package(KWin REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

add_library(blackholesingularity MODULE
    src/main.cpp
    src/blackholesingularityeffect.cpp
)
set_target_properties(blackholesingularity PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/kwin/effects/plugins"
)

target_include_directories(blackholesingularity PRIVATE src)

target_link_libraries(blackholesingularity PRIVATE
    KWin::kwin
    Qt6::Core
    Qt6::Gui
)
install(TARGETS blackholesingularity
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins
)

add_library(kwin_blackholesingularity_config MODULE
    src/blackholesingularityconfig.cpp
)
set_target_properties(kwin_blackholesingularity_config PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/kwin/effects/configs"
)
target_compile_definitions(kwin_blackholesingularity_config PRIVATE
    KPLUGINFACTORY_PLUGIN_CLASS_INTERNAL_NAME=kwin_blackholesingularity_config_factory
)

target_link_libraries(kwin_blackholesingularity_config PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KCMUtils
)
install(TARGETS kwin_blackholesingularity_config
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs
)

qt_add_resources(blackholesingularity "blackhole_shaders"
    PREFIX "/effects/blackhole_singularity"
    BASE src
    FILES
        src/shaders/blackhole.frag
        src/shaders/blackhole_core.frag
)
