add_library(dragongem SHARED dummy.cpp)

add_subdirectory(trace)
# add_subdirectory(llvm)

# Hacky may need to restructure project ;(
add_library(dragongemLLVM SHARED llvm/ExecutableBasicBlock.cpp
                                 llvm/CanonicalId.cpp)
target_compile_options(dragongemLLVM PUBLIC -fno-rtti)
target_include_directories(dragongemLLVM PUBLIC ${dragongem_INCLUDE_DIRS})
target_include_directories(
  dragongemLLVM PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
target_include_directories(dragongemLLVM PRIVATE ${stream_INCLUDE_DIRS})
target_link_libraries(dragongemLLVM PUBLIC dragongemTrace)

add_library(
  dragongemInstrumentationPasses SHARED
  llvm/trace/InstrumentSimpointPass.cpp llvm/trace/InstrumentInstTracePass.cpp
  llvm/trace/Registry.cpp llvm/trace/InstrumentationInterface.cpp
  llvm/trace/CLOpts.cpp)
target_compile_options(dragongemInstrumentationPasses PUBLIC -fno-rtti)
target_link_libraries(dragongemInstrumentationPasses PUBLIC dragongemLLVM)
target_link_libraries(dragongem dragongemInstrumentationPasses)
install(TARGETS dragongemInstrumentationPasses dragongemLLVM)

# Note: Insrumentation tools can only be built after the trace library has been
# built.
add_library(dragongemInstrumentationTools_shared SHARED
            llvm/trace/Instrumentation.cpp)
set_target_properties(
  dragongemInstrumentationTools_shared
  PROPERTIES POSITION_INDEPENDENT_CODE ON OUTPUT_NAME
                                          dragongemInstrumentationTools)
target_link_libraries(dragongemInstrumentationTools_shared dragongemTrace)
target_include_directories(dragongemInstrumentationTools_shared
                           PUBLIC ${dragongem_INCLUDE_DIRS})
target_include_directories(
  dragongemInstrumentationTools_shared
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> ${stream_INCLUDE_DIRS})
install(TARGETS dragongemInstrumentationTools_shared)

# add_library(dragongemInstrumentationTools_static STATIC
#             llvm/trace/Instrumentation.cpp)
# set_target_properties(
#   dragongemInstrumentationTools_static
#   PROPERTIES POSITION_INDEPENDENT_CODE ON OUTPUT_NAME
#                                           dragongemInstrumentationTools)
# target_link_libraries(dragongemInstrumentationTools_static
#                       dragongemTrace_static)
# target_include_directories(
#   dragongemInstrumentationTools_static
#   PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> ${stream_INCLUDE_DIRS})
# install(TARGETS dragongemInstrumentationTools_static)

install(TARGETS dragongem)
install(
  DIRECTORY ${dragongem_INCLUDE_DIRS}/dragongem/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dragongem
  FILES_MATCHING
  PATTERN "*.h*")
