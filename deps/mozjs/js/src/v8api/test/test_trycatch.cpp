/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include "v8api_test_harness.h"

void
test_nested()
{
  HandleScope handle_scope;
  TryCatch trycatch;

  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  Handle<String> source = String::New("oasdohuasdnlqwoi");
  Handle<Script> script = Script::Compile(source);
  {
    TryCatch innerCatcher;
    Handle<Value> result = script->Run();

    do_check_true(innerCatcher.HasCaught());
  }
  context.Dispose();

  do_check_true(!trycatch.HasCaught());
}

void
test_rethrow()
{
  HandleScope handle_scope;
  TryCatch trycatch;

  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  Handle<String> source = String::New("oasdohuasdnlqwoi");
  Handle<Script> script = Script::Compile(source);
  {
    TryCatch innerCatcher;
    Handle<Value> result = script->Run();

    do_check_true(innerCatcher.HasCaught());

    innerCatcher.ReThrow();
  }
  context.Dispose();

  do_check_true(trycatch.HasCaught());
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_nested),
  TEST(test_rethrow),
};

const char* file = __FILE__;
#define TEST_NAME "TryCatch class"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
