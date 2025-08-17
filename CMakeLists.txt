cmake_minimum_required(VERSION 3.12)
project(cats-tracker)

find_package(CUDA REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DAPI_EXPORTS)

# Include CUDA directories
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(src/)

# Add source files
set(SOURCES
    main.cpp
    src/yolov11.cpp
)

# Add CUDA source files
set(CUDA_SOURCES src/preprocess.cu)

# Add headers
set(HEADERS
    src/yolov11.h
    src/macros.h
    src/logging.h
    src/cuda_utils.h
    src/preprocess.h
    src/common.h
)

# Find and include OpenCV
set(OpenCV_DIR "E:/DevLibs/opencv/build")

# Set TensorRT path if not set in environment variables
set(TENSORRT_DIR "E:/DevLibs/Nvidia/TensorRT/v10.2-cu118")

# Include OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Include TensorRT
include_directories(${TENSORRT_DIR}/include)
link_directories(${TENSORRT_DIR}/lib)
# set(TENSORRT_LIBS nvinfer nvinfer_plugin nvparsers nvonnxparser)
set(TENSORRT_LIBS nvinfer_10 nvinfer_plugin_10 nvonnxparser_10)


# Create an executable
cuda_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${CUDA_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${TENSORRT_LIBS}
)