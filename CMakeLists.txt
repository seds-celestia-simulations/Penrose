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

# 7. CPU visualization module (headless library + optional viewer)
set(VISUALIZATION_SOURCES
    visualization/Geometry/Coordinates.cpp
    visualization/Geometry/Mesh.cpp
    visualization/Trajectory/Trajectory.cpp
    visualization/Trajectory/TrajectoryAdapter.cpp
    visualization/Scene/Scene.cpp
    visualization/Scene/SceneBuilder.cpp
    visualization/Camera/Camera.cpp
    visualization/Renderer/Framebuffer.cpp
    visualization/Renderer/CPURasterizer.cpp
    visualization/Presentation/PresentationProfile.cpp
    visualization/Presentation/PresentationDefaults.cpp
    visualization/Presentation/PostProcessor.cpp
    visualization/Presentation/PresentationPipeline.cpp
    visualization/IO/CsvTrajectoryLoader.cpp
    visualization/IO/PpmWriter.cpp
    visualization/IO/OutputPaths.cpp
    visualization/IO/StarfieldBackground.cpp
)

add_library(visualization STATIC ${VISUALIZATION_SOURCES})
target_include_directories(visualization
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/visualization
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(visualization PRIVATE PENROSE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(visualization PUBLIC Eigen3::Eigen)

add_executable(visualization_export visualization/Apps/HeadlessExportMain.cpp)
target_compile_definitions(visualization_export PRIVATE PENROSE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(visualization_export PRIVATE visualization)

add_executable(visualization_benchmark visualization/Apps/BenchmarkMain.cpp)
target_link_libraries(visualization_benchmark PRIVATE visualization)

add_executable(visualization_test
    visualization/Tests/TestMain.cpp
    visualization/Tests/TestCoordinates.cpp
    visualization/Tests/TestTrajectory.cpp
    visualization/Tests/TestCsv.cpp
    visualization/Tests/TestCamera.cpp
    visualization/Tests/TestRenderer.cpp
)
target_link_libraries(visualization_test PRIVATE visualization)

set(PHYSICS_ENGINE_SOURCES
    physics/metrics/CoordinateChart.cpp
    physics/metrics/SchwarzschildMetric.cpp
    physics/geodesics/GeodesicDynamics.cpp
    physics/integrators/RK4Integrator.cpp
    physics/simulation/TrajectorySolver.cpp
    physics/simulation/TerminationPolicy.cpp
)

option(PENROSE_BUILD_VIEWER "Build interactive CPU visualization viewer" ON)
if(PENROSE_BUILD_VIEWER)
    add_executable(visualization_viewer
        visualization/Apps/ViewerMain.cpp
        visualization/Apps/DisplayBlit.cpp
    )
    target_include_directories(visualization_viewer PRIVATE realtime/gpu)
    target_link_libraries(visualization_viewer PRIVATE visualization glfw glad)

    add_executable(visualization_viewer_example
        examples/interactive_viewer/main.cpp
        visualization/Apps/DisplayBlit.cpp
        ${PHYSICS_ENGINE_SOURCES}
    )
    target_include_directories(visualization_viewer_example
        PRIVATE physics visualization realtime/gpu ${CMAKE_SOURCE_DIR})
    target_link_libraries(visualization_viewer_example PRIVATE visualization glfw glad Eigen3::Eigen)
endif()

# 8. Direct integration example (Scientific Engine -> visualization)
add_executable(visualization_example
    examples/direct_integration/main.cpp
    ${PHYSICS_ENGINE_SOURCES}
)
target_include_directories(visualization_example PRIVATE physics visualization ${CMAKE_SOURCE_DIR})
target_compile_definitions(visualization_example PRIVATE PENROSE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(visualization_example PRIVATE visualization Eigen3::Eigen)
