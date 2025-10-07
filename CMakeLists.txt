cmake_minimum_required(VERSION 3.22)
project(cvp_eo_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCV REQUIRED)

# doctest finder (header-only)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(Doctest REQUIRED)

include_directories(include)

add_library(cvp_core
    src/core/logger.cpp
    src/core/metrics.cpp
    src/core/pipeline.cpp)

target_link_libraries(cvp_core PUBLIC ${OpenCV_LIBS})

add_library(cvp_modules
    src/modules/capture/v4l2_capture.cpp
    src/modules/preprocess/basic_preprocess.cpp
    src/modules/infer/onnx_infer.cpp
    src/modules/postprocess/nms.cpp
    src/modules/tracker/sort_tracker.cpp
    src/modules/overlay/draw.cpp
    src/modules/sink/video_writer.cpp)

target_link_libraries(cvp_modules PUBLIC cvp_core ${OpenCV_LIBS})

add_executable(cvp_app src/main.cpp)
target_link_libraries(cvp_app PRIVATE cvp_modules cvp_core ${OpenCV_LIBS})

# Unit tests
file(GLOB TEST_SOURCES tests/unit/*.cpp)
add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests PRIVATE cvp_modules cvp_core ${OpenCV_LIBS} Doctest::Doctest)
add_test(NAME unit_tests COMMAND unit_tests)

# Integration tests
add_executable(integ_tests tests/integration/test_e2e_smoke.cpp)
target_link_libraries(integ_tests PRIVATE cvp_modules cvp_core ${OpenCV_LIBS} Doctest::Doctest)
add_test(NAME integ_tests COMMAND integ_tests)