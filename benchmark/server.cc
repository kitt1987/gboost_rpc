#include "rpc_server.h"
#include "benchmark/wall_timer.h"
#include "benchmark/protocol/benchmark.pb.h"
#include <gperftools/profiler.h>

DEFINE_int32(port, 20000, "port of rpc server");
DEFINE_int32(num_workers, 1, "Number of workers of rpc server");
DEFINE_int32(queue_size, 0xFFFFFFF, "Size of the pending queue");
DEFINE_int32(max_channel, 0xFFFFFFF, "Max channels rpc server could keep");
DEFINE_bool(echo_payload, false, "Echo the payload in requests");
DEFINE_int32(echo_payload_size, 0, "Size of payload echoed");
DEFINE_bool(watch_done_delay, false, "Watch the delay of calling done closure");
DEFINE_bool(profile, false, "Enable profiler");

class Benchmark : public BenchmarkInterface {
 public:
  Benchmark()
      : max_elapse_done_closure_(0),
        min_elapse_done_closure_(-1),
        count_done_closure_(0),
        total_elapse_done_closure_(0),
        elapse_in_ms_(0) {
  }

  virtual void Test(::google::protobuf::RpcController* controller,
                    const ::RequestData* request, ::ResponseData* response,
                    ::google::protobuf::Closure* done) {
    if (FLAGS_watch_done_delay) {
      procedure_timer_.Start();
    }

    if (FLAGS_echo_payload) {
      response->set_payload(request->payload());
    } else if (FLAGS_echo_payload_size > 0) {
      response->mutable_payload()->resize(FLAGS_echo_payload_size);
    }

    if (FLAGS_watch_done_delay) {
      done_closure_timer_.Start();
    }

    done->Run();

    if (FLAGS_watch_done_delay) {
      uint64 elapse = done_closure_timer_.ElapseInMs();
      elapse_in_ms_ += procedure_timer_.ElapseInMs();
      total_elapse_done_closure_ += elapse;
      ++count_done_closure_;

      if (elapse > max_elapse_done_closure_) {
        max_elapse_done_closure_ = elapse;
      }

      if (elapse < min_elapse_done_closure_) {
        min_elapse_done_closure_ = elapse;
      }
    }
  }

  void PrintReport() {
    if (FLAGS_watch_done_delay) {
      LOG(ERROR)<< "Report:";
      LOG(WARNING) << count_done_closure_ << " requests take " << elapse_in_ms_ << "ms";
      LOG(WARNING) << static_cast<double>(count_done_closure_) * 1000.0 / static_cast<double>(elapse_in_ms_) << " tps";
      LOG(ERROR) << "Max done closure delay:" << max_elapse_done_closure_ << "ms";
      LOG(ERROR) << "Min done closure delay:" << min_elapse_done_closure_ << "ms";
      LOG(ERROR) << "Total done closure delay:" << total_elapse_done_closure_ << "ms";
      LOG(ERROR) << "Average done closure delay:" << total_elapse_done_closure_ / count_done_closure_ << "ms";
    }
  }

private:
  uint64 max_elapse_done_closure_;
  uint64 min_elapse_done_closure_;
  uint64 count_done_closure_;
  uint64 total_elapse_done_closure_;
  uint64 elapse_in_ms_;
  WallTimer done_closure_timer_;
  WallTimer procedure_timer_;

};

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  FLAGS_colorlogtostderr = true;
  FLAGS_alsologtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  std::unique_ptr<gboost_rpc::RPCServer> server(
      gboost_rpc::CreateRPCServer("", FLAGS_port, FLAGS_num_workers,
                                  FLAGS_queue_size, FLAGS_max_channel));
  CHECK(server != NULL);
  Benchmark* bench = new Benchmark;
  server->AddService(bench);

  if (FLAGS_profile)
    ProfilerStart("RPCServer");

  server->Run();

  if (FLAGS_profile) {
    ProfilerStop();
    ProfilerFlush();
  }

  bench->PrintReport();
  return 0;
}
