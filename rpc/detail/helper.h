#pragma once
#include "base.h"
#include "city.h"

namespace gboost_rpc {
namespace detail {

inline void CloseSocket(boost::asio::ip::tcp::socket* socket) {
  boost::system::error_code error;
  socket->close(error);
  if (error) {
    LOG_ERROR << "Fail to close socket due to [" << error.value() << "]"
              << error.message();
  }
}

inline ServiceId GetServiceID(std::string const& text) {
  return CityHash32(text.data(), text.size());
}
}
}
