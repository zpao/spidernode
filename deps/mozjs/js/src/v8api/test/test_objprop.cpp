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

static int32_t test_val = 1;

static Handle<Value> ReadTestVal(Local<String> propname, const AccessorInfo &info) {
  do_check_true(!propname.IsEmpty());
  return Integer::New(test_val);
}

static void WriteTestVal(Local<String> propname, Local<Value> v, const AccessorInfo &info) {
  do_check_true(!propname.IsEmpty());
  do_check_true(v->IsInt32());
  do_check_true(!info.Data().IsEmpty());
  do_check_true(info.Data()->IsInt32());
  test_val = v->Int32Value();
  int offset = info.Data()->Int32Value();
  test_val += offset;
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
  int32_t i = result->Int32Value();
  do_check_eq(12, i);
  context.Dispose();
}

static Handle<Value> ReadExn(Local<String> propname, const AccessorInfo &info) {
  return ThrowException(Integer::New(9));
}

static void WriteExn(Local<String> propname, Local<Value> v, const AccessorInfo &info) {
  ThrowException(Integer::New(4));
}

void
test_obj_propexn() {
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();

  Context::Scope context_scope(context);

  Handle<Object> obj = Object::New();
  obj->SetAccessor(String::New("myprop"), ReadExn, WriteExn);
  Local<Object> global = context->Global();
  global->Set(String::New("testobj"), obj);

  Handle<String> source = String::New("var n = 0;"
                                      "try { testobj.myprop; } catch (e) { n += e; };"
                                      "try { testobj.myprop = (n+9); } catch (e) { n += e; }; n");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  TryCatch trycatch;
  // Run the script to get the result.
  Handle<Value> result = script->Run();

  do_check_false(result.IsEmpty());
  do_check_true(result->IsInt32());
  do_check_false(trycatch.HasCaught());
  int32_t i = result->Int32Value();
  do_check_eq(13, i);
  context.Dispose();
}

Handle<Value> NamedGetterThrows(Local<String> property, const AccessorInfo &info) {
  return ThrowException(Integer::New(1));
}
Handle<Value> NamedSetterThrows(Local<String> property, Local<Value> value, const AccessorInfo &info) {
  return ThrowException(Integer::New(2));
}
Handle<Boolean> NamedDeleterThrows(Local<String> property, const AccessorInfo &info) {
  ThrowException(Integer::New(3));
  return True();
}

Handle<Value> IndexedGetterThrows(uint32_t index, const AccessorInfo &info) {
  return ThrowException(Integer::New(4));
}
Handle<Value> IndexedSetterThrows(uint32_t index, Local<Value> value, const AccessorInfo &info) {
  return ThrowException(Integer::New(5));
}

Handle<Boolean> IndexedDeleterThrows(uint32_t index, const AccessorInfo &info) {
  ThrowException(Integer::New(6));
  return True();
}

void
test_obj_tmplexn() {
  HandleScope scope;

  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  Handle<ObjectTemplate> tmpl = ObjectTemplate::New();
  tmpl->SetNamedPropertyHandler(NamedGetterThrows, NamedSetterThrows, 0, NamedDeleterThrows);
  tmpl->SetIndexedPropertyHandler(IndexedGetterThrows, IndexedSetterThrows, 0, IndexedDeleterThrows);
  Handle<Object> obj = tmpl->NewInstance();
  Local<Object> global = context->Global();
  global->Set(String::New("testobj"), obj);

  Handle<String> source = String::New("var n = 0;"
                                      "try { testobj.myprop; } catch (e) { n += e; };"
                                      "try { testobj.myprop = (n+9); } catch (e) { n += e; }"
                                      "try { delete testobj.myprop; } catch (e) { n += e; };"
                                      "try { testobj[4]; } catch (e) { n += e; };"
                                      "try { testobj[5] = (n+9); } catch (e) { n += e; }"
                                      "try { delete testobj[6]; } catch (e) { n += e; }; n");

  // Compile the source code.
  Handle<Script> script = Script::Compile(source);

  TryCatch trycatch;
  // Run the script to get the result.
  Handle<Value> result = script->Run();

  do_check_false(result.IsEmpty());
  do_check_true(result->IsInt32());
  do_check_false(trycatch.HasCaught());
  int32_t i = result->Int32Value();
  do_check_eq(i, 21);
  context.Dispose();
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_obj_setprop),
  TEST(test_obj_getprop),
  TEST(test_obj_defprop),
  TEST(test_obj_propexn),
  TEST(test_obj_tmplexn),
};

const char* file = __FILE__;
#define TEST_NAME "Object Properties"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
