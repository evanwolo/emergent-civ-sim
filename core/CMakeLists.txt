# Core library CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# 1. Enable CUDA
enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 17)

# 2. Collect sources (including .cu)
file(GLOB_RECURSE CORE_SOURCES
  "src/**/*.cpp"
  "src/**/*.cu" # <--- Added
)

file(GLOB_RECURSE CORE_HEADERS "include/**/*.h")

# 3. Create Library
add_library(grandstrategy STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(grandstrategy
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 4. Link Dependencies
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(grandstrategy PUBLIC OpenMP::OpenMP_CXX)
endif()

# Link CUDA Runtime
target_link_libraries(grandstrategy PRIVATE CUDA::cudart)

# Define flag for Kernel.cpp to know CUDA is available
target_compile_definitions(grandstrategy PRIVATE USE_CUDA)

target_compile_features(grandstrategy PUBLIC cxx_std_17)

# Enable Separable Compilation (Good practice for CUDA)
set_property(TARGET grandstrategy PROPERTY CUDA_SEPARABLE_COMPILATION ON)

# Set library properties
set_target_properties(grandstrategy PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)