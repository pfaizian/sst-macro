#include <cstdint>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

namespace {

int TracingDepth = 0;
uint64_t MaxAddrs = 1000000;

struct AddrData {
  void *addr;
  int64_t size;
  int64_t thread_id;
};

std::mutex Mutex;
std::vector<AddrData> LoadAddrs;
std::vector<AddrData> StoreAddrs;

// For now do not call outside of a locked region
void sstmac_flush_addresses() {}
} // namespace

extern "C" {

void sstmac_address_load(void *addr, int64_t size, int32_t thread_id) {
  std::lock_guard<std::mutex> lock(Mutex);
  if (TracingDepth > 0) {
    LoadAddrs.push_back({addr, size, thread_id});
  }
  if (LoadAddrs.size() > MaxAddrs) {
    sstmac_flush_addresses();
  }
}

void sstmac_address_store(void *addr, int64_t size, int32_t thread_id) {
  std::lock_guard<std::mutex> lock(Mutex);
  if (TracingDepth > 0) {
    StoreAddrs.push_back({addr, size, thread_id});
  }
  if (StoreAddrs.size() > MaxAddrs) {
    sstmac_flush_addresses();
  }
}

void sstmac_start_trace() {
  std::lock_guard<std::mutex> lock(Mutex);
  ++TracingDepth;
}

void sstmac_end_trace() {
  std::lock_guard<std::mutex> lock(Mutex);
  --TracingDepth;
  if (TracingDepth == 0) {
    sstmac_flush_addresses();
  }
}

void sstmac_print_address_info() {
  auto stored =
      std::accumulate(StoreAddrs.begin(), StoreAddrs.end(), 0,
                      [](int &val, AddrData const &a) { return val + a.size; });

  auto loaded =
      std::accumulate(LoadAddrs.begin(), LoadAddrs.end(), 0,
                      [](int &val, AddrData const &a) { return val + a.size; });

  std::cout << "Number of stored addresses: " << stored << "\n";
  std::cout << "Number of loaded addresses: " << loaded << "\n";
}
}
