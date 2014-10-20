#include "test/pingpong_server.h"
#include "rpc_server.h"
#include <iostream>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

PingPongServer::PingPongServer() {

}

PingPongServer::~PingPongServer() {

}

void PingPongServer::PingPong(::google::protobuf::RpcController* controller,
                              const ::Ball* request, ::Ball* response,
                              ::google::protobuf::Closure* done) {
  response->set_client_id(request->client_id());
  response->set_count(request->count() + 1);
//  LOG(INFO)<< "[Server] Got the ball " << response->count()
//  << " times from client " << request->client_id();
  done->Run();
}

DEFINE_int32(port, 20141, "");
DEFINE_int32(worker, 1, "");
DEFINE_int32(num_servers, 1, "");

int main(int argc, char** argv) {
//  ::google::InitGoogleLogging (argv[0]);
  FLAGS_colorlogtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);
//  std::vector<gboost_rpc::RpcServer*> servers;
  std::unique_ptr<gboost_rpc::RPCServer> server(
      gboost_rpc::CreateRPCServer("", FLAGS_port, FLAGS_worker, 0xFFFFFF, 0xFFFFFF));
  CHECK(server.get() != NULL);
  server->AddService(new PingPongServer);
  server->Run();
//  servers.push_back(server.release());
//
//  while (true) {
//#ifdef WIN32
//    Sleep(1000);
//#else
//    sleep(1);
//#endif
//  }

  return 0;
}
