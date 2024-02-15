add_library(dragongem SHARED dummy.cpp)

add_subdirectory(llvm)

install(TARGETS dragongem)
