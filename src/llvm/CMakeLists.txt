set(CMAKE_CXX_STANDARD
    17
    CACHE STRING "")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra
# -fdiagnostics-color=always")

if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_library(InstrumentTracePass SHARED trace/InstrumentTracePass.cpp)
target_link_libraries(dragongem InstrumentTracePass)

# TODO: debug why install shared library results in linking errors.
# install(TARGETS InstrumentTracePass)
