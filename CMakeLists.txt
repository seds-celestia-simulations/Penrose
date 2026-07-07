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

# 3. Compile GLAD manually from your local include/src folders
add_library(glad include/glad/glad.c)
target_include_directories(glad PUBLIC include)

# 4. Setup your main executable
add_executable(Penrose 
    src/main.cpp
    src/render/Texture.cpp
    src/render/Window.cpp
    src/render/Renderer.cpp
    src/render/ParticleBuffer.cpp
    src/render/RenderTarget.cpp
)

# Tell your executable where to find the local include folder (for glad)
target_include_directories(Penrose PRIVATE include vendor)

# 5. Link everything together
target_link_libraries(Penrose PRIVATE
    glfw
    glad
    glm::glm
    Eigen3::Eigen
)

# Copy the entire shaders folder (including textures) next to the executable
add_custom_command(TARGET Penrose POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:Penrose>/shaders"
)

# 6. Setup the benchmark executable
add_executable(benchmark_test
    src/benchmarking/main_benchmark.cpp
    src/benchmarking/orbital.cpp
    src/benchmarking/null_geodesic.cpp
    src/benchmarking/freefall.cpp
    src/spacetime/CoordinateChart.cpp
    src/spacetime/SchwarzschildMetric.cpp
    src/dynamics/GeodesicDynamics.cpp
    src/integration/RK4Integrator.cpp
    src/simulation/TrajectorySolver.cpp
    src/simulation/TerminationPolicy.cpp
)
target_include_directories(benchmark_test PRIVATE src)
target_link_libraries(benchmark_test PRIVATE Eigen3::Eigen)