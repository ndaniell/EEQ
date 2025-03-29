cmake_minimum_required(VERSION 3.16)

project(main LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

# Add compiler flags for gcov coverage
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")


add_executable(main tests.c event_queue.c)

install(TARGETS main)

enable_testing()
add_test(NAME main COMMAND main)

# Add custom target for lcov coverage reports
add_custom_target(lcov_coverage
    # Create coverage directory
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lcov_coverage
    # Initialize coverage data
    COMMAND lcov --zerocounters --directory ${CMAKE_BINARY_DIR}/CMakeFiles/main.dir/
    # Run the tests
    COMMAND $<TARGET_FILE:main>             # Third party code
    # Capture the coverage data
    COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR}/CMakeFiles/main.dir/ --output-file lcov_coverage/coverage.info
    COMMAND lcov --remove ${CMAKE_BINARY_DIR}/lcov_coverage/coverage.info 
        --output-file ${CMAKE_BINARY_DIR}/lcov_coverage/filtered_coverage.info
        '*tests.c'
    # Generate HTML report
    COMMAND genhtml ${CMAKE_BINARY_DIR}/lcov_coverage/filtered_coverage.info --output-directory ${CMAKE_BINARY_DIR}/lcov_coverage/html
    COMMENT "Generating lcov coverage reports"
)

# Add custom target for code formatting
add_custom_target(format
    COMMAND clang-format -i -style=llvm
        ${CMAKE_SOURCE_DIR}/tests.c
        ${CMAKE_SOURCE_DIR}/event_queue.c
        ${CMAKE_SOURCE_DIR}/event_queue.h
        ${CMAKE_SOURCE_DIR}/circular_buffer.h
    COMMENT "Formatting source files with clang-format using LLVM style"
)
