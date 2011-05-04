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

Handle<Value> Throws(const Arguments& args) {
  return ThrowException(args[0]);
}

void
test_native() {
  HandleScope handle_scope;
  Handle<ObjectTemplate> templ = ObjectTemplate::New();
  Handle<FunctionTemplate> fnT = v8::FunctionTemplate::New(Throws);
  templ->Set("Throws", fnT);
  fnT->SetClassName(String::NewSymbol("Throws"));

  Persistent<Context> context = Context::New(NULL, templ);
  Handle<Script> script = Script::New(String::New("Throws(9); 4"));

  Context::Scope scope(context);
  TryCatch trycatch;
  Handle<Value> v = script->Run();
  do_check_true(v.IsEmpty());
  do_check_true(trycatch.HasCaught());
  Handle<Value> exn = trycatch.Exception();
  do_check_true(exn->IsInt32());
  do_check_eq(exn->Int32Value(), 9);
}

void
test_native2() {
  HandleScope handle_scope;
  Handle<ObjectTemplate> templ = ObjectTemplate::New();
  Handle<FunctionTemplate> fnT = v8::FunctionTemplate::New(Throws);
  templ->Set("Throws", fnT);
  fnT->SetClassName(String::NewSymbol("Throws"));

  Persistent<Context> context = Context::New(NULL, templ);
  Handle<Script> script = Script::New(String::New("try { Throws(9); 4 } catch (e) { e }"));

  Context::Scope scope(context);
  TryCatch trycatch;
  Handle<Value> v = script->Run();
  do_check_true(!v.IsEmpty());
  do_check_true(v->IsInt32());
  do_check_eq(v->Int32Value(), 9);
  do_check_true(!trycatch.HasCaught());
}

void
test_jsthrows() {
  HandleScope handle_scope;

  Persistent<Context> context = Context::New();
  Handle<Script> script = Script::New(String::New("throw 9;"));

  Context::Scope scope(context);
  TryCatch trycatch;
  Handle<Value> v = script->Run();
  do_check_true(v.IsEmpty());
  do_check_true(trycatch.HasCaught());
  Handle<Value> exn = trycatch.Exception();
  do_check_true(exn->IsInt32());
  do_check_eq(exn->Int32Value(), 9);
}
////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_nested),
  TEST(test_rethrow),
  TEST(test_native),
  TEST(test_native2),
  TEST(test_jsthrows),
};

const char* file = __FILE__;
#define TEST_NAME "TryCatch class"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
