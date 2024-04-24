set(CMAKE_CXX_STANDARD
    17
    CACHE STRING "")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra
# -fdiagnostics-color=always")

if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# add_library(InstrumentSimpointPass SHARED trace/InstrumentSimpointPass.cpp)
# target_link_libraries(dragongem InstrumentSimpointPass) install(TARGETS
# InstrumentSimpointPass)

# add_library(InstrumentationTools SHARED trace/Instrumentation.cpp)
# target_compile_options(InstrumentationTools PRIVATE -fPIC) install(TARGETS
# InstrumentationTools)

# TODO: debug why install shared library results in linking errors.
# install(TARGETS InstrumentTracePass)

include(CMakePrintHelpers)
cmake_print_variables(dragongem_TRACE_INCLUDE_DIR)
add_library(dragongemLLVM SHARED ExecutableBasicBlock.cpp CanonicalId.cpp)
target_include_directories(dragongemLLVM PUBLIC ${dragongem_INCLUDE_DIRS})
target_include_directories(dragongemLLVM PRIVATE ${dragongem_TRACE_INCLUDE_DIR})
target_link_libraries(dragongemLLVM PUBLIC dragongemTrace)

add_library(dragongemInstrumentationPasses SHARED
            trace/InstrumentSimpointPass.cpp trace/InstrumentationInterface.cpp)
target_link_libraries(dragongemInstrumentationPasses PUBLIC dragongemLLVM)
target_link_libraries(dragongem dragongemInstrumentationPasses)
install(TARGETS dragongemInstrumentationPasses dragongemLLVM)
