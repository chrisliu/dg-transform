# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS BBinterval.proto)

# add_library(trace SHARED BBInterval.proto) protobuf_generate(TARGET trace
# LANGUAGE cpp OUT_VAR trace_proto) install(TARGETS trace)

protobuf_generate_cpp(TRACE_SRCS TRACE_HDRS BBInterval.proto InstTrace.proto
                      LLVMUID.proto)
add_library(dragongemTrace SHARED ${TRACE_HDRS} ${TRACE_SRCS})
set_target_properties(dragongemTrace PROPERTIES PUBLIC_HEADER "${TRACE_HDRS}")
target_link_libraries(dragongemTrace PUBLIC ${PROTOBUF_LIBRARY})
install(TARGETS dragongemTrace
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dragongem/trace)
