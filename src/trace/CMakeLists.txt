# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS BBinterval.proto)

# add_library(trace SHARED BBInterval.proto) protobuf_generate(TARGET trace
# LANGUAGE cpp OUT_VAR trace_proto) install(TARGETS trace)

# protobuf_generate_cpp( TRACE_SRCS TRACE_HDRS BBInterval.proto InstTrace.proto
# LLVMUID.proto SimStats.proto Metadata.proto)

set(trace_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/dragongem/trace)
protobuf_generate(
  LANGUAGE
  cpp
  PROTOS
  BBInterval.proto
  InstTrace.proto
  LLVMUID.proto
  SimStats.proto
  Metadata.proto
  PROTOC_OUT_DIR
  ${trace_INCLUDE_DIR}
  OUT_VAR
  TRACE_SRCS)

foreach(FILENAME ${TRACE_SRCS})
  string(REGEX MATCH ".*\.h$" HEADER ${FILENAME})
  if(HEADER)
    list(APPEND TRACE_HDRS ${HEADER})
  endif()
endforeach()

# protobuf_generate_python(TRACE_PY BBInterval.proto InstTrace.proto
# LLVMUID.proto SimStats.proto Metadata.proto) message("${TRACE_PY}")
# message("${TRACE_HDRS}")
add_library(dragongemTrace SHARED ${TRACE_HDRS} ${TRACE_SRCS})
set_target_properties(dragongemTrace PROPERTIES PUBLIC_HEADER "${TRACE_HDRS}")
target_link_libraries(dragongemTrace PUBLIC ${PROTOBUF_LIBRARY})
install(TARGETS dragongemTrace
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dragongem/trace)

add_library(dragongemTrace_static STATIC ${TRACE_HDRS} ${TRACE_SRCS})
set_target_properties(
  dragongemTrace_static
  PROPERTIES PUBLIC_HEADER "${TRACE_HDRS}"
             POSITION_INDEPENDENT_CODE ON
             OUTPUT_NAME dragongemTrace)
target_link_libraries(dragongemTrace_static PUBLIC ${PROTOBUF_LIBRARY})
install(TARGETS dragongemTrace_static
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dragongem/trace)
