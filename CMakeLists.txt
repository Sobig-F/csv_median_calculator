cmake_minimum_required(VERSION 3.23)
project(csv_median_calculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURE_DEPENDS true)

# Включаем FetchContent
include(FetchContent)

# Boost через FetchContent
set(BOOST_INCLUDE_LIBRARIES
    program_options
    filesystem
    interprocess
    regex
    algorithm
    lexical_cast
    accumulators
)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG        boost-1.84.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Boost)

# toml++
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.x
)
FetchContent_MakeAvailable(spdlog)


# Исходные файлы
file(GLOB SRC "src/*.cpp")
add_executable(csv_median_calculator
    ${SRC}
)

# Подключаем заголовочные файлы
target_include_directories(csv_median_calculator PRIVATE
    headers
)

# Линкуем библиотеки
target_link_libraries(csv_median_calculator PRIVATE
    Boost::program_options
    Boost::filesystem
    Boost::regex
    Boost::interprocess
    Boost::algorithm
    Boost::lexical_cast
    Boost::accumulators
    tomlplusplus::tomlplusplus
    spdlog::spdlog
)

# Опции компиляции
if(MSVC)
    # target_compile_options(csv_median_calculator PRIVATE -g)
    target_compile_options(csv_median_calculator PRIVATE /W4
        $<$<CONFIG:Debug>:-g>  # только для Debug
    )
    target_compile_options(csv_median_calculator PRIVATE /W4)
else()
    target_compile_options(csv_median_calculator PRIVATE -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-g>  # только для Debug
    )
    target_compile_options(csv_median_calculator PRIVATE -Wall -Wextra -Wpedantic)
endif()