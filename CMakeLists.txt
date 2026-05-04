cmake_minimum_required(VERSION 3.22)
project(PerlinNoise LANGUAGES C CXX)

# Define C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option: try to use system SFML first. If OFF, CMake will fetch and build SFML as a subproject.
option(USE_SYSTEM_SFML "Use system-installed SFML (don't download/build SFML)" ON)

if(USE_SYSTEM_SFML)
    find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
    )
    FetchContent_MakeAvailable(sfml)
endif()

# Create executable
add_executable(PerlinNoise main.cpp)

# ==============================================================================
# OTIMIZAÇÃO DE HARDWARE E MULTITHREADING
# ==============================================================================

# 1. Procura pela biblioteca OpenMP no sistema do usuário
find_package(OpenMP REQUIRED)

# 2. Configura flags de otimização agressiva caso o compilador seja GCC ou Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(PerlinNoise PRIVATE
        -O3             # Ativa Vetorização (SIMD) e Loop Unrolling
        -ffast-math     # Relaxa a precisão IEEE-754 para acelerar trigonometria e decimais
    )

    # Define a macro NDEBUG para remover asserções lentas e bounds-checking do std::vector
    target_compile_definitions(PerlinNoise PRIVATE NDEBUG)
endif()

# 3. Faz a linkagem da biblioteca de multithreading ao seu executável
target_link_libraries(PerlinNoise PRIVATE OpenMP::OpenMP_CXX)

# ==============================================================================
# LINKAGEM DO SFML
# ==============================================================================

# Link libraries (SFML 3 uses imported targets named either SFML::Graphics or sfml::Graphics)
if (TARGET SFML::Graphics)
    target_link_libraries(PerlinNoise PRIVATE SFML::Graphics SFML::Window SFML::System)
elseif (TARGET sfml::Graphics)
    target_link_libraries(PerlinNoise PRIVATE sfml::Graphics sfml::Window sfml::System)
else()
    message(FATAL_ERROR "SFML targets not found. Install SFML system-wide (set -DUSE_SYSTEM_SFML=ON) or allow CMake to fetch it (set -DUSE_SYSTEM_SFML=OFF)")
endif()

# Install rule (optional)
install(TARGETS PerlinNoise RUNTIME DESTINATION bin)
