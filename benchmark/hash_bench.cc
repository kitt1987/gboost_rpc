#include "base.h"
#include "benchmark/wall_timer.h"
#include "detail2/city.h"
#include "detail2/util.h"
#include <time.h>
#include <stdlib.h>

DEFINE_int32(round, 1000000, "");
DEFINE_int32(size, 128, "");
DEFINE_string(pattern, "rpc.benchmark.Service", "");

std::string GetData(int index) {
  std::string data(FLAGS_pattern);
  return data.append(ConvertToStringFrom(index, 10));
}

std::vector<std::string> GetRandomSource() {
  std::vector<std::string> source_data;
  for (int i = 0; i < FLAGS_round; ++i) {
    srand(time(NULL));
    int index = rand() % FLAGS_size;
    source_data.push_back(GetData(index));
  }

  return source_data;
}

void HashStringAndSearch() {
  std::set<uint32> data;
  for (int i = 0; i < FLAGS_size; ++i) {
    std::string data_i = GetData(i);
    CHECK(data.insert(CityHash32(data_i.data(), data_i.size())).second);
  }

  std::vector<std::string> source_data = GetRandomSource();

  LOG_WARNING << "Hash before searching";

  WallTimer timer;
  timer.Start();

  for (auto source : source_data) {
    uint32 hash = CityHash32(source.data(), source.size());
    std::set<uint32>::iterator dest = data.find(hash);
    CHECK(dest != data.end());
  }

  LOG_WARNING << "Run " << FLAGS_round << " in " << timer.ElapseInMs() << "ms";
}

void SearchString() {
  std::set<std::string> data;
  for (int i = 0; i < FLAGS_size; ++i) {
    CHECK(data.insert(GetData(i)).second);
  }

  std::vector<std::string> source_data = GetRandomSource();

  LOG_WARNING << "Search string directly";

  WallTimer timer;
  timer.Start();

  for (auto source : source_data) {
    std::set<std::string>::iterator dest = data.find(source);
    CHECK(dest != data.end());
  }

  LOG_WARNING << "Run " << FLAGS_round << " in " << timer.ElapseInMs() << "ms";
}

int main(int argc, char** argv) {
  HashStringAndSearch();
  SearchString();
  return 0;
}
