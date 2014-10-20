#include "rpc_client.h"
#include "sem_event.h"
#include "sync_event.h"
#include "atomic_event.h"
#include "benchmark/protocol/benchmark.pb.h"
#include "benchmark/wall_timer.h"
#include <gperftools/profiler.h>

DEFINE_string(server, "127.0.0.1", "Server IP");
DEFINE_int32(port, 20000, "port of rpc server");
DEFINE_int32(round, 50000, "Round will be executed");
DEFINE_int32(time, 0, "Test for minutes");
DEFINE_int32(payload_size, 0, "Payload size");
DEFINE_bool(client_per_request, false, "Reset the client before request");
DEFINE_bool(watch_delay, false, "Save delay of each request");
DEFINE_bool(profile, false, "Enable profiler");
DEFINE_int32(num_threads, 10, "Number of threads concurrently");

struct StatisticData {
  StatisticData()
      : elapse_in_ms(0),
        num_rounds(0),
        request_size(0) {
    if (FLAGS_time == 0) {
      delays.resize(FLAGS_round);
    } else {
      delays.reserve(100000);
    }
  }

  void PushDelay(uint64 delay) {
    delays.push_back(delay);
  }

  uint64 elapse_in_ms;
  int num_rounds;
  int request_size;

  std::vector<uint64> delays;
};

void ARound(gboost_rpc::RPCClient* client, RequestData const& request) {
  std::unique_ptr<gboost_rpc::RPCClient> client_holder;
  if (client == NULL) {
    client = gboost_rpc::CreateRPCClient(FLAGS_server, FLAGS_port, 1000, false);
    client_holder.reset(client);
  }

  CHECK(client != NULL);
  BenchmarkInterface_Stub service(client);
  ResponseData response;
  ::gboost_rpc::Semaphore sync;
//  ::gboost_rpc::AtomicEvent sync;
//  ::rpc::SyncEvent sync;

  service.Test(
      NULL, &request, &response,
      ::google::protobuf::NewCallback(&sync, &::gboost_rpc::Semaphore::Notify));
  sync.Wait();
}

void RunInRounds(StatisticData* statistic) {
  std::unique_ptr<gboost_rpc::RPCClient> client;
  if (!FLAGS_client_per_request) {
    client.reset(
        gboost_rpc::CreateRPCClient(FLAGS_server, FLAGS_port, 1000, false));
    CHECK(client != NULL);
  }

  RequestData request;
  if (FLAGS_payload_size > 0) {
    std::string payload;
    payload.resize(FLAGS_payload_size);
    request.set_payload(payload);
  }

  WallTimer timer;
  WallTimer delay_timer;

  if (FLAGS_profile)
    ProfilerStart("RunInRounds");

  timer.Start();
  for (int i = 0; i < FLAGS_round; ++i) {
    if (FLAGS_watch_delay) {
      delay_timer.Start();
    }

    ARound(client.get(), request);

    if (FLAGS_watch_delay) {
      statistic->delays[i] = delay_timer.ElapseInMs();
    }
  }

  if (FLAGS_profile) {
    ProfilerStop();
    ProfilerFlush();
  }

  statistic->elapse_in_ms = timer.ElapseInMs();
  statistic->request_size = request.ByteSize() + 8;
  statistic->num_rounds = FLAGS_round;
}

void RunInMinutes(StatisticData* statistic) {
  std::unique_ptr<gboost_rpc::RPCClient> client;
  if (!FLAGS_client_per_request) {
    client.reset(
        gboost_rpc::CreateRPCClient("127.0.0.1", FLAGS_port, 1000, false));
    CHECK(client != NULL);
  }

  RequestData request;
  if (FLAGS_payload_size > 0) {
    std::string payload;
    payload.resize(FLAGS_payload_size);
    request.set_payload(payload);
  }

  uint64 total_in_ms = FLAGS_time * 60000;
  WallTimer timer;
  WallTimer delay_timer;

  if (FLAGS_profile)
    ProfilerStart("RunInMinutes");

  timer.Start();
  int round = 0;
  do {
    if (FLAGS_watch_delay) {
      delay_timer.Start();
    }

    ARound(client.get(), request);
    if (FLAGS_watch_delay) {
      statistic->PushDelay(delay_timer.ElapseInMs());
    }

    ++round;
  } while (timer.ElapseInMs() < total_in_ms);

  if (FLAGS_profile) {
    ProfilerStop();
    ProfilerFlush();
  }

  statistic->elapse_in_ms = total_in_ms;
  statistic->request_size = request.ByteSize() + 8;
  statistic->num_rounds = round;
}

void PrintReport(std::vector<StatisticData> const& statistics,
                 uint64 total_elapse) {
  LOG(WARNING)<< "Report:";

  int thread_idx = 0;
  int total_rounds = 0;
  int total_bytes = 0;
  uint64 total_delay_inall = 0;
  uint64 max_delay_inall = 0;
  uint64 min_delay_inall = -1;

  std::vector<std::pair<int, uint64>> max_delays;

  for (auto data : statistics) {
    LOG(WARNING) << "===============+++ thread " << thread_idx << " +++===============";

    if (FLAGS_watch_delay) {
      int delay_idx = 0;
      uint64 total_delay = 0;
      uint64 max_delay = 0;
      uint64 min_delay = -1;

      int less_than_10 = 0;
      int less_than_50 = 0;
      int less_than_100 = 0;
      int less_than_150 = 0;
      int less_than_200 = 0;
      int less_than_250 = 0;
      int more_than_250 = 0;

      max_delays.clear();

      for (auto delay : data.delays) {
        total_delay += delay;
        if (delay > max_delay) {
          max_delay = delay;
        }

        if (delay < min_delay) {
          min_delay = delay;
        }

        if (delay > max_delay_inall) {
          max_delay_inall = delay;
        }

        if (delay < min_delay_inall) {
          min_delay_inall = delay;
        }

        if (delay < 10) {
          ++less_than_10;
        } else if (delay < 50) {
          ++less_than_50;
        } else if (delay < 100) {
          ++less_than_100;
        } else if (delay < 150) {
          ++less_than_150;
        } else if (delay < 200) {
          ++less_than_200;
        } else if (delay < 250) {
          ++less_than_250;
          max_delays.push_back(std::make_pair(delay_idx, delay));
        } else {
          ++more_than_250;
          max_delays.push_back(std::make_pair(delay_idx, delay));
        }

        ++delay_idx;
      }

      LOG(WARNING)<< "Total delay:" << total_delay << "ms";
      LOG(WARNING) << "Average delay:" << total_delay / data.num_rounds << "ms";
      LOG(WARNING) << "Max delay:" << max_delay << "ms";
      LOG(WARNING) << "Min delay:" << min_delay << "ms";
      LOG(ERROR) << "Distribution:" << less_than_10 << "(<10ms) " << less_than_50 << "(<50ms) "
      << less_than_100 << "(<100ms) " << less_than_150 << "(<150ms) " << less_than_200 << "(<200ms) "
      << less_than_250 << "(<250ms) " << more_than_250 << "(>250ms)";

      for (auto max_delay : max_delays) {
        LOG(ERROR) << "Max delay index " << max_delay.first << " delays " << max_delay.second << "ms";
      }

      total_delay_inall += total_delay;
    }

    LOG(WARNING)<< data.num_rounds << " round ran";
    LOG(WARNING)<< static_cast<double>(data.elapse_in_ms) / 1000.0 << "s(" << data.elapse_in_ms << "ms) elapsed";
    LOG(WARNING)<< data.num_rounds * 1000.0 / static_cast<double>(data.elapse_in_ms) << " tps";

    LOG(WARNING) << "Request Size:" << data.request_size;
    LOG(WARNING)<< data.request_size * data.num_rounds * 1000.0 / static_cast<double>(data.elapse_in_ms) << " Bps";
    LOG(WARNING) << "===============+++ End of thread " << thread_idx << " +++===============";

    total_rounds += data.num_rounds;
    total_bytes += data.request_size * data.num_rounds;

    ++thread_idx;
  }

  LOG(WARNING) << "||===============+++ Total  +++===============||";
  LOG(WARNING) << FLAGS_num_threads << " threads ran";
  LOG(WARNING) << total_rounds << " rounds in " << static_cast<double>(total_elapse) / 1000 << "s(" << total_elapse << "ms)";
  LOG(WARNING) << total_rounds * 1000.0 / static_cast<double>(total_elapse) << " tps";
  LOG(WARNING) << total_bytes * 1000.0 / static_cast<double>(total_elapse) << " Bps";

  if (FLAGS_watch_delay) {
    LOG(WARNING) << "Total delay:" << total_delay_inall << "ms";
    LOG(ERROR) << "Max delay:" << max_delay_inall << "ms";
    LOG(ERROR) << "Min delay:" << min_delay_inall << "ms";
  }
}

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  FLAGS_colorlogtostderr = true;
  FLAGS_alsologtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_round > 0) {
    std::vector<boost::thread*> threads;
    threads.reserve(FLAGS_num_threads);

    std::vector<StatisticData> statistics(FLAGS_num_threads);

    LOG(WARNING)<< FLAGS_num_threads << " threads get started ";

    WallTimer total;
    for (int i = 0; i < FLAGS_num_threads; ++i) {
      threads.push_back(new boost::thread(RunInRounds, &statistics[i]));
    }

    for (auto the_thread : threads) {
      the_thread->join();
    }

    PrintReport(statistics, total.ElapseInMs());
  }

  if (FLAGS_time > 0) {
    std::vector<boost::thread*> threads;
    threads.reserve(FLAGS_num_threads);

    std::vector<StatisticData> statistics(FLAGS_num_threads);

    LOG(WARNING)<< FLAGS_num_threads << " threads get started ";

    WallTimer total;

    for (int i = 0; i < FLAGS_num_threads; ++i) {
      threads.push_back(new boost::thread(RunInMinutes, &statistics[i]));
    }

    for (auto the_thread : threads) {
      the_thread->join();
    }

    PrintReport(statistics, total.ElapseInMs());
  }

  return 0;
}
