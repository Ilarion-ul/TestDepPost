cmake_minimum_required(VERSION 3.21)

# Знайти саме Qt6 з потрібними модулями
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Widgets Charts)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# WIN32 дає GUI-підсистему на Windows (без консольного вікна)
add_executable(SerpentPostGui WIN32
  src/main.cpp
  src/MainWindow.h src/MainWindow.cpp
  src/DepBridge.h   src/DepBridge.cpp
  src/ParamTableModel.h src/ParamTableModel.cpp
)

target_include_directories(SerpentPostGui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Критично: явно лінкуємо Core, Gui, Widgets, Charts (у такому порядку)
target_link_libraries(SerpentPostGui PRIVATE
  serpent_core
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Charts
)
