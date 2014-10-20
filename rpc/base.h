#pragma once
#include "dll_define.h"
#include "data_types.h"
#include "discopiable.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/intrusive_ptr.hpp>
#include <memory>

typedef uint64 SeqType;

#define MEMORY_ALLOCATION_VLOG VLOG(5)
#define NETWORK_BEHAVIOR_VLOG  VLOG(4)
#define LOG_FATAL (LOG(FATAL))
#define LOG_ERROR (LOG(ERROR))
#define LOG_WARNING (LOG(WARNING))
#define LOG_INFO (LOG(INFO))

namespace gboost_rpc {
namespace detail {

enum RpcStatus {
  STATUS_OK = -1,
  UNIMPLEMENTED_METHOD_CALLING = 1,
  NETWORK_ERROR
};

typedef uint32 ServiceId;
typedef boost::function0<void> ZeroArgumentCB;
}
}
