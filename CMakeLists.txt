cmake_minimum_required (VERSION 2.6)
project (librpc)

file(GLOB DETAIL_SRCS "rpc/detail/*.cc")
file(GLOB PROTOCOL_SRCS "rpc/detail/protocol/*.cc")
set(LIB_SOURCES ${DETAIL_SRCS} ${PROTOCOL_SRCS})

file(GLOB PP_PROTOCOL "test/protocol/*.cc")
set(PPC_SOURCES ${PP_PROTOCOL} "test/pingpong_client.cc")
set(PPS_SOURCES ${PP_PROTOCOL} "test/pingpong_server.cc")

file(GLOB BENCH_PROTOCOL "benchmark/protocol/*.cc")
set(BENCHMARK_C_SOURCES ${BENCH_PROTOCOL} "benchmark/client.cc")
set(BENCHMARK_S_SOURCES ${BENCH_PROTOCOL} "benchmark/server.cc")

set(LIB_NAME rpc)
set(STATIC_LIB_NAME ${LIB_NAME}_static)
set(STATIC_LIB_NAME_WITHOUT_THIRD_PARTY ${LIB_NAME}_no_3rd_party)

if(${WIN32})
message(FATAL_ERROR "This version of Visual Studio is not supported.")
endif(${WIN32})

option(DISABLE_TCMALLOC "DO NOT link to tcmalloc libraries")

message(STATUS "Building for Unix")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
  set(LIB_NAME ${LIB_NAME}_debug)
  set(STATIC_LIB_NAME ${STATIC_LIB_NAME}_debug)
  set(STATIC_LIB_NAME_WITHOUT_THIRD_PARTY ${STATIC_LIB_NAME_WITHOUT_THIRD_PARTY}_debug)
endif()

set(CXX_FLAGS
  -g
  -Wall
  -Wextra
  -Wconversion
  -Wno-unused-parameter
  -Wold-style-cast
  -Woverloaded-virtual
  -Wpointer-arith
  -Wshadow
  -Wwrite-strings
  -std=c++0x
  -rdynamic
)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(LIBRARY_OUTPUT_PATH "out")

#add_definitions(-DBOOST_ASIO_DISABLE_THREADS)

include_directories("." "rpc")
add_library(${STATIC_LIB_NAME} STATIC ${LIB_SOURCES})
add_library(${STATIC_LIB_NAME_WITHOUT_THIRD_PARTY} STATIC ${LIB_SOURCES})
add_library(${LIB_NAME} SHARED ${LIB_SOURCES})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/out /usr/local/lib /usr/lib/x86_64-linux-gnu)

set(STATIC_LIBS
  -pthread
  rt
  unwind
  lib${STATIC_LIB_NAME}.a
  boost_thread-mt
  boost_system-mt
  glog
  gflags
  protobuf
)

set(DYNAMIC_LIBS
  pthread
  ${LIB_NAME}
  boost_thread-mt
  boost_system-mt
  gflags
  glog
  protobuf
  profiler
)

if (${DISABLE_TCMALLOC} STREQUAL "OFF")
  message(STATUS "Link to tcmalloc")
  set(STATIC_LIBS ${STATIC_LIBS} tcmalloc)
  set(DYNAMIC_LIBS ${DYNAMIC_LIBS} tcmalloc)
endif (${DISABLE_TCMALLOC} STREQUAL "OFF")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  # static lib pingpong test
  add_executable(pp_client_static ${PPC_SOURCES})
  target_link_libraries(pp_client_static ${STATIC_LIBS}) 
  add_dependencies(pp_client_static ${STATIC_LIB_NAME})

  add_executable(pp_server_static ${PPS_SOURCES})
  target_link_libraries(pp_server_static ${STATIC_LIBS}) 
  add_dependencies(pp_server_static ${STATIC_LIB_NAME})

  add_executable(stability_static "test/stability.cc" ${PP_PROTOCOL})
  target_link_libraries(stability_static ${STATIC_LIBS})
  add_dependencies(stability_static ${STATIC_LIB_NAME})

  add_executable(pp_client ${PPC_SOURCES})
  target_link_libraries(pp_client ${DYNAMIC_LIBS})
  add_dependencies(pp_client ${LIB_NAME})

  add_executable(pp_server ${PPS_SOURCES})
  target_link_libraries(pp_server ${DYNAMIC_LIBS})
  add_dependencies(pp_server ${LIB_NAME})

  add_executable(stability "test/stability.cc" ${PP_PROTOCOL})
  target_link_libraries(stability ${DYNAMIC_LIBS})
  add_dependencies(stability ${LIB_NAME})

endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
  add_executable(bench_c ${BENCHMARK_C_SOURCES})
  target_link_libraries(bench_c ${DYNAMIC_LIBS})
  add_dependencies(bench_c ${LIB_NAME})

  add_executable(bench_s ${BENCHMARK_S_SOURCES})
  target_link_libraries(bench_s ${DYNAMIC_LIBS})
  add_dependencies(bench_s ${LIB_NAME})

  add_executable(sc_bench "benchmark/sc_bench.cc")
  target_link_libraries(sc_bench ${DYNAMIC_LIBS})
endif(${CMAKE_BUILD_TYPE} MATCHES "Release")
