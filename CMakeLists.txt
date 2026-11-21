cmake_minimum_required(VERSION 3.12)
project(equaliser LANGUAGES C CXX)

add_library(equaliser equaliser.cpp)
target_include_directories(equaliser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(equaliser PUBLIC cxx_std_11)

option(EQUALISER_BUILD_TESTS "Build equaliser tests" ON)
option(EQUALISER_BUILD_SAMPLES "Build equaliser samples" ON)

if(EQUALISER_BUILD_TESTS)
    enable_testing()

    add_executable(equaliser_cpp_test tests/test_equaliser.cpp)
    target_link_libraries(equaliser_cpp_test PRIVATE equaliser)
    add_test(NAME equaliser_cpp_test COMMAND equaliser_cpp_test)

    add_executable(equaliser_c_test tests/test_c_api.c)
    target_link_libraries(equaliser_c_test PRIVATE equaliser)
    set_target_properties(equaliser_c_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME equaliser_c_test COMMAND equaliser_c_test)
endif()

if(EQUALISER_BUILD_SAMPLES)
    add_executable(equaliser_cpp_sample samples/cpp_sample.cpp)
    target_link_libraries(equaliser_cpp_sample PRIVATE equaliser)

    add_executable(equaliser_c_sample samples/c_sample.c)
    target_link_libraries(equaliser_c_sample PRIVATE equaliser)
    set_target_properties(equaliser_c_sample PROPERTIES LINKER_LANGUAGE CXX)
endif()
