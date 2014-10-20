#include "sem_event.h"
#include "sync_event.h"
#include "atomic_event.h"
#include "wall_timer.h"
#include <gperftools/profiler.h>

DEFINE_int32(rounds, 1000000, "");
DEFINE_bool(profile, false, "Enable profiler");
DEFINE_bool(sem, false, "Benchmark semaphore");
DEFINE_bool(cv, false, "Benchmark condition variable");
DEFINE_bool(atomic, false, "Benchmark atomic event");
DEFINE_int32(num_producers, 1, "");
DEFINE_int32(num_consumers, 1, "");

struct Statistic {
  uint64 elapse_ms;
  int rounds;
};

template<typename EventType>
void ProduceEvent(EventType* event, Statistic* statistic) {
  int counter = 0;
  int all_rounds = FLAGS_rounds / FLAGS_num_producers;
  WallTimer timer;
  while (counter < all_rounds) {
    event->Notify();
    ++counter;
  }

  statistic->elapse_ms = timer.ElapseInMs();
  statistic->rounds = all_rounds;
}

template<typename EventType>
void ConsumeEvent(EventType* event, Statistic* statistic) {
  int counter = 0;
  int all_rounds = FLAGS_rounds / FLAGS_num_consumers;
  WallTimer timer;
  while (counter < all_rounds) {
    event->Wait();
    ++counter;
  }

  statistic->elapse_ms = timer.ElapseInMs();
  statistic->rounds = all_rounds;
}

template<typename EventType>
void Bench(EventType* event) {
  std::vector<boost::thread> consumers(FLAGS_num_consumers);
  std::vector<boost::thread> producers(FLAGS_num_producers);
  std::vector<Statistic> consumer_statistics(FLAGS_num_consumers);
  std::vector<Statistic> producer_statistics(FLAGS_num_producers);

  WallTimer total_timer;
  int i = 0;
  for (boost::thread& consumer : consumers) {
    consumer = boost::thread(
        boost::bind(ConsumeEvent<EventType>, event, &consumer_statistics[i]));
    ++i;
  }

  i = 0;
  for (boost::thread& producer : producers) {
    producer = boost::thread(
        boost::bind(ProduceEvent<EventType>, event, &producer_statistics[i]));
    ++i;
  }

  for (boost::thread& consumer : consumers) {
    consumer.join();
  }

  for (boost::thread& producer : producers) {
    producer.join();
  }

  uint64 total_elapse = total_timer.ElapseInMs();
  LOG(WARNING)<< total_elapse << "ms elapses in total";

  for (auto statistic : consumer_statistics) {
    LOG(WARNING)<< "Consumer runs " << statistic.rounds << " rounds which takes " << statistic.elapse_ms << "ms";
  }

  for (auto statistic : producer_statistics) {
    LOG(WARNING)<< "Producer runs " << statistic.rounds << " rounds which takes " << statistic.elapse_ms << "ms";
  }
}

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  FLAGS_colorlogtostderr = true;
  FLAGS_alsologtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_profile)
    ProfilerStart("sc_bench");

  if (FLAGS_num_consumers > FLAGS_num_producers) {
    CHECK_EQ(FLAGS_num_consumers % FLAGS_num_producers, 0);
  } else {
    CHECK_EQ(FLAGS_num_producers % FLAGS_num_consumers, 0);
  }

  if (FLAGS_sem) {
    ::gboost_rpc::Semaphore event;
    Bench(&event);
  }

  if (FLAGS_cv) {
    ::gboost_rpc::SyncEvent event;
    Bench(&event);
  }

  if (FLAGS_atomic) {
    ::gboost_rpc::AtomicEvent event;
    Bench(&event);
  }

  if (FLAGS_profile) {
    ProfilerStop();
    ProfilerFlush();
  }

  return 0;
}
