// clang-format off
#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/services/hello/hello.h"
// clang-format on

using namespace hebpf::services::hello;

TEST(HelloEbpfTest, MoveConstructor) {
  HelloEbpf skeleton1{};
  HelloEbpf skeleton2{std::move(skeleton1)};
  EXPECT_FALSE(skeleton1);
}

TEST(HelloEbpfTest, MoveAssignment) {
  HelloEbpf skeleton1{};
  HelloEbpf skeleton2{};
  skeleton2 = std::move(skeleton1);
  EXPECT_FALSE(skeleton1);
}

TEST(HelloEbpfTest, IntegratedSuccess) {
  {
    HelloEbpf skeleton{};
    skeleton.open();
    EXPECT_NO_THROW(skeleton.load());
    EXPECT_NO_THROW(skeleton.attach());
    skeleton.detach();
    skeleton.destroy();
    EXPECT_FALSE(skeleton);
  }
  {
    HelloEbpf skeleton{std::unique_ptr<hello_bpf>(hello_bpf::open())};
    skeleton.open();
    skeleton.destroy();
    EXPECT_FALSE(skeleton);
  }
}

TEST(HelloEbpfTest, IntegratedFailure) {
  {
    auto *skel = hello_bpf::open();
    auto *obj = skel->skeleton->obj;
    auto *obj_elem0 = *(obj);
    enum bpf_object_state { OBJ_OPEN, OBJ_PREPARED, OBJ_LOADED, OBJ_INVALID };
    enum bpf_object_state *state = reinterpret_cast<enum bpf_object_state *>(
        reinterpret_cast<char *>(obj_elem0) + BPF_OBJ_NAME_LEN + 64 + sizeof(__u32));
    *state = OBJ_INVALID;
    HelloEbpf skeleton{std::unique_ptr<hello_bpf>(skel)};
    skeleton.open();
    EXPECT_ANY_THROW(skeleton.load());
    EXPECT_ANY_THROW(skeleton.attach());
    skeleton.destroy();
    EXPECT_FALSE(skeleton);
  }
}
