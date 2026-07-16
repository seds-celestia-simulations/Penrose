cmake_minimum_required(VERSION 3.22)

# Automatically hook up the local vcpkg toolchain if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Penrose)

# Enforce modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Fetch and build GLFW automatically (Since it's not using vcpkg)
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Find external packages via your local vcpkg
find_package(glm CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

# 3. Compile GLAD manually from the realtime GPU headers
add_library(glad realtime/gpu/glad/glad.c)
target_include_directories(glad PUBLIC realtime/gpu)

# 4. GPU real-time visualization executable
add_executable(Penrose 
    realtime/main.cpp
    realtime/renderer/Texture.cpp
    realtime/renderer/Window.cpp
    realtime/renderer/Renderer.cpp
    realtime/renderer/ParticleBuffer.cpp
    realtime/renderer/RenderTarget.cpp
    realtime/renderer/LutBaker.cpp
    realtime/renderer/AccretionDisk.cpp
    realtime/renderer/Engine.cpp
)

# Tell the executable where to find realtime headers and vendor (stb)
target_include_directories(Penrose PRIVATE realtime vendor)

# 5. Link everything together
target_link_libraries(Penrose PRIVATE
    glfw
    glad
    glm::glm
    Eigen3::Eigen
)

# Copy shaders and resources next to the executable
add_custom_command(TARGET Penrose POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/realtime/shaders"
    "$<TARGET_FILE_DIR:Penrose>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/realtime/resources"
    "$<TARGET_FILE_DIR:Penrose>/resources"
)

# 6. CPU physics validation / benchmark executable
add_executable(benchmark_test
    physics/validation/main_benchmark.cpp
    physics/validation/orbital.cpp
    physics/validation/null_geodesic.cpp
    physics/validation/freefall.cpp
    physics/metrics/CoordinateChart.cpp
    physics/metrics/SchwarzschildMetric.cpp
    physics/geodesics/GeodesicDynamics.cpp
    physics/integrators/RK4Integrator.cpp
    physics/simulation/TrajectorySolver.cpp
    physics/simulation/TerminationPolicy.cpp
)
target_include_directories(benchmark_test PRIVATE physics)
target_compile_definitions(benchmark_test PRIVATE PENROSE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(benchmark_test PRIVATE Eigen3::Eigen)
