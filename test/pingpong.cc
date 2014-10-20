#include "base.h"
#include "rpc_server.h"
#include "rpc_client.h"
#include "sync_event.h"
#include "callback.h"
#include "test/pingpong_server.h"

#ifdef _MSC_VER
#define _WIN32_WINNT 0x0501
#endif

#define BOOST_REGX_NO_LIB
#define BOOST_DATE_TIME_SOURCE
#define BOOST_SYSTEM_NO_LIB

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using namespace boost::asio;

uint16 port = 5959;
uint64 round = 2;
uint32 num_clients = 1;

void ClientThread(uint32 client_id) {
  //�����client ��ָ��RpcClientImpl��
  std::unique_ptr<rpc::RpcClient> client(
      rpc::CreateRpcClient("127.0.0.1", port, 1000));
  // ASSERT_TRUE(client.Get() != NULL);

  PingPongTesting_Stub client_service(client.get());

  uint64 count = 0;
  Ball request, response;

  //Э����
  request.set_client_id(client_id);
  request.set_count(count);

  while (request.count() < round) {
    rpc::SyncWait sync_wait;
    Sleep(1000);
    //	���������ù��
    //	CallMethod==>RpcClientImpl::CallMethod==>ClientRpcProcessor::SendRequest=>LocalRpcChannel::SendPackage
    //	LocalRpcChannel::SendPackage���õ��� libevent ��bufferevent_write ����������ֱ�ӷ���ȥ

    client_service.PingPong(NULL, &request, &response, &sync_wait);

    //���������ȴ�������˷���
    sync_wait.Wait();

    CHECK_EQ(response.client_id(), request.client_id());
    CHECK_EQ(response.count(), request.count() + 1);
    LOG(INFO)<< "[Client " << client_id << "] Got the ball " << response.count() << " times";

    request.set_count(response.count());
  }

  while (1)
    ;
  CHECK_EQ(response.count(), round);
}

void ServerThread(uint32 client_id) {
  ScopedPtr<rpc::RpcServer> server(rpc::CreateRpcServer("", port, 5, 500));
  server->AddService(new PingPongServer);
  server->RunInAnotherThread();
  while (1)
    ;
}

int main() {

  //////////////////////////////////////////////////////////////////////////
  //��������
  //
  //�ͻ���
  //std::vector<boost::thread*> clients;
  uint32 client_id1 = 0;

  /*while (client_id < num_clients)
   {*/
  new boost::thread(boost::bind(ServerThread, client_id1++));
  //}

  Sleep(1000);

  //////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////
  //�ͻ���
  std::vector<boost::thread*> clients;
  uint32 client_id = 0;

  while (client_id < num_clients) {
    clients.push_back(
        new boost::thread(boost::bind(ClientThread, client_id++)));
  }

  for (std::vector<boost::thread*>::iterator each = clients.begin();
      each != clients.end(); ++each) {
    (*each)->join();
    delete (*each);
  }
  //////////////////////////////////////////////////////////////////////////

  while (1)
    ;

}
