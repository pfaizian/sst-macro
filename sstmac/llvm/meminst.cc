#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include <numeric>

namespace {
struct AddrData {
  void *addr;
  int64_t size;
  int64_t thread_id;
};

std::mutex LoadMutex;
std::vector<AddrData> LoadAddrs;

std::mutex StoreMutex;
std::vector<AddrData> StoreAddrs;
} // namespace

extern "C" {
void sstmac_address_load(void *addr, int64_t size, int32_t thread_id) {
  std::lock_guard<std::mutex> lock(LoadMutex);
  LoadAddrs.push_back({addr, size, thread_id});
}

void sstmac_address_store(void *addr, int64_t size, int32_t thread_id) {
  std::lock_guard<std::mutex> lock(StoreMutex);
  StoreAddrs.push_back({addr, size, thread_id});
}

void sstmac_print_address_info() {
  auto stored = std::accumulate(
      StoreAddrs.begin(), StoreAddrs.end(), 0,
      [](int &val, AddrData const &a) { return val + a.size; });

  auto loaded = std::accumulate(
      LoadAddrs.begin(), LoadAddrs.end(), 0,
      [](int &val, AddrData const &a) { return val + a.size; });

  std::cout << "Number of stored addresses: " << stored << "\n";
  std::cout << "Number of loaded addresses: " << loaded << "\n";
}
}
