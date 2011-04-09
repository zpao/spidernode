#include "v8api_test_harness.h"

void
test_obj_setprop()
{
  HandleScope handle_scope;

  Persistent<Context> context = Context::New();

  Context::Scope context_scope(context);

  Handle<Object> obj = Object::New();

  Handle<String> k = String::New("someprop");
  Handle<String> v = String::New("somevalue");

  obj->Set(k, v);

  Handle<Value> v2 = obj->Get(k);

  String::AsciiValue vs(v);
  String::AsciiValue vs2(v2);

  do_check_eq(vs.length(), vs2.length());
  do_check_eq(0, strcmp(*vs,*vs2));

  context.Dispose();
}

void
test_obj_getprop()
{
  HandleScope handle_scope;

  Persistent<Context> context = Context::New();

  Context::Scope context_scope(context);

  Handle<Object> obj = Object::New();

  Handle<Value> k = String::New("toString");
  Handle<Value> v = obj->Get(k);

  do_check_true(v->IsFunction());

  context.Dispose();
}

static JSInt32 test_val = 1;

static Handle<Value> ReadTestVal(Local<String> propname, const AccessorInfo &info) {
  do_check_true(!propname.IsEmpty());
  return Integer::New(test_val);
}

static Handle<Value> WriteTestVal(Local<String> propname, Local<Value> v, const AccessorInfo &info) {
  do_check_true(!propname.IsEmpty());
  do_check_true(v->IsInt32());
  do_check_true(!info.Data().IsEmpty());
  do_check_true(info.Data()->IsInt32());
  test_val = v->Int32Value();
  int offset = info.Data()->Int32Value();
  return Integer::New(test_val += offset);
}

void
test_obj_defprop() {
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();

  Context::Scope context_scope(context);

  Handle<Object> obj = Object::New();
  Handle<Value> data = Integer::New(2);
  obj->SetAccessor(String::New("myprop"), ReadTestVal, WriteTestVal, data);
  Local<Object> global = context->Global();
  global->Set(String::New("testobj"), obj);

  Handle<String> source = String::New("var n = testobj.myprop; testobj.myprop = (n+9); testobj.myprop");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  // Run the script to get the result.
  Handle<Value> result = script->Run();

  do_check_true(!result.IsEmpty());
  do_check_true(result->IsInt32());
  JSInt32 i = result->Int32Value();
  do_check_eq(12, i);
  context.Dispose();
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_obj_setprop),
  TEST(test_obj_getprop),
  TEST(test_obj_defprop),
};

const char* file = __FILE__;
#define TEST_NAME "Object Properties"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
