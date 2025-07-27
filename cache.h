#pragma once
#include <intrin.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class Cache {
 public:
  Cache(std::size_t table_size, std::size_t max_nodes)
      : table_size_(table_size),
        max_nodes_(max_nodes),
        table_(std::make_unique<Node *[]>(table_size)) {
    if (table_size == 0) {
      throw std::runtime_error("Zero table size");
    }
    // Surprisingly complicated code to figure out how many bits
    // are in the largest index for the given table size.
    unsigned long table_bits;
    if (table_size == 1) {
      // The only index is zero. We don't need no stinkin bits.
      table_bits = 0;
    } else {
       // table_size - 1 is the max index.
      _BitScanReverse64(&table_bits, table_size - 1);

      // Convert from zero-based to 1-based bit indices.
      ++table_bits;
    }

    // In the hash function, we shift away all but the most significant
    // table_bits.
    hash_shift_ = 64 - table_bits;
  }

  ~Cache();

  std::optional<std::size_t> Lookup(std::uint64_t key1, std::uint64_t key2);
  void Insert(std::uint64_t key1, std::uint64_t key2, std::size_t value);

  std::size_t size() const { return num_nodes_; }

  // For debugging
  std::vector<std::pair<std::uint64_t, std::uint64_t>> LruOrder();
  int hash_shift() const { return hash_shift_; }

 private:
  // Returns a value in the range [0, kHashSize).
  std::size_t HashKeys(std::uint64_t key1, std::uint64_t key2);

  struct Node {
    std::uint64_t key1;
    std::uint64_t key2;
    std::size_t value;

    Node *bucket_next;
    Node **bucket_prev;

    // Circulary linked list.
    Node *lru_next;
    Node *lru_prev;
  };

  // Searches for the key without modifying the LRU list.
  Node *ReadonlyLookup(std::uint64_t key1, std::uint64_t key2);

  const std::size_t table_size_;
  const std::size_t max_nodes_;
  const std::unique_ptr<Node *[]> table_;
  int hash_shift_;

  // The next node to be replaced.
  Node *lru_tail_ = nullptr;

  std::size_t num_nodes_ = 0;
};
