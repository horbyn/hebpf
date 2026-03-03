// clang-format off
#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/services/examples/hello/example_hello.h"
// clang-format on

using namespace hebpf::services::examples;

TEST(HelloEbpfTest, MoveConstructor) {
  ExampleHelloEbpf skeleton1{};
  ExampleHelloEbpf skeleton2{std::move(skeleton1)};
  EXPECT_FALSE(skeleton1);
}

TEST(HelloEbpfTest, MoveAssignment) {
  ExampleHelloEbpf skeleton1{};
  ExampleHelloEbpf skeleton2{};
  skeleton2 = std::move(skeleton1);
  EXPECT_FALSE(skeleton1);
}

TEST(HelloEbpfTest, IntegratedSuccess) {
  {
    ExampleHelloEbpf skeleton{};
    EXPECT_NO_THROW(skeleton.start());
    skeleton.stop();
    EXPECT_FALSE(skeleton);
  }
}

TEST(HelloEbpfTest, IntegratedFailure) {
  {
    auto *skel = example_hello_bpf::open();
    auto *obj = skel->skeleton->obj;
    auto *obj_elem0 = *(obj);
    enum bpf_object_state { OBJ_OPEN, OBJ_PREPARED, OBJ_LOADED, OBJ_INVALID };
    enum bpf_object_state *state = reinterpret_cast<enum bpf_object_state *>(
        reinterpret_cast<char *>(obj_elem0) + BPF_OBJ_NAME_LEN + 64 + sizeof(__u32));
    *state = OBJ_INVALID;
    ExampleHelloEbpf skeleton{std::unique_ptr<example_hello_bpf>(skel)};
    EXPECT_ANY_THROW(skeleton.start());
    skeleton.stop();
    EXPECT_FALSE(skeleton);
  }
}
