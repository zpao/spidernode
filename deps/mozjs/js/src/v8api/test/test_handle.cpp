/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include "v8api_test_harness.h"

////////////////////////////////////////////////////////////////////////////////
//// Tests

void
test_ArrayConversion()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  Local<String> s = String::New("hey");
  Local<String> uncaught_exception_symbol_l = Local<String>::New(s);
  Local<Value> argv[1] = { uncaught_exception_symbol_l  };
}

void
test_HandleScope() {
  HandleScope outer;
  Local<Value> v;
  {
    HandleScope inner;
    v = inner.Close(String::New("hey"));
  }
  do_check_true(!v.IsEmpty());
  Local<String> s = v->ToString();
  do_check_true(!s.IsEmpty());
  do_check_true(s->Equals(v));
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_ArrayConversion),
  TEST(test_HandleScope),
};

const char* file = __FILE__;
#define TEST_NAME "Handle Classes"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"

