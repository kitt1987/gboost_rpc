#include "base.h"
#include "rpc_client.h"
#include "sync_event.h"
#include "test/pingpong_server.h"
//#include "util/wall_timer.h"
#include "callback.h"
#include "sync_event.h"

DEFINE_int32(port, 20141, "");
DEFINE_int32(num_clients, 1, "");
DEFINE_int32(round, 0xFFFFFFFF, "");

void ClientThread(uint32 client_id) {
  std::unique_ptr<gboost_rpc::RPCClient> client(
      gboost_rpc::CreateRPCClient("127.0.0.1", FLAGS_port, 1000, true));
  CHECK(client.get() != NULL);
  PingPongTesting_Stub client_service(client.get());

  uint64 count = 0;
  Ball request;
  request.set_client_id(client_id);
  request.set_count(count);

//  WallTimer timer;
  while (request.count() < FLAGS_round) {
    Ball response;
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
//    LOG(INFO)<< "[Client " << client_id << "] Got the ball "
//    << response.count() << " times";
    request.set_count(response.count());
  }

//  LOG(WARNING)<< FLAGS_round << " rounds take " << timer.TimeSpanInMs() << " ms";
}

//void JustCalling1() {
//  std::unique_ptr<gboost_rpc::RpcClient> client(
//      gboost_rpc::CreateRpcClient("127.0.0.1", FLAGS_port, 1000, true));
//  CHECK(client.get() != NULL);
//  PingPongTesting_Stub client_service(client.get());
//
//  uint64 count = 0;
//  Ball request, response;
//  request.set_client_id(0);
//  request.set_count(count);
//
//  ::gboost_rpc::SyncEvent sync;
//  client_service.PingPong(
//      NULL, &request, &response,
//      ::google::protobuf::NewCallback(&sync, &::gboost_rpc::SyncEvent::Notify));
//  sync.Wait();
//  CHECK_EQ(response.client_id(), request.client_id());
//  CHECK_EQ(response.count(), request.count() + 1);
//}
//
//void JustCalling3() {
//  std::unique_ptr<gboost_rpc::RpcClient> client(
//      gboost_rpc::CreateRpcClient("127.0.0.1", FLAGS_port, 1000, true));
//  CHECK(client.get() != NULL);
//  PingPongTesting_Stub client_service(client.get());
//
//  uint64 count = 0;
//  Ball* request = new Ball;
//  Ball* response = new Ball;
//  request->set_client_id(0);
//  request->set_count(count);
//
//  std::shared_ptr<gboost_rpc::CancellableWait> cb(
//      new gboost_rpc::CancellableWait(request, response));
//  client_service.PingPong(
//      NULL,
//      request,
//      response,
//      ::google::protobuf::NewCallback(
//          boost::bind(&gboost_rpc::CancellableWait::Done, cb)));
//  if (!cb->TimedWait(1000)) {
//    // CAUTION It must be done here
//    cb->Cancel();
//    return;
//  }
//
//  CHECK_EQ(response->client_id(), request->client_id());
//  CHECK_EQ(response->count(), request->count() + 1);
//}

int main(int argc, char** argv) {
//  ::google::InitGoogleLogging (argv[0]);
  FLAGS_colorlogtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  std::vector<boost::thread*> clients;
  uint32 client_id = 0;

  while (client_id < FLAGS_num_clients) {
    clients.push_back(
        new boost::thread(boost::bind(ClientThread, client_id++)));
  }

  for (std::vector<boost::thread*>::iterator each = clients.begin();
      each != clients.end(); ++each) {
    (*each)->join();
    delete (*each);
  }

  return 0;
}
