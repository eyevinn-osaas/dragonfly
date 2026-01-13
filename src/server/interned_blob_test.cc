// Copyright 2025, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.

#include "core/json/detail/interned_blob.h"

#include "base/gtest.h"
#include "core/detail/stateless_allocator.h"
#include "core/mi_memory_resource.h"

namespace {

dfly::MiMemoryResource* MemoryResource() {
  thread_local mi_heap_t* heap = mi_heap_new();
  thread_local dfly::MiMemoryResource memory_resource{heap};
  return &memory_resource;
}

}  // namespace

using namespace std::literals;

class InternedBlobTest : public testing::Test {
 protected:
  void SetUp() override {
    dfly::InitTLStatelessAllocMR(MemoryResource());
  }
};

TEST_F(InternedBlobTest, MemoryUsage) {
  const auto* mr = MemoryResource();
  const auto usage_before = mr->used();
  {
    const auto blob = dfly::detail::InternedBlob{"1234567"};
    const auto usage_after = mr->used();
    // header size (4 bytes size + 4 bytes refcount) + strlen + 1 byte for nul char
    constexpr auto expected_delta = 8 + 7 + 1;
    EXPECT_EQ(usage_before + expected_delta, usage_after);
  }
  const auto usage_after = mr->used();
  EXPECT_EQ(usage_before, usage_after);
}

TEST_F(InternedBlobTest, Accessors) {
  const auto blob = dfly::detail::InternedBlob{"1234567"};
  EXPECT_EQ(blob.Size(), 7);
  EXPECT_STREQ(blob.Data(), "1234567");
  EXPECT_EQ(blob.View(), "1234567"sv);
}

TEST_F(InternedBlobTest, RefCounts) {
  auto blob = dfly::detail::InternedBlob{"1234567"};
  EXPECT_EQ(blob.RefCount(), 1);
  blob.IncrRefCount();
  blob.IncrRefCount();
  blob.IncrRefCount();
  EXPECT_EQ(blob.RefCount(), 4);
  blob.DecrRefCount();
  blob.DecrRefCount();
  blob.DecrRefCount();
  blob.DecrRefCount();
  EXPECT_EQ(blob.RefCount(), 0);
  EXPECT_DEATH(blob.DecrRefCount(), "Attempt to decrease zero refcount");
  blob.SetRefCount(std::numeric_limits<uint32_t>::max());
  EXPECT_DEATH(blob.IncrRefCount(), "Attempt to increase max refcount");
}

TEST_F(InternedBlobTest, Pool) {
  dfly::detail::InternedBlobPool pool{};
  const auto b1 = std::make_unique<dfly::detail::InternedBlob>("foo");
  pool.emplace(b1.get());

  // search by string view
  EXPECT_TRUE(pool.contains("foo"));

  // increment the refcount. The blob is still found because the hasher only looks at the string
  b1->IncrRefCount();
  b1->IncrRefCount();
  b1->IncrRefCount();

  EXPECT_TRUE(pool.contains("foo"));
}
