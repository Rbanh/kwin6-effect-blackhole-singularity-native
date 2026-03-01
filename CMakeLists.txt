cmake_minimum_required(VERSION 3.16.0)

project(blackhole_singularity_native VERSION 0.1.0)

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.6.0")
set(QT_MAJOR_VERSION 6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose Release or Debug" FORCE)
endif()

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    I18n
    KCMUtils
    WindowSystem
)

find_package(KWin REQUIRED COMPONENTS kwineffects)
find_package(KDecoration3 REQUIRED)
find_package(epoxy REQUIRED)

add_library(blackholesingularity MODULE
    src/main.cpp
    src/blackholesingularityeffect.cpp
)

target_link_libraries(blackholesingularity PRIVATE
    KWin::kwin
    KDecoration3::KDecoration
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    KF6::ConfigGui
    epoxy::epoxy
)

set_target_properties(blackholesingularity PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kwin/effects/plugins"
)

install(TARGETS blackholesingularity
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins
)

add_library(kwin_blackholesingularity_config MODULE
    src/blackholesingularityconfig.cpp
)

target_link_libraries(kwin_blackholesingularity_config PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KCMUtils
)

set_target_properties(kwin_blackholesingularity_config PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kwin/effects/configs"
)

target_compile_definitions(kwin_blackholesingularity_config PRIVATE
    KPLUGINFACTORY_PLUGIN_CLASS_INTERNAL_NAME=kwin_blackholesingularity_config_factory
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

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
