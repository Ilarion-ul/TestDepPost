cmake_minimum_required(VERSION 3.28)
project(SerpentPost VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- сторонній spdlog (header-only) -------------------
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY  https://github.com/gabime/spdlog.git
  GIT_TAG         v1.14.2
)
FetchContent_MakeAvailable(spdlog)

# ---------- бібліотека ядра ----------------------------------
add_library(serpent_core
    src/Logger.cpp   src/Logger.h
    src/DepParser.cpp  src/DepParser.h
    src/CsvWriter.cpp  src/CsvWriter.h
)
target_link_libraries(serpent_core PUBLIC spdlog::spdlog_header_only)
target_include_directories(serpent_core PUBLIC src)

# ---------- консольний приклад -------------------------------
add_executable(SerpentPostCli src/main.cpp)
target_link_libraries(SerpentPostCli PRIVATE serpent_core)
