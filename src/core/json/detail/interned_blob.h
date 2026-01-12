// Copyright 2025, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string_view>

namespace dfly::detail {
class InternedBlob {
  static constexpr auto kHeaderSize = sizeof(uint32_t) * 2;

 public:
  // TODO - does this need to accept an allocator??
  explicit InternedBlob(std::string_view sv);
  ~InternedBlob();

  InternedBlob(const InternedBlob& other) = delete;
  InternedBlob& operator=(const InternedBlob& other) = delete;

  InternedBlob(InternedBlob&& other) noexcept;
  InternedBlob& operator=(InternedBlob&& other) noexcept;

  uint32_t Size() const;

  uint32_t RefCount() const;

  std::string_view View() const;

  const char* Data() const;

  void IncrRefCount();

  void DecrRefCount();

 private:
  void Destroy();

  // Layout is: 4 bytes size, 4 bytes refcount, char data, followed by nul-char.
  // The trailing nul-char is required because jsoncons needs to access c_str/data without a
  // size. The blob_ itself points directly to the data, so that callers do not have to perform
  // pointer arithmetic for c_str() and data() calls:
  //     [size:4] [refcount:4] [string] [\0]
  //     ^-8      ^- 4         ^blob_
  char* blob_ = nullptr;
};

struct BlobHash {
  // allow heterogeneous lookup, so there are no conversions from string_view to InternedBlob:
  // https://abseil.io/tips/144
  using is_transparent = void;
  size_t operator()(const InternedBlob*) const;
  size_t operator()(std::string_view) const;
};

struct BlobEq {
  using is_transparent = void;
  bool operator()(const InternedBlob* a, const InternedBlob* b) const;
  bool operator()(const InternedBlob* a, std::string_view b) const;
  bool operator()(std::string_view a, const InternedBlob* b) const;
};

// This pool holds blob pointers and is used by InternedString to manage string access.
using InternedBlobPool = absl::flat_hash_set<InternedBlob*, BlobHash, BlobEq>;
static_assert(sizeof(InternedBlob) == sizeof(char*));

}  // namespace dfly::detail
