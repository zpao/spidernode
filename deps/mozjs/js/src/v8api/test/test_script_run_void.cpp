#include "v8api_test_harness.h"

void
test_script_run_void()
{
  // Create a stack-allocated handle scope.
  HandleScope handle_scope;

  TryCatch trycatch;

  // Create a new context.
  Persistent<Context> context = Context::New();

  // Enter the created context for compiling.
  Context::Scope context_scope(context);

  // Create a string containing the JavaScript source code.
  Handle<String> source = String::New("oasdohuasdnlqwoi");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  // Run the script to get the result.
  Handle<Value> result = script->Run();

  do_check_true(trycatch.HasCaught());

  // Dispose the persistent context.
  context.Dispose();

  do_check_true(result.IsEmpty());
}

void
test_script_err()
{
  // Create a stack-allocated handle scope.
  HandleScope handle_scope;

  // Create a new context.
  Persistent<Context> context = Context::New();

  // Enter the created context for compiling.
  Context::Scope context_scope(context);

  // Create a string containing the JavaScript source code.
  Handle<String> source = String::New("a b");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  do_check_true(script.IsEmpty());

  // Dispose the persistent context.
  context.Dispose();
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_script_run_void),
  TEST(test_script_err),
};

const char* file = __FILE__;
#define TEST_NAME "Script Run Void"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
