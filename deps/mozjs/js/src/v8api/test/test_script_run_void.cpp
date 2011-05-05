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

  TryCatch trycatch;

  // Create a string containing the JavaScript source code.
  Handle<String> source = String::New("a b");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  do_check_true(script.IsEmpty());
  do_check_true(trycatch.HasCaught());

  // Dispose the persistent context.
  context.Dispose();
}

void
test_script_origin()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();

  Context::Scope context_scope(context);

  Handle<String> source = String::New("throw new Error;");
  ScriptOrigin origin(String::New("foo"), Integer::New(12));

  Handle<Script> script = Script::Compile(source, &origin);
  TryCatch trycatch;
  trycatch.SetCaptureMessage(true);

  Handle<Value> result = script->Run();
  do_check_true(result.IsEmpty());
  do_check_true(trycatch.HasCaught());
  do_check_true(trycatch.Exception()->IsObject());
  Handle<v8::Message> msg = trycatch.Message();
  do_check_true(!msg.IsEmpty());
  do_check_eq(12, msg->GetLineNumber());
  do_check_true(String::New("foo")->Equals(msg->GetScriptResourceName()));

  context.Dispose();
}
////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_script_run_void),
  TEST(test_script_err),
  TEST(test_script_origin),
};

const char* file = __FILE__;
#define TEST_NAME "Script Run Void"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
