#include "base.h"
#include "rpc_client.h"
#include "test/pingpong_server.h"
#include "callback.h"
#include "sync_event.h"

DEFINE_int32(port, 20141, "");
DEFINE_int32(round, 30000, "");

int main(int argc, char** argv) {
  FLAGS_colorlogtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  uint32 client_id = 0;

  for (int i = 0; i < FLAGS_round; ++i) {
    std::unique_ptr<gboost_rpc::RPCClient> client(
        gboost_rpc::CreateRPCClient("127.0.0.1", FLAGS_port, 1000, true));
    CHECK(client.get() != NULL);
    PingPongTesting_Stub client_service(client.get());

    Ball request, response;
    request.set_client_id(client_id);
    request.set_count(i);
    ::gboost_rpc::SyncEvent sync;
    client_service.PingPong(
        NULL,
        &request,
        &response,
        ::google::protobuf::NewCallback(&sync,
                                        &::gboost_rpc::SyncEvent::Notify));

    sync.Wait();
    CHECK_EQ(response.client_id(), request.client_id());
    CHECK_EQ(response.count(), request.count() + 1);
  }

  return 0;
}
