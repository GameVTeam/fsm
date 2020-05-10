//
// Created by fanghr on 2020/5/10.
//

#include <type_traits>

#include <gtest/gtest.h>
#include <fsm/fsm.h>

class TraitsTestSuite : public testing::Test {};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

using namespace fsm::detail;

TEST_F(TraitsTestSuite, TestConcat) {
  ASSERT_TRUE((std::is_same<List<>, Concat<>::Type>::value));
  ASSERT_TRUE((std::is_same<List<int>, Concat<int, List<>>::Type>::value));
  ASSERT_TRUE((std::is_same<List<int, char>, Concat<int, List<char>>::Type>::value));
}

TEST_F(TraitsTestSuite, TestFilter) {
  ASSERT_TRUE((std::is_same<List<>, Filter<std::is_integral>::Type>::value));
  ASSERT_TRUE((std::is_same<List<>, Filter<std::is_integral, float>::Type>::value));
  ASSERT_TRUE((std::is_same<List<int>, Filter<std::is_integral, int, float>::Type>::value));
  ASSERT_TRUE((std::is_same<List<int>, Filter<std::is_integral, float, int>::Type>::value));
  ASSERT_TRUE((std::is_same<List<int, long>, Filter<std::is_integral, float, int, long>::Type>::value));
  ASSERT_TRUE((std::is_same<List<long, int>, Filter<std::is_integral, long, int>::Type>::value));
}

static void FooFunc() {}
struct Foo {
  void operator()() {}
  void operator()(int) {}
  void operator()(char *) {}
  static void Func() {}
};

TEST_F(TraitsTestSuite, TestIsInvocable) {
  ASSERT_TRUE((IsInvocableType<Foo>::value));
  ASSERT_TRUE((IsInvocableType<Foo, int>::value));
  ASSERT_TRUE((IsInvocableType<Foo, int &>::value));
  ASSERT_TRUE((IsInvocableType<Foo, const int &>::value));
  ASSERT_TRUE((IsInvocableType<Foo, long>::value));
  ASSERT_TRUE((IsInvocableType<Foo, char *>::value));
  ASSERT_TRUE((IsInvocableType<decltype(FooFunc)>::value));
  ASSERT_TRUE((IsInvocableType<decltype(Foo::Func)>::value));

  ASSERT_FALSE((IsInvocableType<Foo, int *>::value));
  ASSERT_FALSE((IsInvocableType<Foo, const char *>::value));
  ASSERT_FALSE((IsInvocableType<Foo, int, const char *>::value));
  ASSERT_FALSE((IsInvocableType<Foo, void *>::value));
}
#pragma clang diagnostic pop