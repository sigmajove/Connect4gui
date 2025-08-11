#if 0
#include "cache.h"

#include <format>
#include <iostream>
#include <stdexcept>

// Deletes all the nodes.
Cache::~Cache() {
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

// Here is some simple code recommended by Perplexity for hashing
// two 64-bit integers.

// Mixes the 64 bits in x by multiplying by the fractional part of the
// Golden Ratio.
std::uint64_t GoldenHash(std::uint64_t x) { return 0x9e3779b97f4a7c13 * x; }

std::size_t Cache::HashKeys(std::uint64_t key1, std::uint64_t key2) {
  return (GoldenHash((GoldenHash(key1) & 0xFFFFFFFF00000000) |
                     (GoldenHash(key2) >> 32)) >>
          hash_shift_) %
         table_size_;
}

Cache::Node *Cache::ReadonlyLookup(std::uint64_t key1, std::uint64_t key2) {
  Node *p = table_[HashKeys(key1, key2)];
  while (p != nullptr) {
    if (p->key1 == key1 && p->key2 == key2) {
      break;
    }
    p = p->bucket_next;
  }
  return p;
}

std::optional<std::size_t> Cache::Lookup(std::uint64_t key1,
                                         std::uint64_t key2) {
  Node *p = table_[HashKeys(key1, key2)];
  while (p != nullptr) {
    if (p->key1 == key1 && p->key2 == key2) {
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

void Cache::Insert(std::uint64_t key1, std::uint64_t key2, std::size_t value) {
  Node **pred = &(table_[HashKeys(key1, key2)]);
  Node *p = *pred;
  while (p != nullptr) {
    if (p->key1 == key1 && p->key2 == key2) {
      throw std::runtime_error("Key is already present");
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

  n->key1 = key1;
  n->key2 = key2;
  n->value = value;
  n->bucket_next = nullptr;
  n->bucket_prev = pred;
  *pred = n;
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> Cache::LruOrder() {
  std::vector<std::pair<std::uint64_t, std::uint64_t>> result;
  result.reserve(num_nodes_);
  if (lru_tail_ != nullptr) {
    const Node *prev = lru_tail_;
    const Node *p = lru_tail_->lru_next;
    for (;;) {
      if (p->lru_prev != prev) {
        throw std::runtime_error("malformed list");
      }
      result.emplace_back(p->key1, p->key2);
      if (ReadonlyLookup(p->key1, p->key2) != p) {
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
};
#endif
