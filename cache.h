#pragma once
#include <intrin.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// Mixes the 64 bits in x by multiplying by the fractional part of the
// Golden Ratio.
inline std::uint64_t GoldenHash(std::uint64_t x) {
  return 0x9e3779b97f4a7c13 * x;
}

template <class Key, class Value>
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

  ~Cache() {
    if (lru_tail_ != nullptr) {
      // Awkward traversal that never touches a deleted node.
      // Initialize p to the head of the list.
      Node *p = lru_tail_->lru_prev;
      for (;;) {
        if (p == lru_tail_) {
          delete p;
          return;
        }
        Node *const next = p->lru_next;
        delete p;
        p = next;
      }
    }
  }

  std::optional<Value> Lookup(Key key) {
    Node *p = table_[HashKeys(key)];
    while (p != nullptr) {
      if (p->key == key) {
        if (lru_tail_ == p) {
          lru_tail_ = p->lru_prev;
        } else {
          // Delete p from the lru list
          Node *const prev = p->lru_prev;
          Node *const next = p->lru_next;
          prev->lru_next = next;
          next->lru_prev = prev;

          // p has been completely removed.
          // Now insert it after the tail.
          Node *const succ = lru_tail_->lru_next;
          p->lru_prev = lru_tail_;
          lru_tail_->lru_next = p;
          p->lru_next = succ;
          succ->lru_prev = p;
        }
        return p->value;
      }
      p = p->bucket_next;
    }
    return std::nullopt;
  }

  // Finds or Creates an entry for key
  // Returns a pointer to the value associated with key1, key2.
  // If newly created, it will be the default value for Value.
  // Warning: Do not hold the returned pointer past subsequent
  // calls to GetOrAdd.
  Value *GetOrAdd(Key key) {
    Node **pred = &(table_[HashKeys(key)]);
    Node *p = *pred;
    while (p != nullptr) {
      if (p->key == key) {
        return &(p->value);
      }
      pred = &(p->bucket_next);
      p = *pred;
    }

    Node *n;
    if (num_nodes_ >= max_nodes_) {
      // Repurpose the node at the end of the LRU list.
      n = lru_tail_;
      lru_tail_ = lru_tail_->lru_prev;

      Node *const next = n->bucket_next;
      *n->bucket_prev = next;
      if (next != nullptr) {
        next->bucket_prev = n->bucket_prev;
      }
    } else {
      n = new Node;
      ++num_nodes_;

      if (lru_tail_ == nullptr) {
        n->lru_next = n;
        n->lru_prev = n;
        lru_tail_ = n;
      } else {
        Node *const succ = lru_tail_->lru_next;
        // Insert n between lru_tail_ and succ
        n->lru_prev = lru_tail_;
        lru_tail_->lru_next = n;

        n->lru_next = succ;
        succ->lru_prev = n;
      }
    }

    n->key = key;
    n->bucket_next = nullptr;
    n->bucket_prev = pred;
    n->id = ++node_counter_;
    *pred = n;
    return &(n->value);
  }

  std::size_t size() const { return num_nodes_; }

  // For debugging
  std::vector<Key> LruOrder() {
    std::vector<Key> result;
    result.reserve(num_nodes_);
    if (lru_tail_ != nullptr) {
      const Node *prev = lru_tail_;
      const Node *p = lru_tail_->lru_next;
      for (;;) {
        if (p->lru_prev != prev) {
          throw std::runtime_error("malformed list");
        }
        result.emplace_back(p->key);
        if (ReadonlyLookup(p->key) != p) {
          throw std::runtime_error("item missing from table");
        }

        if (result.size() > num_nodes_) {
          throw std::runtime_error("loop in lru list");
        }
        if (p == lru_tail_) {
          break;
        }
        prev = p;
        p = p->lru_next;
      }
    }
    return result;
  }

  int hash_shift() const { return hash_shift_; }

 private:
  struct Node {
    Key key;
    Value value;

    Node *bucket_next;
    Node **bucket_prev;

    // Circulary linked list.
    Node *lru_next;
    Node *lru_prev;

    std::size_t id;
  };

  // Returns a value in the range [0, table_size).
  std::size_t HashKeys(Key key) { return key.hash() % table_size_; }

  // Searches for the key without modifying the LRU list.
  Node *ReadonlyLookup(Key key) {
    Node *p = table_[HashKeys(key)];
    while (p != nullptr) {
      if (p->key == key) {
        break;
      }
      p = p->bucket_next;
    }
    return p;
  }

  std::size_t node_counter_ = 0;
  const std::size_t table_size_;
  const std::size_t max_nodes_;
  const std::unique_ptr<Node *[]> table_;
  int hash_shift_;

  // The next node to be replaced.
  Node *lru_tail_ = nullptr;

  std::size_t num_nodes_ = 0;
};
