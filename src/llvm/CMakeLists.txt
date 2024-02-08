add_library(InstrumentTracePass MODULE trace/InstrumentTracePass.cpp)
set_target_properties(InstrumentTracePass PROPERTIES COMPILE_FLAGS "-fno-rtti")
target_compile_options(InstrumentTracePass PRIVATE -Wall -Wextra)
target_compile_options(InstrumentTracePass PRIVATE -fdiagnostics-color=always)
