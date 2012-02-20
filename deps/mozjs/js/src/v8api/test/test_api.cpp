// Copyright 2007-2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * This test file is an adaptation of test-api.cc from the v8 source tree.  We
 * would need to import a whole bunch of their source files to just drop in that
 * test file, which we don't really want to do.
 */

#include "v8api_test_harness.h"
#include "v8api_test_checks.h"

////////////////////////////////////////////////////////////////////////////////
//// Fake Internal Namespace
namespace v8 {
namespace internal {
  template <typename T>
  static T* NewArray(int size) {
    T* result = new T[size];
    do_check_true(result);
    return result;
  }


  template <typename T>
  static void DeleteArray(T* array) {
    delete[] array;
  }

  struct Heap {
    static void CollectAllGarbage(bool force_compaction) {
      JS_GC(cx());
    }
  };

  struct OS {
    static double nan_value() {
      return JSVAL_TO_DOUBLE(JS_GetNaNValue(cx()));
    }
  };

  static size_t StrLength(const char *s) {
    return strlen(s);
  }
} // namespace internal
} // namespace v8

////////////////////////////////////////////////////////////////////////////////
//// Helpers

// V8 uses this to context switch when running their thread fuzzing tests
struct ApiTestFuzzer {
  static void Fuzz() { }
};

// our own version, not copied from test-api.cc
static bool IsNaN(double x) {
  // Hack: IEEE floating point math says that NaN != NaN.
  volatile double d = x;
  return d != d;
}


static inline v8::Local<v8::Value> v8_num(double x) {
  return v8::Number::New(x);
}
static inline v8::Local<v8::String> v8_str(const char* x) {
  return v8::String::New(x);
}

static inline v8::Local<v8::Script> v8_compile(const char* x) {
  return v8::Script::Compile(v8_str(x));
}

// Helper function that compiles and runs the source.
static inline v8::Local<v8::Value> CompileRun(const char* source) {
  return v8::Script::Compile(v8::String::New(source))->Run();
}

static void ExpectString(const char* code, const char* expected) {
  Local<Value> result = CompileRun(code);
  CHECK(result->IsString());
  String::AsciiValue ascii(result);
  CHECK_EQ(expected, *ascii);
}

static void ExpectBoolean(const char* code, bool expected) {
  Local<Value> result = CompileRun(code);
  CHECK(result->IsBoolean());
  CHECK_EQ(expected, result->BooleanValue());
}

static void ExpectTrue(const char* code) {
  ExpectBoolean(code, true);
}

static void ExpectFalse(const char* code) {
  ExpectBoolean(code, false);
}

static void ExpectObject(const char* code, Local<Value> expected) {
  Local<Value> result = CompileRun(code);
  CHECK(result->Equals(expected));
}

void CheckProperties(v8::Handle<v8::Value> val, int elmc, const char* elmv[]) {
  v8::Handle<v8::Object> obj = val.As<v8::Object>();
  v8::Handle<v8::Array> props = obj->GetPropertyNames();
  CHECK_EQ(elmc, props->Length());
  for (int i = 0; i < elmc; i++) {
    v8::String::Utf8Value elm(props->Get(v8::Integer::New(i)));
    CHECK_EQ(elmv[i], *elm);
  }
}

static void ExpectUndefined(const char* code) {
  Local<Value> result = CompileRun(code);
  CHECK(result->IsUndefined());
}

static int StrCmp16(uint16_t* a, uint16_t* b) {
  while (true) {
    if (*a == 0 && *b == 0) return 0;
    if (*a != *b) return 0 + *a - *b;
    a++;
    b++;
  }
}

static int StrNCmp16(uint16_t* a, uint16_t* b, int n) {
  while (true) {
    if (n-- == 0) return 0;
    if (*a == 0 && *b == 0) return 0;
    if (*a != *b) return 0 + *a - *b;
    a++;
    b++;
  }
}

static v8::Handle<Value> GetFlabby(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8_num(17.2);
}

static v8::Handle<Value> GetKnurd(Local<String> property, const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  return v8_num(15.2);
}

v8::Persistent<Value> xValue;

static void SetXValue(Local<String> name,
                      Local<Value> value,
                      const AccessorInfo& info) {
  CHECK_EQ(value, v8_num(4));
  CHECK_EQ(info.Data(), v8_str("donut"));
  CHECK_EQ(name, v8_str("x"));
  CHECK(xValue.IsEmpty());
  xValue = v8::Persistent<Value>::New(value);
}

static v8::Handle<Value> GetXValue(Local<String> name,
                                   const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(info.Data(), v8_str("donut"));
  CHECK_EQ(name, v8_str("x"));
  return name;
}

static int signature_callback_count;

static v8::Handle<Value> IncrementingSignatureCallback(
    const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  signature_callback_count++;
  v8::Handle<v8::Array> result = v8::Array::New(args.Length());
  for (int i = 0; i < args.Length(); i++)
    result->Set(v8::Integer::New(i), args[i]);
  return result;
}

static v8::Handle<Value> SignatureCallback(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  v8::Handle<v8::Array> result = v8::Array::New(args.Length());
  for (int i = 0; i < args.Length(); i++) {
    result->Set(v8::Integer::New(i), args[i]);
  }
  return result;
}

v8::Handle<Value> ThrowFromC(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8::ThrowException(v8_str("konto"));
}

v8::Handle<Value> CCatcher(const v8::Arguments& args) {
  if (args.Length() < 1) return v8::Boolean::New(false);
  v8::HandleScope scope;
  v8::TryCatch try_catch;
  Local<Value> result = v8::Script::Compile(args[0]->ToString())->Run();
  CHECK(!try_catch.HasCaught() || result.IsEmpty());
  return v8::Boolean::New(try_catch.HasCaught());
}

v8::Handle<v8::Value> WithTryCatch(const v8::Arguments& args) {
  v8::TryCatch try_catch;
  return v8::Undefined();
}

v8::Persistent<v8::Object> some_object;
v8::Persistent<v8::Object> bad_handle;

void NewPersistentHandleCallback(v8::Persistent<v8::Value> handle, void*) {
  v8::HandleScope scope;
  bad_handle = v8::Persistent<v8::Object>::New(some_object);
  handle.Dispose();
}

v8::Persistent<v8::Object> to_be_disposed;

void DisposeAndForceGcCallback(v8::Persistent<v8::Value> handle, void*) {
  to_be_disposed.Dispose();
  i::Heap::CollectAllGarbage(false);
  handle.Dispose();
}

void DisposingCallback(v8::Persistent<v8::Value> handle, void*) {
  handle.Dispose();
}

void HandleCreatingCallback(v8::Persistent<v8::Value> handle, void*) {
  v8::HandleScope scope;
  v8::Persistent<v8::Object>::New(v8::Object::New());
  handle.Dispose();
}

static v8::Handle<Value> GetterWhichReturns42(Local<String> name,
                                              const AccessorInfo& info) {
  return v8_num(42);
}

static void SetterWhichSetsYOnThisTo23(Local<String> name,
                                       Local<Value> value,
                                       const AccessorInfo& info) {
  info.This()->Set(v8_str("y"), v8_num(23));
}

static v8::Handle<Value> ShadowFunctionCallback(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8_num(42);
}

static int shadow_y;
static int shadow_y_setter_call_count;
static int shadow_y_getter_call_count;

static void ShadowYSetter(Local<String>, Local<Value>, const AccessorInfo&) {
  shadow_y_setter_call_count++;
  shadow_y = 42;
}

static v8::Handle<Value> ShadowYGetter(Local<String> name,
                                       const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  shadow_y_getter_call_count++;
  return v8_num(shadow_y);
}

static v8::Handle<Value> ShadowIndexedGet(uint32_t index,
                                          const AccessorInfo& info) {
  return v8::Handle<Value>();
}

static v8::Handle<Value> ShadowNamedGet(Local<String> key,
                                        const AccessorInfo&) {
  return v8::Handle<Value>();
}

static v8::Handle<Value> InstanceFunctionCallback(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8_num(12);
}

static v8::Handle<Value>
GlobalObjectInstancePropertiesGet(Local<String> key, const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  return v8::Handle<Value>();
}

static void CheckUncle(v8::TryCatch* try_catch) {
  CHECK(try_catch->HasCaught());
  String::AsciiValue str_value(try_catch->Exception());
  CHECK_EQ(*str_value, "uncle?");
  try_catch->Reset();
}

static v8::Handle<Value> FunctionNameCallback(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8_num(42);
}

// A LocalContext holds a reference to a v8::Context.
class LocalContext {
 public:
  LocalContext(v8::ExtensionConfiguration* extensions = 0,
               v8::Handle<v8::ObjectTemplate> global_template =
                   v8::Handle<v8::ObjectTemplate>(),
               v8::Handle<v8::Value> global_object = v8::Handle<v8::Value>())
    : context_(v8::Context::New(extensions, global_template, global_object)) {
    context_->Enter();
  }

  virtual ~LocalContext() {
    context_->Exit();
    context_.Dispose();
  }

  v8::Context* operator->() { return *context_; }
  v8::Context* operator*() { return *context_; }
  bool IsReady() { return !context_.IsEmpty(); }

  v8::Local<v8::Context> local() {
    return v8::Local<v8::Context>::New(context_);
  }

 private:
  v8::Persistent<v8::Context> context_;
};

v8::Handle<Value> HandleF(const v8::Arguments& args) {
  v8::HandleScope scope;
  ApiTestFuzzer::Fuzz();
  Local<v8::Array> result = v8::Array::New(args.Length());
  for (int i = 0; i < args.Length(); i++)
    result->Set(i, args[i]);
  return scope.Close(result);
}

static v8::Handle<v8::Object> GetGlobalProperty(LocalContext* context,
                                                char const* name) {
  return v8::Handle<v8::Object>::Cast((*context)->Global()->Get(v8_str(name)));
}

static v8::Handle<Value> Get239Value(Local<String> name,
                                     const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(info.Data(), v8_str("donut"));
  CHECK_EQ(name, v8_str("239"));
  return name;
}

static v8::Handle<Value> XPropertyGetter(Local<String> property,
                                         const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.Data()->IsUndefined());
  return property;
}

static v8::Handle<Value> IdentityIndexedPropertyGetter(
    uint32_t index,
    const AccessorInfo& info) {
  return v8::Integer::NewFromUnsigned(index);
}

static v8::Handle<Value> HandleLogDelegator(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8::Undefined();
}

v8::Handle<Function> args_fun;
static v8::Handle<Value> ArgumentsTestCallback(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(args_fun, args.Callee());
  CHECK_EQ(3, args.Length());
  CHECK_EQ(v8::Integer::New(1), args[0]);
  CHECK_EQ(v8::Integer::New(2), args[1]);
  CHECK_EQ(v8::Integer::New(3), args[2]);
  CHECK_EQ(v8::Undefined(), args[3]);
  v8::HandleScope scope;
  i::Heap::CollectAllGarbage(false);
  return v8::Undefined();
}

static v8::Handle<Value> NoBlockGetterX(Local<String> name,
                                        const AccessorInfo&) {
  return v8::Handle<Value>();
}

static v8::Handle<Value> NoBlockGetterI(uint32_t index,
                                        const AccessorInfo&) {
  return v8::Handle<Value>();
}

static v8::Handle<v8::Boolean> PDeleter(Local<String> name,
                                        const AccessorInfo&) {
  if (!name->Equals(v8_str("foo"))) {
    return v8::Handle<v8::Boolean>();  // not intercepted
  }

  return v8::False();  // intercepted, and don't delete the property
}

static v8::Handle<v8::Boolean> IDeleter(uint32_t index, const AccessorInfo&) {
  if (index != 2) {
    return v8::Handle<v8::Boolean>();  // not intercepted
  }

  return v8::False();  // intercepted, and don't delete the property
}

static v8::Handle<Value> GetK(Local<String> name, const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  if (name->Equals(v8_str("foo")) ||
      name->Equals(v8_str("bar")) ||
      name->Equals(v8_str("baz"))) {
    return v8::Undefined();
  }
  return v8::Handle<Value>();
}

static v8::Handle<Value> IndexedGetK(uint32_t index, const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  if (index == 0 || index == 1) return v8::Undefined();
  return v8::Handle<Value>();
}

static v8::Handle<v8::Array> NamedEnum(const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  v8::Handle<v8::Array> result = v8::Array::New(3);
  result->Set(v8::Integer::New(0), v8_str("foo"));
  result->Set(v8::Integer::New(1), v8_str("bar"));
  result->Set(v8::Integer::New(2), v8_str("baz"));
  return result;
}

static v8::Handle<v8::Array> IndexedEnum(const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  v8::Handle<v8::Array> result = v8::Array::New(2);
  result->Set(v8::Integer::New(0), v8_str("0"));
  result->Set(v8::Integer::New(1), v8_str("1"));
  return result;
}

int p_getter_count;
int p_getter_count2;

static v8::Handle<Value> PGetter(Local<String> name, const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  p_getter_count++;
  v8::Handle<v8::Object> global = Context::GetCurrent()->Global();
  CHECK_EQ(info.Holder(), global->Get(v8_str("o1")));
  if (name->Equals(v8_str("p1"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o1")));
  } else if (name->Equals(v8_str("p2"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o2")));
  } else if (name->Equals(v8_str("p3"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o3")));
  } else if (name->Equals(v8_str("p4"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o4")));
  }
  return v8::Undefined();
}

static void RunHolderTest(v8::Handle<v8::ObjectTemplate> obj) {
  ApiTestFuzzer::Fuzz();
  LocalContext context;
  context->Global()->Set(v8_str("o1"), obj->NewInstance());
  CompileRun(
    "o1.__proto__ = { };"
    "var o2 = { __proto__: o1 };"
    "var o3 = { __proto__: o2 };"
    "var o4 = { __proto__: o3 };"
    "for (var i = 0; i < 10; i++) o4.p4;"
    "for (var i = 0; i < 10; i++) o3.p3;"
    "for (var i = 0; i < 10; i++) o2.p2;"
    "for (var i = 0; i < 10; i++) o1.p1;");
}

static v8::Handle<Value> PGetter2(Local<String> name,
                                  const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  p_getter_count2++;
  v8::Handle<v8::Object> global = Context::GetCurrent()->Global();
  CHECK_EQ(info.Holder(), global->Get(v8_str("o1")));
  if (name->Equals(v8_str("p1"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o1")));
  } else if (name->Equals(v8_str("p2"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o2")));
  } else if (name->Equals(v8_str("p3"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o3")));
  } else if (name->Equals(v8_str("p4"))) {
    CHECK_EQ(info.This(), global->Get(v8_str("o4")));
  }
  return v8::Undefined();
}

static v8::Handle<Value> YGetter(Local<String> name, const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  return v8_num(10);
}

static void YSetter(Local<String> name,
                    Local<Value> value,
                    const AccessorInfo& info) {
  if (info.This()->Has(name)) {
    info.This()->Delete(name);
  }
  info.This()->Set(name, value);
}

int echo_named_call_count;

static v8::Handle<Value> EchoNamedProperty(Local<String> name,
                                           const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(v8_str("data"), info.Data());
  echo_named_call_count++;
  return name;
}

int echo_indexed_call_count = 0;

static v8::Handle<Value> EchoIndexedProperty(uint32_t index,
                                             const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(v8_num(637), info.Data());
  echo_indexed_call_count++;
  return v8_num(index);
}

v8::Handle<v8::Object> bottom;

static v8::Handle<Value> CheckThisIndexedPropertyHandler(
    uint32_t index,
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<Value>();
}

static v8::Handle<Value> CheckThisNamedPropertyHandler(
    Local<String> name,
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<Value>();
}

v8::Handle<Value> CheckThisIndexedPropertySetter(uint32_t index,
                                                 Local<Value> value,
                                                 const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<Value>();
}

v8::Handle<Value> CheckThisNamedPropertySetter(Local<String> property,
                                               Local<Value> value,
                                               const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<Value>();
}

v8::Handle<v8::Integer> CheckThisIndexedPropertyQuery(
    uint32_t index,
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Integer>();
}

v8::Handle<v8::Integer> CheckThisNamedPropertyQuery(Local<String> property,
                                                    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Integer>();
}

v8::Handle<v8::Boolean> CheckThisIndexedPropertyDeleter(
    uint32_t index,
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Boolean>();
}

v8::Handle<v8::Boolean> CheckThisNamedPropertyDeleter(
    Local<String> property,
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Boolean>();
}

v8::Handle<v8::Array> CheckThisIndexedPropertyEnumerator(
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Array>();
}


v8::Handle<v8::Array> CheckThisNamedPropertyEnumerator(
    const AccessorInfo& info) {
  ApiTestFuzzer::Fuzz();
  CHECK(info.This()->Equals(bottom));
  return v8::Handle<v8::Array>();
}

static v8::Handle<Value> handle_call(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  return v8_num(102);
}

static v8::Handle<Value> construct_call(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  args.This()->Set(v8_str("x"), v8_num(1));
  args.This()->Set(v8_str("y"), v8_num(2));
  return args.This();
}

static v8::Handle<Value> Return239(Local<String> name, const AccessorInfo&) {
  ApiTestFuzzer::Fuzz();
  return v8_num(239);
}

////////////////////////////////////////////////////////////////////////////////
//// Tests

// from test-api.cc:125
void
test_Handles()
{
  v8::HandleScope scope;
  Local<Context> local_env;
  {
    LocalContext env;
    local_env = env.local();
  }

  // Local context should still be live.
  CHECK(!local_env.IsEmpty());
  local_env->Enter();

  v8::Handle<v8::Primitive> undef = v8::Undefined();
  CHECK(!undef.IsEmpty());
  CHECK(undef->IsUndefined());

  const char* c_source = "1 + 2 + 3";
  Local<String> source = String::New(c_source);
  Local<Script> script = Script::Compile(source);
  CHECK_EQ(6, script->Run()->Int32Value());

  local_env->Exit();
}

// from test-api.cc:150
void
test_ReceiverSignature()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<v8::FunctionTemplate> fun = v8::FunctionTemplate::New();
  v8::Handle<v8::Signature> sig = v8::Signature::New(fun);
  fun->PrototypeTemplate()->Set(
      v8_str("m"),
      v8::FunctionTemplate::New(IncrementingSignatureCallback,
                                v8::Handle<Value>(),
                                sig));
  env->Global()->Set(v8_str("Fun"), fun->GetFunction());
  signature_callback_count = 0;
  CompileRun(
      "var o = new Fun();"
      "o.m();");
  CHECK_EQ(1, signature_callback_count);
  v8::Handle<v8::FunctionTemplate> sub_fun = v8::FunctionTemplate::New();
  sub_fun->Inherit(fun);
  env->Global()->Set(v8_str("SubFun"), sub_fun->GetFunction());
  CompileRun(
      "var o = new SubFun();"
      "o.m();");
  CHECK_EQ(2, signature_callback_count);

  v8::TryCatch try_catch;
  CompileRun(
      "var o = { };"
      "o.m = Fun.prototype.m;"
      "o.m();");
  CHECK_EQ(2, signature_callback_count);
  CHECK(try_catch.HasCaught());
  try_catch.Reset();
  v8::Handle<v8::FunctionTemplate> unrel_fun = v8::FunctionTemplate::New();
  sub_fun->Inherit(fun);
  env->Global()->Set(v8_str("UnrelFun"), unrel_fun->GetFunction());
  CompileRun(
      "var o = new UnrelFun();"
      "o.m = Fun.prototype.m;"
      "o.m();");
  CHECK_EQ(2, signature_callback_count);
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:196
void
test_ArgumentSignature()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<v8::FunctionTemplate> cons = v8::FunctionTemplate::New();
  cons->SetClassName(v8_str("Cons"));
  v8::Handle<v8::Signature> sig =
      v8::Signature::New(v8::Handle<v8::FunctionTemplate>(), 1, &cons);
  v8::Handle<v8::FunctionTemplate> fun =
      v8::FunctionTemplate::New(SignatureCallback, v8::Handle<Value>(), sig);
  env->Global()->Set(v8_str("Cons"), cons->GetFunction());
  env->Global()->Set(v8_str("Fun1"), fun->GetFunction());

  v8::Handle<Value> value1 = CompileRun("Fun1(4) == '';");
  CHECK(value1->IsTrue());

  v8::Handle<Value> value2 = CompileRun("Fun1(new Cons()) == '[object Cons]';");
  CHECK(value2->IsTrue());

  v8::Handle<Value> value3 = CompileRun("Fun1() == '';");
  CHECK(value3->IsTrue());

  v8::Handle<v8::FunctionTemplate> cons1 = v8::FunctionTemplate::New();
  cons1->SetClassName(v8_str("Cons1"));
  v8::Handle<v8::FunctionTemplate> cons2 = v8::FunctionTemplate::New();
  cons2->SetClassName(v8_str("Cons2"));
  v8::Handle<v8::FunctionTemplate> cons3 = v8::FunctionTemplate::New();
  cons3->SetClassName(v8_str("Cons3"));

  v8::Handle<v8::FunctionTemplate> args[3] = { cons1, cons2, cons3 };
  v8::Handle<v8::Signature> wsig =
      v8::Signature::New(v8::Handle<v8::FunctionTemplate>(), 3, args);
  v8::Handle<v8::FunctionTemplate> fun2 =
      v8::FunctionTemplate::New(SignatureCallback, v8::Handle<Value>(), wsig);

  env->Global()->Set(v8_str("Cons1"), cons1->GetFunction());
  env->Global()->Set(v8_str("Cons2"), cons2->GetFunction());
  env->Global()->Set(v8_str("Cons3"), cons3->GetFunction());
  env->Global()->Set(v8_str("Fun2"), fun2->GetFunction());
  v8::Handle<Value> value4 = CompileRun(
      "Fun2(new Cons1(), new Cons2(), new Cons3()) =="
      "'[object Cons1],[object Cons2],[object Cons3]'");
  CHECK(value4->IsTrue());

  v8::Handle<Value> value5 = CompileRun(
      "Fun2(new Cons1(), new Cons2(), 5) == '[object Cons1],[object Cons2],'");
  CHECK(value5->IsTrue());

  v8::Handle<Value> value6 = CompileRun(
      "Fun2(new Cons3(), new Cons2(), new Cons1()) == ',[object Cons2],'");
  CHECK(value6->IsTrue());

  v8::Handle<Value> value7 = CompileRun(
      "Fun2(new Cons1(), new Cons2(), new Cons3(), 'd') == "
      "'[object Cons1],[object Cons2],[object Cons3],d';");
  CHECK(value7->IsTrue());

  v8::Handle<Value> value8 = CompileRun(
      "Fun2(new Cons1(), new Cons2()) == '[object Cons1],[object Cons2]'");
  CHECK(value8->IsTrue());
}

// from test-api.cc:258
void
test_HulIgennem()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<v8::Primitive> undef = v8::Undefined();
  Local<String> undef_str = undef->ToString();
  char* value = i::NewArray<char>(undef_str->Length() + 1);
  undef_str->WriteAscii(value);
  CHECK_EQ(0, strcmp(value, "undefined"));
  i::DeleteArray(value);
}

// from test-api.cc:270
void
test_Access()
{
  v8::HandleScope scope;
  LocalContext env;
  Local<v8::Object> obj = v8::Object::New();
  Local<Value> foo_before = obj->Get(v8_str("foo"));
  CHECK(foo_before->IsUndefined());
  Local<String> bar_str = v8_str("bar");
  obj->Set(v8_str("foo"), bar_str);
  Local<Value> foo_after = obj->Get(v8_str("foo"));
  CHECK(!foo_after->IsUndefined());
  CHECK(foo_after->IsString());
  CHECK_EQ(bar_str, foo_after);
}

// from test-api.cc:285
void
test_AccessElement()
{
  v8::HandleScope scope;
  LocalContext env;
  Local<v8::Object> obj = v8::Object::New();
  Local<Value> before = obj->Get(1);
  CHECK(before->IsUndefined());
  Local<String> bar_str = v8_str("bar");
  obj->Set(1, bar_str);
  Local<Value> after = obj->Get(1);
  CHECK(!after->IsUndefined());
  CHECK(after->IsString());
  CHECK_EQ(bar_str, after);

  Local<v8::Array> value = CompileRun("[\"a\", \"b\"]").As<v8::Array>();
  CHECK_EQ(v8_str("a"), value->Get(0));
  CHECK_EQ(v8_str("b"), value->Get(1));
}

// from test-api.cc:304
void
test_Script()
{
  v8::HandleScope scope;
  LocalContext env;
  const char* c_source = "1 + 2 + 3";
  Local<String> source = String::New(c_source);
  Local<Script> script = Script::Compile(source);
  CHECK_EQ(6, script->Run()->Int32Value());
}

// from test-api.cc:381
void
test_ScriptUsingStringResource()
{ }

// from test-api.cc:406
void
test_ScriptUsingAsciiStringResource()
{ }

// from test-api.cc:427
void
test_ScriptMakingExternalString()
{ }

// from test-api.cc:452
void
test_ScriptMakingExternalAsciiString()
{ }

// from test-api.cc:478
void
test_MakingExternalStringConditions()
{ }

// from test-api.cc:524
void
test_MakingExternalAsciiStringConditions()
{ }

// from test-api.cc:561
void
test_UsingExternalString()
{ }

// from test-api.cc:579
void
test_UsingExternalAsciiString()
{ }

// from test-api.cc:597
void
test_ScavengeExternalString()
{ }

// from test-api.cc:616
void
test_ScavengeExternalAsciiString()
{ }

// from test-api.cc:655
void
test_ExternalStringWithDisposeHandling()
{ }

// from test-api.cc:701
void
test_StringConcat()
{ }

// from test-api.cc:747
void
test_GlobalProperties()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<v8::Object> global = env->Global();
  global->Set(v8_str("pi"), v8_num(3.1415926));
  Local<Value> pi = global->Get(v8_str("pi"));
  CHECK_EQ(3.1415926, pi->NumberValue());
}

// from test-api.cc:776
void
test_FunctionTemplate()
{
  v8::HandleScope scope;
  LocalContext env;
  {
    Local<v8::FunctionTemplate> fun_templ =
        v8::FunctionTemplate::New(handle_call);
    Local<Function> fun = fun_templ->GetFunction();
    env->Global()->Set(v8_str("obj"), fun);
    Local<Script> script = v8_compile("obj()");
    CHECK_EQ(102, script->Run()->Int32Value());
  }
  // Use SetCallHandler to initialize a function template, should work like the
  // previous one.
  {
    Local<v8::FunctionTemplate> fun_templ = v8::FunctionTemplate::New();
    fun_templ->SetCallHandler(handle_call);
    Local<Function> fun = fun_templ->GetFunction();
    env->Global()->Set(v8_str("obj"), fun);
    Local<Script> script = v8_compile("obj()");
    CHECK_EQ(102, script->Run()->Int32Value());
  }
  // Test constructor calls.
  {
    Local<v8::FunctionTemplate> fun_templ =
        v8::FunctionTemplate::New(construct_call);
    fun_templ->SetClassName(v8_str("funky"));
    fun_templ->InstanceTemplate()->SetAccessor(v8_str("m"), Return239);
    Local<Function> fun = fun_templ->GetFunction();
    env->Global()->Set(v8_str("obj"), fun);
    Local<Script> script = v8_compile("var s = new obj(); s.x");
    CHECK_EQ(1, script->Run()->Int32Value());

    Local<Value> result = v8_compile("(new obj()).toString()")->Run();
    CHECK_EQ(v8_str("[object funky]"), result);

    result = v8_compile("(new obj()).m")->Run();
    CHECK_EQ(239, result->Int32Value());
  }
}

// from test-api.cc:844
void
test_ExternalWrap()
{ }

// from test-api.cc:890
void
test_FindInstanceInPrototypeChain()
{ }

// from test-api.cc:937
void
test_TinyInteger()
{
  v8::HandleScope scope;
  LocalContext env;
  int32_t value = 239;
  Local<v8::Integer> value_obj = v8::Integer::New(value);
  CHECK_EQ(static_cast<int64_t>(value), value_obj->Value());
}

// from test-api.cc:946
void
test_BigSmiInteger()
{ }

// from test-api.cc:960
void
test_BigInteger()
{ }

// from test-api.cc:977
void
test_TinyUnsignedInteger()
{
  v8::HandleScope scope;
  LocalContext env;
  uint32_t value = 239;
  Local<v8::Integer> value_obj = v8::Integer::NewFromUnsigned(value);
  CHECK_EQ(static_cast<int64_t>(value), value_obj->Value());
}

// from test-api.cc:986
void
test_BigUnsignedSmiInteger()
{ }

// from test-api.cc:997
void
test_BigUnsignedInteger()
{ }

// from test-api.cc:1008
void
test_OutOfSignedRangeUnsignedInteger()
{
  v8::HandleScope scope;
  LocalContext env;
  uint32_t INT32_MAX_AS_UINT = (1U << 31) - 1;
  uint32_t value = INT32_MAX_AS_UINT + 1;
  CHECK(value > INT32_MAX_AS_UINT);  // No overflow.
  Local<v8::Integer> value_obj = v8::Integer::NewFromUnsigned(value);
  CHECK_EQ(static_cast<int64_t>(value), value_obj->Value());
}

// from test-api.cc:1019
void
test_Number()
{
  v8::HandleScope scope;
  LocalContext env;
  double PI = 3.1415926;
  Local<v8::Number> pi_obj = v8::Number::New(PI);
  CHECK_EQ(PI, pi_obj->NumberValue());
}

// from test-api.cc:1028
void
test_ToNumber()
{
  v8::HandleScope scope;
  LocalContext env;
  Local<String> str = v8_str("3.1415926");
  CHECK_EQ(3.1415926, str->NumberValue());
  v8::Handle<v8::Boolean> t = v8::True();
  CHECK_EQ(1.0, t->NumberValue());
  v8::Handle<v8::Boolean> f = v8::False();
  CHECK_EQ(0.0, f->NumberValue());
}

// from test-api.cc:1040
void
test_Date()
{
  v8::HandleScope scope;
  LocalContext env;
  double PI = 3.1415926;
  Local<Value> date_obj = v8::Date::New(PI);
  CHECK(date_obj->IsDate());
  CHECK_EQ(3.0, date_obj->NumberValue());
}

// from test-api.cc:1049
void
test_Boolean()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<v8::Boolean> t = v8::True();
  CHECK(t->Value());
  v8::Handle<v8::Boolean> f = v8::False();
  CHECK(!f->Value());
  v8::Handle<v8::Primitive> u = v8::Undefined();
  CHECK(!u->BooleanValue());
  v8::Handle<v8::Primitive> n = v8::Null();
  CHECK(!n->BooleanValue());
  v8::Handle<String> str1 = v8_str("");
  CHECK(!str1->BooleanValue());
  v8::Handle<String> str2 = v8_str("x");
  CHECK(str2->BooleanValue());
  CHECK(!v8::Number::New(0)->BooleanValue());
  CHECK(v8::Number::New(-1)->BooleanValue());
  CHECK(v8::Number::New(1)->BooleanValue());
  CHECK(v8::Number::New(42)->BooleanValue());
  CHECK(!v8_compile("NaN")->Run()->BooleanValue());
}

// from test-api.cc:1084
void
test_GlobalPrototype()
{ }

// from test-api.cc:1103
void
test_ObjectTemplate()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ1 = ObjectTemplate::New();
  templ1->Set("x", v8_num(10));
  templ1->Set("y", v8_num(13));
  LocalContext env;
  Local<v8::Object> instance1 = templ1->NewInstance();
  env->Global()->Set(v8_str("p"), instance1);
  CHECK(v8_compile("(p.x == 10)")->Run()->BooleanValue());
  CHECK(v8_compile("(p.y == 13)")->Run()->BooleanValue());
  Local<v8::FunctionTemplate> fun = v8::FunctionTemplate::New();
  fun->PrototypeTemplate()->Set("nirk", v8_num(123));
  Local<ObjectTemplate> templ2 = fun->InstanceTemplate();
  templ2->Set("a", v8_num(12));
  templ2->Set("b", templ1);
  Local<v8::Object> instance2 = templ2->NewInstance();
  env->Global()->Set(v8_str("q"), instance2);
  CHECK(v8_compile("(q.nirk == 123)")->Run()->BooleanValue());
  CHECK(v8_compile("(q.a == 12)")->Run()->BooleanValue());
  CHECK(v8_compile("(q.b.x == 10)")->Run()->BooleanValue());
  CHECK(v8_compile("(q.b.y == 13)")->Run()->BooleanValue());
}

// from test-api.cc:1139
void
test_DescriptorInheritance()
{
  v8::HandleScope scope;
  v8::Handle<v8::FunctionTemplate> super = v8::FunctionTemplate::New();
  super->PrototypeTemplate()->Set("flabby",
                                  v8::FunctionTemplate::New(GetFlabby));
  super->PrototypeTemplate()->Set("PI", v8_num(3.14));

  super->InstanceTemplate()->SetAccessor(v8_str("knurd"), GetKnurd);

  v8::Handle<v8::FunctionTemplate> base1 = v8::FunctionTemplate::New();
  base1->Inherit(super);
  base1->PrototypeTemplate()->Set("v1", v8_num(20.1));

  v8::Handle<v8::FunctionTemplate> base2 = v8::FunctionTemplate::New();
  base2->Inherit(super);
  base2->PrototypeTemplate()->Set("v2", v8_num(10.1));

  LocalContext env;

  env->Global()->Set(v8_str("s"), super->GetFunction());
  env->Global()->Set(v8_str("base1"), base1->GetFunction());
  env->Global()->Set(v8_str("base2"), base2->GetFunction());

  // Checks right __proto__ chain.
  CHECK(CompileRun("base1.prototype.__proto__ == s.prototype")->BooleanValue());
  CHECK(CompileRun("base2.prototype.__proto__ == s.prototype")->BooleanValue());

  CHECK(v8_compile("s.prototype.PI == 3.14")->Run()->BooleanValue());

  // Instance accessor should not be visible on function object or its prototype
  CHECK(CompileRun("s.knurd == undefined")->BooleanValue());
  CHECK(CompileRun("s.prototype.knurd == undefined")->BooleanValue());
  CHECK(CompileRun("base1.prototype.knurd == undefined")->BooleanValue());

  env->Global()->Set(v8_str("obj"),
                     base1->GetFunction()->NewInstance());
  CHECK_EQ(17.2, v8_compile("obj.flabby()")->Run()->NumberValue());
  CHECK(v8_compile("'flabby' in obj")->Run()->BooleanValue());
  CHECK_EQ(15.2, v8_compile("obj.knurd")->Run()->NumberValue());
  CHECK(v8_compile("'knurd' in obj")->Run()->BooleanValue());
  CHECK_EQ(20.1, v8_compile("obj.v1")->Run()->NumberValue());

  env->Global()->Set(v8_str("obj2"),
                     base2->GetFunction()->NewInstance());
  CHECK_EQ(17.2, v8_compile("obj2.flabby()")->Run()->NumberValue());
  CHECK(v8_compile("'flabby' in obj2")->Run()->BooleanValue());
  CHECK_EQ(15.2, v8_compile("obj2.knurd")->Run()->NumberValue());
  CHECK(v8_compile("'knurd' in obj2")->Run()->BooleanValue());
  CHECK_EQ(10.1, v8_compile("obj2.v2")->Run()->NumberValue());

  // base1 and base2 cannot cross reference to each's prototype
  CHECK(v8_compile("obj.v2")->Run()->IsUndefined());
  CHECK(v8_compile("obj2.v1")->Run()->IsUndefined());
}

// from test-api.cc:1207
void
test_NamedPropertyHandlerGetter()
{
  echo_named_call_count = 0;
  v8::HandleScope scope;
  v8::Handle<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  templ->InstanceTemplate()->SetNamedPropertyHandler(EchoNamedProperty,
                                                     0, 0, 0, 0,
                                                     v8_str("data"));
  LocalContext env;
  env->Global()->Set(v8_str("obj"),
                     templ->GetFunction()->NewInstance());
  CHECK_EQ(echo_named_call_count, 0);
  v8_compile("obj.x")->Run();
  CHECK_EQ(echo_named_call_count, 1);
  const char* code = "var str = 'oddle'; obj[str] + obj.poddle;";
  v8::Handle<Value> str = CompileRun(code);
  String::AsciiValue value(str);
  CHECK_EQ(*value, "oddlepoddle");
  // Check default behavior
  CHECK_EQ(v8_compile("obj.flob = 10;")->Run()->Int32Value(), 10);
  CHECK(v8_compile("'myProperty' in obj")->Run()->BooleanValue());
  CHECK(v8_compile("delete obj.myProperty")->Run()->BooleanValue());
}

// from test-api.cc:1243
void
test_IndexedPropertyHandlerGetter()
{
  v8::HandleScope scope;
  v8::Handle<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  templ->InstanceTemplate()->SetIndexedPropertyHandler(EchoIndexedProperty,
                                                       0, 0, 0, 0,
                                                       v8_num(637));
  LocalContext env;
  env->Global()->Set(v8_str("obj"),
                     templ->GetFunction()->NewInstance());
  Local<Script> script = v8_compile("obj[900]");
  CHECK_EQ(script->Run()->Int32Value(), 900);
}

// from test-api.cc:1344
void
test_PropertyHandlerInPrototype()
{
  v8::HandleScope scope;
  LocalContext env;

  // Set up a prototype chain with three interceptors.
  v8::Handle<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  templ->InstanceTemplate()->SetIndexedPropertyHandler(
      CheckThisIndexedPropertyHandler,
      CheckThisIndexedPropertySetter,
      CheckThisIndexedPropertyQuery,
      CheckThisIndexedPropertyDeleter,
      CheckThisIndexedPropertyEnumerator);

  templ->InstanceTemplate()->SetNamedPropertyHandler(
      CheckThisNamedPropertyHandler,
      CheckThisNamedPropertySetter,
      CheckThisNamedPropertyQuery,
      CheckThisNamedPropertyDeleter,
      CheckThisNamedPropertyEnumerator);

  bottom = templ->GetFunction()->NewInstance();
  Local<v8::Object> top = templ->GetFunction()->NewInstance();
  Local<v8::Object> middle = templ->GetFunction()->NewInstance();

  bottom->Set(v8_str("__proto__"), middle);
  middle->Set(v8_str("__proto__"), top);
  env->Global()->Set(v8_str("obj"), bottom);

  // Indexed and named get.
  Script::Compile(v8_str("obj[0]"))->Run();
  Script::Compile(v8_str("obj.x"))->Run();

  // Indexed and named set.
  Script::Compile(v8_str("obj[1] = 42"))->Run();
  Script::Compile(v8_str("obj.y = 42"))->Run();

  // Indexed and named query.
  Script::Compile(v8_str("0 in obj"))->Run();
  Script::Compile(v8_str("'x' in obj"))->Run();

  // Indexed and named deleter.
  Script::Compile(v8_str("delete obj[0]"))->Run();
  Script::Compile(v8_str("delete obj.x"))->Run();

  // Enumerators.
  Script::Compile(v8_str("for (var p in obj) ;"))->Run();
}

// from test-api.cc:1413
void
test_PrePropertyHandler()
{ }

// from test-api.cc:1431
void
test_UndefinedIsNotEnumerable()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::Handle<Value> result = Script::Compile(v8_str(
      "this.propertyIsEnumerable(undefined)"))->Run();
  CHECK(result->IsFalse());
}

// from test-api.cc:1468
void
test_DeepCrossLanguageRecursion()
{ }

// from test-api.cc:1502
void
test_CallbackExceptionRegression()
{ }

// from test-api.cc:1518
void
test_FunctionPrototype()
{
  v8::HandleScope scope;
  Local<v8::FunctionTemplate> Foo = v8::FunctionTemplate::New();
  Foo->PrototypeTemplate()->Set(v8_str("plak"), v8_num(321));
  LocalContext env;
  env->Global()->Set(v8_str("Foo"), Foo->GetFunction());
  Local<Script> script = Script::Compile(v8_str("Foo.prototype.plak"));
  CHECK_EQ(script->Run()->Int32Value(), 321);
}

// from test-api.cc:1529
void
test_InternalFields()
{
  v8::HandleScope scope;
  LocalContext env;

  Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  Local<v8::ObjectTemplate> instance_templ = templ->InstanceTemplate();
  instance_templ->SetInternalFieldCount(1);
  Local<v8::Object> obj = templ->GetFunction()->NewInstance();
  CHECK_EQ(1, obj->InternalFieldCount());
  CHECK(obj->GetInternalField(0)->IsUndefined());
  obj->SetInternalField(0, v8_num(17));
  CHECK_EQ(17, obj->GetInternalField(0)->Int32Value());
}

// from test-api.cc:1544
void
test_GlobalObjectInternalFields()
{ }

// from test-api.cc:1558
void
test_InternalFieldsNativePointers()
{ }

// from test-api.cc:1590
void
test_InternalFieldsNativePointersAndExternal()
{ }

// from test-api.cc:1628
void
test_IdentityHash()
{ }

// from test-api.cc:1672
void
test_HiddenProperties()
{
  v8::HandleScope scope;
  LocalContext env;

  v8::Local<v8::Object> obj = v8::Object::New();
  v8::Local<v8::String> key = v8_str("api-test::hidden-key");
  v8::Local<v8::String> empty = v8_str("");
  v8::Local<v8::String> prop_name = v8_str("prop_name");

  i::Heap::CollectAllGarbage(false);

  // Make sure delete of a non-existent hidden value works
  CHECK(obj->DeleteHiddenValue(key));

  CHECK(obj->SetHiddenValue(key, v8::Integer::New(1503)));
  CHECK_EQ(1503, obj->GetHiddenValue(key)->Int32Value());
  CHECK(obj->SetHiddenValue(key, v8::Integer::New(2002)));
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());

  i::Heap::CollectAllGarbage(false);

  // Make sure we do not find the hidden property.
  CHECK(!obj->Has(empty));
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());
  CHECK(obj->Get(empty)->IsUndefined());
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());
  CHECK(obj->Set(empty, v8::Integer::New(2003)));
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());
  CHECK_EQ(2003, obj->Get(empty)->Int32Value());

  i::Heap::CollectAllGarbage(false);

  // Add another property and delete it afterwards to force the object in
  // slow case.
  CHECK(obj->Set(prop_name, v8::Integer::New(2008)));
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());
  CHECK_EQ(2008, obj->Get(prop_name)->Int32Value());
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());
  CHECK(obj->Delete(prop_name));
  CHECK_EQ(2002, obj->GetHiddenValue(key)->Int32Value());

  i::Heap::CollectAllGarbage(false);

  CHECK(obj->DeleteHiddenValue(key));
  CHECK(obj->GetHiddenValue(key).IsEmpty());
}


// from test-api.cc:1728
void
test_HiddenPropertiesWithInterceptors()
{ }

// from test-api.cc:1748
void
test_External()
{ }

// from test-api.cc:1780
void
test_GlobalHandle()
{
  v8::Persistent<String> global;
  {
    v8::HandleScope scope;
    Local<String> str = v8_str("str");
    global = v8::Persistent<String>::New(str);
  }
  CHECK_EQ(global->Length(), 3);
  global.Dispose();
}

// from test-api.cc:1792
void
test_ScriptException()
{
  v8::HandleScope scope;
  LocalContext env;
  Local<Script> script = Script::Compile(v8_str("throw 'panama!';"));
  v8::TryCatch try_catch;
  Local<Value> result = script->Run();
  CHECK(result.IsEmpty());
  CHECK(try_catch.HasCaught());
  String::AsciiValue exception_value(try_catch.Exception());
  //CHECK_EQ(*exception_value, "panama!");
  do_check_true(0 == strcmp(*exception_value, "panama!"));
}

// from test-api.cc:1817
void
test_MessageHandlerData()
{ }

// from test-api.cc:1835
void
test_GetSetProperty()
{
  v8::HandleScope scope;
  LocalContext context;
  context->Global()->Set(v8_str("foo"), v8_num(14));
  context->Global()->Set(v8_str("12"), v8_num(92));
  context->Global()->Set(v8::Integer::New(16), v8_num(32));
  context->Global()->Set(v8_num(13), v8_num(56));
  Local<Value> foo = Script::Compile(v8_str("this.foo"))->Run();
  CHECK_EQ(14, foo->Int32Value());
  Local<Value> twelve = Script::Compile(v8_str("this[12]"))->Run();
  CHECK_EQ(92, twelve->Int32Value());
  Local<Value> sixteen = Script::Compile(v8_str("this[16]"))->Run();
  CHECK_EQ(32, sixteen->Int32Value());
  Local<Value> thirteen = Script::Compile(v8_str("this[13]"))->Run();
  CHECK_EQ(56, thirteen->Int32Value());
  CHECK_EQ(92, context->Global()->Get(v8::Integer::New(12))->Int32Value());
  CHECK_EQ(92, context->Global()->Get(v8_str("12"))->Int32Value());
  CHECK_EQ(92, context->Global()->Get(v8_num(12))->Int32Value());
  CHECK_EQ(32, context->Global()->Get(v8::Integer::New(16))->Int32Value());
  CHECK_EQ(32, context->Global()->Get(v8_str("16"))->Int32Value());
  CHECK_EQ(32, context->Global()->Get(v8_num(16))->Int32Value());
  CHECK_EQ(56, context->Global()->Get(v8::Integer::New(13))->Int32Value());
  CHECK_EQ(56, context->Global()->Get(v8_str("13"))->Int32Value());
  CHECK_EQ(56, context->Global()->Get(v8_num(13))->Int32Value());
}

// from test-api.cc:1862
void
test_PropertyAttributes()
{
  v8::HandleScope scope;
  LocalContext context;
  // read-only
  Local<String> prop = v8_str("read_only");
  context->Global()->Set(prop, v8_num(7), v8::ReadOnly);
  CHECK_EQ(7, context->Global()->Get(prop)->Int32Value());
  Script::Compile(v8_str("read_only = 9"))->Run();
  CHECK_EQ(7, context->Global()->Get(prop)->Int32Value());
  context->Global()->Set(prop, v8_num(10));
  CHECK_EQ(7, context->Global()->Get(prop)->Int32Value());
  // dont-delete
  prop = v8_str("dont_delete");
  context->Global()->Set(prop, v8_num(13), v8::DontDelete);
  CHECK_EQ(13, context->Global()->Get(prop)->Int32Value());
  Script::Compile(v8_str("delete dont_delete"))->Run();
  CHECK_EQ(13, context->Global()->Get(prop)->Int32Value());
}

// from test-api.cc:1882
void
test_Array()
{
  v8::HandleScope scope;
  LocalContext context;
  Local<v8::Array> array = v8::Array::New();
  CHECK_EQ(0, array->Length());
  CHECK(array->Get(0)->IsUndefined());
  CHECK(!array->Has(0));
  CHECK(array->Get(100)->IsUndefined());
  CHECK(!array->Has(100));
  array->Set(2, v8_num(7));
  CHECK_EQ(3, array->Length());
  CHECK(!array->Has(0));
  CHECK(!array->Has(1));
  CHECK(array->Has(2));
  CHECK_EQ(7, array->Get(2)->Int32Value());
  Local<Value> obj = Script::Compile(v8_str("[1, 2, 3]"))->Run();
  Local<v8::Array> arr = obj.As<v8::Array>();
  CHECK_EQ(3, arr->Length());
  CHECK_EQ(1, arr->Get(0)->Int32Value());
  CHECK_EQ(2, arr->Get(1)->Int32Value());
  CHECK_EQ(3, arr->Get(2)->Int32Value());
}

// from test-api.cc:1916
void
test_Vector()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> global = ObjectTemplate::New();
  global->Set(v8_str("f"), v8::FunctionTemplate::New(HandleF));
  LocalContext context(0, global);

  const char* fun = "f()";
  Local<v8::Array> a0 = CompileRun(fun).As<v8::Array>();
  CHECK_EQ(0, a0->Length());

  const char* fun2 = "f(11)";
  Local<v8::Array> a1 = CompileRun(fun2).As<v8::Array>();
  CHECK_EQ(1, a1->Length());
  CHECK_EQ(11, a1->Get(0)->Int32Value());

  const char* fun3 = "f(12, 13)";
  Local<v8::Array> a2 = CompileRun(fun3).As<v8::Array>();
  CHECK_EQ(2, a2->Length());
  CHECK_EQ(12, a2->Get(0)->Int32Value());
  CHECK_EQ(13, a2->Get(1)->Int32Value());

  const char* fun4 = "f(14, 15, 16)";
  Local<v8::Array> a3 = CompileRun(fun4).As<v8::Array>();
  CHECK_EQ(3, a3->Length());
  CHECK_EQ(14, a3->Get(0)->Int32Value());
  CHECK_EQ(15, a3->Get(1)->Int32Value());
  CHECK_EQ(16, a3->Get(2)->Int32Value());

  const char* fun5 = "f(17, 18, 19, 20)";
  Local<v8::Array> a4 = CompileRun(fun5).As<v8::Array>();
  CHECK_EQ(4, a4->Length());
  CHECK_EQ(17, a4->Get(0)->Int32Value());
  CHECK_EQ(18, a4->Get(1)->Int32Value());
  CHECK_EQ(19, a4->Get(2)->Int32Value());
  CHECK_EQ(20, a4->Get(3)->Int32Value());
}

// from test-api.cc:1954
void
test_FunctionCall()
{
  v8::HandleScope scope;
  LocalContext context;
  CompileRun(
    "function Foo() {"
    "  var result = [];"
    "  for (var i = 0; i < arguments.length; i++) {"
    "    result.push(arguments[i]);"
    "  }"
    "  return result;"
    "}");
  Local<Function> Foo =
      Local<Function>::Cast(context->Global()->Get(v8_str("Foo")));

  v8::Handle<Value>* args0 = NULL;
  Local<v8::Array> a0 = Local<v8::Array>::Cast(Foo->Call(Foo, 0, args0));
  CHECK_EQ(0, a0->Length());

  v8::Handle<Value> args1[] = { v8_num(1.1) };
  Local<v8::Array> a1 = Local<v8::Array>::Cast(Foo->Call(Foo, 1, args1));
  CHECK_EQ(1, a1->Length());
  CHECK_EQ(1.1, a1->Get(v8::Integer::New(0))->NumberValue());

  v8::Handle<Value> args2[] = { v8_num(2.2),
                                v8_num(3.3) };
  Local<v8::Array> a2 = Local<v8::Array>::Cast(Foo->Call(Foo, 2, args2));
  CHECK_EQ(2, a2->Length());
  CHECK_EQ(2.2, a2->Get(v8::Integer::New(0))->NumberValue());
  CHECK_EQ(3.3, a2->Get(v8::Integer::New(1))->NumberValue());

  v8::Handle<Value> args3[] = { v8_num(4.4),
                                v8_num(5.5),
                                v8_num(6.6) };
  Local<v8::Array> a3 = Local<v8::Array>::Cast(Foo->Call(Foo, 3, args3));
  CHECK_EQ(3, a3->Length());
  CHECK_EQ(4.4, a3->Get(v8::Integer::New(0))->NumberValue());
  CHECK_EQ(5.5, a3->Get(v8::Integer::New(1))->NumberValue());
  CHECK_EQ(6.6, a3->Get(v8::Integer::New(2))->NumberValue());

  v8::Handle<Value> args4[] = { v8_num(7.7),
                                v8_num(8.8),
                                v8_num(9.9),
                                v8_num(10.11) };
  Local<v8::Array> a4 = Local<v8::Array>::Cast(Foo->Call(Foo, 4, args4));
  CHECK_EQ(4, a4->Length());
  CHECK_EQ(7.7, a4->Get(v8::Integer::New(0))->NumberValue());
  CHECK_EQ(8.8, a4->Get(v8::Integer::New(1))->NumberValue());
  CHECK_EQ(9.9, a4->Get(v8::Integer::New(2))->NumberValue());
  CHECK_EQ(10.11, a4->Get(v8::Integer::New(3))->NumberValue());
}

// from test-api.cc:2012
void
test_OutOfMemory()
{ }

// from test-api.cc:2053
void
test_OutOfMemoryNested()
{ }

// from test-api.cc:2082
void
test_HugeConsStringOutOfMemory()
{ }

// from test-api.cc:2108
void
test_ConstructCall()
{ }

// from test-api.cc:2168
void
test_ConversionNumber()
{
  v8::HandleScope scope;
  LocalContext env;
  // Very large number.
  CompileRun("var obj = Math.pow(2,32) * 1237;");
  Local<Value> obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(5312874545152.0, obj->ToNumber()->Value());
  CHECK_EQ(0, obj->ToInt32()->Value());
  CHECK(0u == obj->ToUint32()->Value());  // NOLINT - no CHECK_EQ for unsigned.
  // Large number.
  CompileRun("var obj = -1234567890123;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(-1234567890123.0, obj->ToNumber()->Value());
  CHECK_EQ(-1912276171, obj->ToInt32()->Value());
  CHECK(2382691125u == obj->ToUint32()->Value());  // NOLINT
  // Small positive integer.
  CompileRun("var obj = 42;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(42.0, obj->ToNumber()->Value());
  CHECK_EQ(42, obj->ToInt32()->Value());
  CHECK(42u == obj->ToUint32()->Value());  // NOLINT
  // Negative integer.
  CompileRun("var obj = -37;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(-37.0, obj->ToNumber()->Value());
  CHECK_EQ(-37, obj->ToInt32()->Value());
  CHECK(4294967259u == obj->ToUint32()->Value());  // NOLINT
  // Positive non-int32 integer.
  CompileRun("var obj = 0x81234567;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(2166572391.0, obj->ToNumber()->Value());
  CHECK_EQ(-2128394905, obj->ToInt32()->Value());
  CHECK(2166572391u == obj->ToUint32()->Value());  // NOLINT
  // Fraction.
  CompileRun("var obj = 42.3;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(42.3, obj->ToNumber()->Value());
  CHECK_EQ(42, obj->ToInt32()->Value());
  CHECK(42u == obj->ToUint32()->Value());  // NOLINT
  // Large negative fraction.
  CompileRun("var obj = -5726623061.75;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK_EQ(-5726623061.75, obj->ToNumber()->Value());
  CHECK_EQ(-1431655765, obj->ToInt32()->Value());
  CHECK(2863311531u == obj->ToUint32()->Value());  // NOLINT
}

// from test-api.cc:2216
void
test_isNumberType()
{
  v8::HandleScope scope;
  LocalContext env;
  // Very large number.
  CompileRun("var obj = Math.pow(2,32) * 1237;");
  Local<Value> obj = env->Global()->Get(v8_str("obj"));
  CHECK(!obj->IsInt32());
  CHECK(!obj->IsUint32());
  // Large negative number.
  CompileRun("var obj = -1234567890123;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(!obj->IsInt32());
  CHECK(!obj->IsUint32());
  // Small positive integer.
  CompileRun("var obj = 42;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(obj->IsInt32());
  CHECK(obj->IsUint32());
  // Negative integer.
  CompileRun("var obj = -37;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(obj->IsInt32());
  CHECK(!obj->IsUint32());
  // Positive non-int32 integer.
  CompileRun("var obj = 0x81234567;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(!obj->IsInt32());
  CHECK(obj->IsUint32());
  // Fraction.
  CompileRun("var obj = 42.3;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(!obj->IsInt32());
  CHECK(!obj->IsUint32());
  // Large negative fraction.
  CompileRun("var obj = -5726623061.75;");
  obj = env->Global()->Get(v8_str("obj"));
  CHECK(!obj->IsInt32());
  CHECK(!obj->IsUint32());
}

// from test-api.cc:2257
void
test_ConversionException()
{
  v8::HandleScope scope;
  LocalContext env;
  CompileRun(
    "function TestClass() { };"
    "TestClass.prototype.toString = function () { throw 'uncle?'; };"
    "var obj = new TestClass();");
  Local<Value> obj = env->Global()->Get(v8_str("obj"));

  v8::TryCatch try_catch;

  Local<Value> to_string_result = obj->ToString();
  CHECK(to_string_result.IsEmpty());
  CheckUncle(&try_catch);

  Local<Value> to_number_result = obj->ToNumber();
  CHECK(to_number_result.IsEmpty());
  CheckUncle(&try_catch);

  Local<Value> to_integer_result = obj->ToInteger();
  CHECK(to_integer_result.IsEmpty());
  CheckUncle(&try_catch);

  Local<Value> to_uint32_result = obj->ToUint32();
  CHECK(to_uint32_result.IsEmpty());
  CheckUncle(&try_catch);

  Local<Value> to_int32_result = obj->ToInt32();
  CHECK(to_int32_result.IsEmpty());
  CheckUncle(&try_catch);

  Local<Value> to_object_result = v8::Undefined()->ToObject();
  CHECK(to_object_result.IsEmpty());
  CHECK(try_catch.HasCaught());
  try_catch.Reset();

  int32_t int32_value = obj->Int32Value();
  CHECK_EQ(0, int32_value);
  CheckUncle(&try_catch);

  uint32_t uint32_value = obj->Uint32Value();
  CHECK_EQ(0, uint32_value);
  CheckUncle(&try_catch);

  double number_value = obj->NumberValue();
  CHECK_NE(0, IsNaN(number_value));
  CheckUncle(&try_catch);

  int64_t integer_value = obj->IntegerValue();
  CHECK_EQ(0.0, static_cast<double>(integer_value));
  CheckUncle(&try_catch);
}

// from test-api.cc:2327
void
test_APICatch()
{ }

// from test-api.cc:2345
void
test_APIThrowTryCatch()
{ }

// from test-api.cc:2364
void
test_TryCatchInTryFinally()
{ }

// from test-api.cc:2399
void
test_APIThrowMessageOverwrittenToString()
{ }

// from test-api.cc:2444
void
test_APIThrowMessage()
{ }

// from test-api.cc:2458
void
test_APIThrowMessageAndVerboseTryCatch()
{ }

// from test-api.cc:2476
void
test_ExternalScriptException()
{ }

// from test-api.cc:2543
void
test_EvalInTryFinally()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::TryCatch try_catch;
  CompileRun("(function() {"
             "  try {"
             "    eval('asldkf (*&^&*^');"
             "  } finally {"
             "    return;"
             "  }"
             "})()");
  CHECK(!try_catch.HasCaught());
}

// from test-api.cc:2578
void
test_ExceptionOrder()
{ }

v8::Handle<Value> ThrowValue(const v8::Arguments& args) {
  ApiTestFuzzer::Fuzz();
  CHECK_EQ(1, args.Length());
  return v8::ThrowException(args[0]);
}

// from test-api.cc:2642
void
test_ThrowValues()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->Set(v8_str("Throw"), v8::FunctionTemplate::New(ThrowValue));
  LocalContext context(0, templ);
  v8::Handle<v8::Array> result = v8::Handle<v8::Array>::Cast(CompileRun(
    "function Run(obj) {"
    "  try {"
    "    Throw(obj);"
    "  } catch (e) {"
    "    return e;"
    "  }"
    "  return 'no exception';"
    "}"
    "[Run('str'), Run(1), Run(0), Run(null), Run(void 0)];"));
  CHECK_EQ(5, result->Length());
  CHECK(result->Get(v8::Integer::New(0))->IsString());
  CHECK(result->Get(v8::Integer::New(1))->IsNumber());
  CHECK_EQ(1, result->Get(v8::Integer::New(1))->Int32Value());
  CHECK(result->Get(v8::Integer::New(2))->IsNumber());
  CHECK_EQ(0, result->Get(v8::Integer::New(2))->Int32Value());
  CHECK(result->Get(v8::Integer::New(3))->IsNull());
  CHECK(result->Get(v8::Integer::New(4))->IsUndefined());
}

// from test-api.cc:2668
void
test_CatchZero()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::TryCatch try_catch;
  CHECK(!try_catch.HasCaught());
  Script::Compile(v8_str("throw 10"))->Run();
  CHECK(try_catch.HasCaught());
  CHECK_EQ(10, try_catch.Exception()->Int32Value());
  try_catch.Reset();
  CHECK(!try_catch.HasCaught());
  Script::Compile(v8_str("throw 0"))->Run();
  CHECK(try_catch.HasCaught());
  CHECK_EQ(0, try_catch.Exception()->Int32Value());
}

// from test-api.cc:2684
void
test_CatchExceptionFromWith()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::TryCatch try_catch;
  CHECK(!try_catch.HasCaught());
  Script::Compile(v8_str("var o = {}; with (o) { throw 42; }"))->Run();
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:2694
void
test_TryCatchAndFinallyHidingException()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::TryCatch try_catch;
  CHECK(!try_catch.HasCaught());
  CompileRun("function f(k) { try { this[k]; } finally { return 0; } };");
  CompileRun("f({toString: function() { throw 42; }});");
  CHECK(!try_catch.HasCaught());
}

// from test-api.cc:2711
void
test_TryCatchAndFinally()
{
  v8::HandleScope scope;
  LocalContext context;
  context->Global()->Set(
      v8_str("native_with_try_catch"),
      v8::FunctionTemplate::New(WithTryCatch)->GetFunction());
  v8::TryCatch try_catch;
  CHECK(!try_catch.HasCaught());
  CompileRun(
      "try {\n"
      "  throw new Error('a');\n"
      "} finally {\n"
      "  native_with_try_catch();\n"
      "}\n");
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:2729
void
test_Equality()
{
  v8::HandleScope scope;
  LocalContext context;
  // Check that equality works at all before relying on CHECK_EQ
  CHECK(v8_str("a")->Equals(v8_str("a")));
  CHECK(!v8_str("a")->Equals(v8_str("b")));

  CHECK_EQ(v8_str("a"), v8_str("a"));
  CHECK_NE(v8_str("a"), v8_str("b"));
  CHECK_EQ(v8_num(1), v8_num(1));
  CHECK_EQ(v8_num(1.00), v8_num(1));
  CHECK_NE(v8_num(1), v8_num(2));

  // Assume String is not symbol.
  CHECK(v8_str("a")->StrictEquals(v8_str("a")));
  CHECK(!v8_str("a")->StrictEquals(v8_str("b")));
  CHECK(!v8_str("5")->StrictEquals(v8_num(5)));
  CHECK(v8_num(1)->StrictEquals(v8_num(1)));
  CHECK(!v8_num(1)->StrictEquals(v8_num(2)));
  CHECK(v8_num(0)->StrictEquals(v8_num(-0)));
  Local<Value> not_a_number = v8_num(i::OS::nan_value());
  CHECK(!not_a_number->StrictEquals(not_a_number));
  CHECK(v8::False()->StrictEquals(v8::False()));
  CHECK(!v8::False()->StrictEquals(v8::Undefined()));

  v8::Handle<v8::Object> obj = v8::Object::New();
  v8::Persistent<v8::Object> alias = v8::Persistent<v8::Object>::New(obj);
  CHECK(alias->StrictEquals(obj));
  alias.Dispose();
}

// from test-api.cc:2761
void
test_MultiRun()
{
  v8::HandleScope scope;
  LocalContext context;
  Local<Script> script = Script::Compile(v8_str("x"));
  for (int i = 0; i < 10; i++)
    script->Run();
}

// from test-api.cc:2779
void
test_SimplePropertyRead()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut"));
  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());
  Local<Script> script = Script::Compile(v8_str("obj.x"));
  for (int i = 0; i < 10; i++) {
    Local<Value> result = script->Run();
    CHECK_EQ(result, v8_str("x"));
  }
}

// from test-api.cc:2792
void
test_DefinePropertyOnAPIAccessor()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut"));
  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());

  // Uses getOwnPropertyDescriptor to check the configurable status
  Local<Script> script_desc
    = Script::Compile(v8_str("var prop = Object.getOwnPropertyDescriptor( "
                             "obj, 'x');"
                             "prop.configurable;"));
  Local<Value> result = script_desc->Run();
  CHECK_EQ(result->BooleanValue(), true);

  // Redefine get - but still configurable
  Local<Script> script_define
    = Script::Compile(v8_str("var desc = { get: function(){return 42; },"
                             "            configurable: true };"
                             "Object.defineProperty(obj, 'x', desc);"
                             "obj.x"));
  result = script_define->Run();
  CHECK_EQ(result, v8_num(42));

  // Check that the accessor is still configurable
  result = script_desc->Run();
  CHECK_EQ(result->BooleanValue(), true);

  // Redefine to a non-configurable
  script_define
    = Script::Compile(v8_str("var desc = { get: function(){return 43; },"
                             "             configurable: false };"
                             "Object.defineProperty(obj, 'x', desc);"
                             "obj.x"));
  result = script_define->Run();
  CHECK_EQ(result, v8_num(43));
  result = script_desc->Run();
  CHECK_EQ(result->BooleanValue(), false);

  // Make sure that it is not possible to redefine again
  v8::TryCatch try_catch;
  result = script_define->Run();
  CHECK(try_catch.HasCaught());
  String::AsciiValue exception_value(try_catch.Exception());
  CHECK_EQ(*exception_value,
           "TypeError: Cannot redefine property: defineProperty");
}

// from test-api.cc:2840
void
test_DefinePropertyOnDefineGetterSetter()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut"));
  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());

  Local<Script> script_desc = Script::Compile(v8_str("var prop ="
                                    "Object.getOwnPropertyDescriptor( "
                                    "obj, 'x');"
                                    "prop.configurable;"));
  Local<Value> result = script_desc->Run();
  CHECK_EQ(result->BooleanValue(), true);

  Local<Script> script_define =
    Script::Compile(v8_str("var desc = {get: function(){return 42; },"
                           "            configurable: true };"
                           "Object.defineProperty(obj, 'x', desc);"
                           "obj.x"));
  result = script_define->Run();
  CHECK_EQ(result, v8_num(42));


  result = script_desc->Run();
  CHECK_EQ(result->BooleanValue(), true);


  script_define =
    Script::Compile(v8_str("var desc = {get: function(){return 43; },"
                           "            configurable: false };"
                           "Object.defineProperty(obj, 'x', desc);"
                           "obj.x"));
  result = script_define->Run();
  CHECK_EQ(result, v8_num(43));
  result = script_desc->Run();

  CHECK_EQ(result->BooleanValue(), false);

  v8::TryCatch try_catch;
  result = script_define->Run();
  CHECK(try_catch.HasCaught());
  String::AsciiValue exception_value(try_catch.Exception());
  CHECK_EQ(*exception_value,
           "TypeError: Cannot redefine property: defineProperty");
}

// from test-api.cc:2893
void
test_DefineAPIAccessorOnObject()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  LocalContext context;

  context->Global()->Set(v8_str("obj1"), templ->NewInstance());
  CompileRun("var obj2 = {};");

  CHECK(CompileRun("obj1.x")->IsUndefined());
  CHECK(CompileRun("obj2.x")->IsUndefined());

  CHECK(GetGlobalProperty(&context, "obj1")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));

  ExpectString("obj1.x", "x");
  CHECK(CompileRun("obj2.x")->IsUndefined());

  CHECK(GetGlobalProperty(&context, "obj2")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));

  ExpectString("obj1.x", "x");
  ExpectString("obj2.x", "x");

  ExpectTrue("Object.getOwnPropertyDescriptor(obj1, 'x').configurable");
  ExpectTrue("Object.getOwnPropertyDescriptor(obj2, 'x').configurable");

  CompileRun("Object.defineProperty(obj1, 'x',"
             "{ get: function() { return 'y'; }, configurable: true })");

  ExpectString("obj1.x", "y");
  ExpectString("obj2.x", "x");

  CompileRun("Object.defineProperty(obj2, 'x',"
             "{ get: function() { return 'y'; }, configurable: true })");

  ExpectString("obj1.x", "y");
  ExpectString("obj2.x", "y");

  ExpectTrue("Object.getOwnPropertyDescriptor(obj1, 'x').configurable");
  ExpectTrue("Object.getOwnPropertyDescriptor(obj2, 'x').configurable");

  CHECK(GetGlobalProperty(&context, "obj1")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));
  CHECK(GetGlobalProperty(&context, "obj2")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));

  ExpectString("obj1.x", "x");
  ExpectString("obj2.x", "x");

  ExpectTrue("Object.getOwnPropertyDescriptor(obj1, 'x').configurable");
  ExpectTrue("Object.getOwnPropertyDescriptor(obj2, 'x').configurable");

  // Define getters/setters, but now make them not configurable.
  CompileRun("Object.defineProperty(obj1, 'x',"
             "{ get: function() { return 'z'; }, configurable: false })");
  CompileRun("Object.defineProperty(obj2, 'x',"
             "{ get: function() { return 'z'; }, configurable: false })");

  ExpectTrue("!Object.getOwnPropertyDescriptor(obj1, 'x').configurable");
  ExpectTrue("!Object.getOwnPropertyDescriptor(obj2, 'x').configurable");

  ExpectString("obj1.x", "z");
  ExpectString("obj2.x", "z");

  CHECK(!GetGlobalProperty(&context, "obj1")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));
  CHECK(!GetGlobalProperty(&context, "obj2")->
      SetAccessor(v8_str("x"), GetXValue, NULL, v8_str("donut")));

  ExpectString("obj1.x", "z");
  ExpectString("obj2.x", "z");
}

// from test-api.cc:2967
void
test_DontDeleteAPIAccessorsCannotBeOverriden()
{ }

// from test-api.cc:3025
void
test_ElementAPIAccessor()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  LocalContext context;

  context->Global()->Set(v8_str("obj1"), templ->NewInstance());
  CompileRun("var obj2 = {};");

  CHECK(GetGlobalProperty(&context, "obj1")->SetAccessor(
        v8_str("239"),
        Get239Value, NULL,
        v8_str("donut")));
  CHECK(GetGlobalProperty(&context, "obj2")->SetAccessor(
        v8_str("239"),
        Get239Value, NULL,
        v8_str("donut")));

  ExpectString("obj1[239]", "239");
  ExpectString("obj2[239]", "239");
  ExpectString("obj1['239']", "239");
  ExpectString("obj2['239']", "239");
}

// from test-api.cc:3063
void
test_SimplePropertyWrite()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"), GetXValue, SetXValue, v8_str("donut"));
  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());
  Local<Script> script = Script::Compile(v8_str("obj.x = 4;"));
  for (int i = 0; i < 10; i++) {
    CHECK(xValue.IsEmpty());
    script->Run();
    CHECK_EQ(v8_num(4), xValue);
    xValue.Dispose();
    xValue = v8::Persistent<Value>();
  }
}

// from test-api.cc:3088
void
test_NamedInterceptorPropertyRead()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetNamedPropertyHandler(XPropertyGetter);
  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());
  Local<Script> script = Script::Compile(v8_str("obj.x"));
  for (int i = 0; i < 10; i++) {
    Local<Value> result = script->Run();
    CHECK_EQ(result, v8_str("x"));
  }
}

// from test-api.cc:3102
void
test_NamedInterceptorDictionaryIC()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetNamedPropertyHandler(XPropertyGetter);
  LocalContext context;
  // Create an object with a named interceptor.
  context->Global()->Set(v8_str("interceptor_obj"), templ->NewInstance());
  Local<Script> script = Script::Compile(v8_str("interceptor_obj.x"));
  for (int i = 0; i < 10; i++) {
    Local<Value> result = script->Run();
    CHECK_EQ(result, v8_str("x"));
  }
  // Create a slow case object and a function accessing a property in
  // that slow case object (with dictionary probing in generated
  // code). Then force object with a named interceptor into slow-case,
  // pass it to the function, and check that the interceptor is called
  // instead of accessing the local property.
  Local<Value> result =
      CompileRun("function get_x(o) { return o.x; };"
                 "var obj = { x : 42, y : 0 };"
                 "delete obj.y;"
                 "for (var i = 0; i < 10; i++) get_x(obj);"
                 "interceptor_obj.x = 42;"
                 "interceptor_obj.y = 10;"
                 "delete interceptor_obj.y;"
                 "get_x(interceptor_obj)");
  CHECK_EQ(result, v8_str("x"));
}

// from test-api.cc:3132
void
test_NamedInterceptorDictionaryICMultipleContext()
{ }

// from test-api.cc:3186
void
test_NamedInterceptorMapTransitionRead()
{ }

// from test-api.cc:3223
void
test_IndexedInterceptorWithIndexedAccessor()
{ }

// from test-api.cc:3260
void
test_IndexedInterceptorWithGetOwnPropertyDescriptor()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetIndexedPropertyHandler(IdentityIndexedPropertyGetter);

  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());

  // Check fast object case.
  const char* fast_case_code =
      "Object.getOwnPropertyDescriptor(obj, 0).value.toString()";
  ExpectString(fast_case_code, "0");

  // Check slow case.
  const char* slow_case_code =
      "obj.x = 1; delete obj.x;"
      "Object.getOwnPropertyDescriptor(obj, 1).value.toString()";
  ExpectString(slow_case_code, "1");
}

// from test-api.cc:3281
void
test_IndexedInterceptorWithNoSetter()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetIndexedPropertyHandler(IdentityIndexedPropertyGetter);

  LocalContext context;
  context->Global()->Set(v8_str("obj"), templ->NewInstance());

  const char* code =
      "try {"
      "  obj[0] = 239;"
      "  for (var i = 0; i < 100; i++) {"
      "    var v = obj[0];"
      "    if (v != 0) throw 'Wrong value ' + v + ' at iteration ' + i;"
      "  }"
      "  'PASSED'"
      "} catch(e) {"
      "  e"
      "}";
  ExpectString(code, "PASSED");
}

// from test-api.cc:3304
void
test_IndexedInterceptorWithAccessorCheck()
{ }

// from test-api.cc:3328
void
test_IndexedInterceptorWithAccessorCheckSwitchedOn()
{ }

// from test-api.cc:3358
void
test_IndexedInterceptorWithDifferentIndices()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetIndexedPropertyHandler(IdentityIndexedPropertyGetter);

  LocalContext context;
  Local<v8::Object> obj = templ->NewInstance();
  context->Global()->Set(v8_str("obj"), obj);

  const char* code =
      "try {"
      "  for (var i = 0; i < 100; i++) {"
      "    var v = obj[i];"
      "    if (v != i) throw 'Wrong value ' + v + ' at iteration ' + i;"
      "  }"
      "  'PASSED'"
      "} catch(e) {"
      "  e"
      "}";
  ExpectString(code, "PASSED");
}

// from test-api.cc:3381
void
test_IndexedInterceptorWithNegativeIndices()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetIndexedPropertyHandler(IdentityIndexedPropertyGetter);

  LocalContext context;
  Local<v8::Object> obj = templ->NewInstance();
  context->Global()->Set(v8_str("obj"), obj);

  const char* code =
      "try {"
      "  for (var i = 0; i < 100; i++) {"
      "    var expected = i;"
      "    var key = i;"
      "    if (i == 25) {"
      "       key = -1;"
      "       expected = undefined;"
      "    }"
      "    if (i == 50) {"
      "       /* probe minimal Smi number on 32-bit platforms */"
      "       key = -(1 << 30);"
      "       expected = undefined;"
      "    }"
      "    if (i == 75) {"
      "       /* probe minimal Smi number on 64-bit platforms */"
      "       key = 1 << 31;"
      "       expected = undefined;"
      "    }"
      "    var v = obj[key];"
      "    if (v != expected) throw 'Wrong value ' + v + ' at iteration ' + i;"
      "  }"
      "  'PASSED'"
      "} catch(e) {"
      "  e"
      "}";
  ExpectString(code, "PASSED");
}

// from test-api.cc:3420
void
test_IndexedInterceptorWithNotSmiLookup()
{ }

// from test-api.cc:3449
void
test_IndexedInterceptorGoingMegamorphic()
{ }

// from test-api.cc:3479
void
test_IndexedInterceptorReceiverTurningSmi()
{ }

// from test-api.cc:3509
void
test_IndexedInterceptorOnProto()
{ }

// from test-api.cc:3533
void
test_MultiContexts()
{ }

// from test-api.cc:3566
void
test_FunctionPrototypeAcrossContexts()
{ }

// from test-api.cc:3596
void
test_Regress892105()
{ }

// from test-api.cc:3619
void
test_UndetectableObject()
{
  v8::HandleScope scope;
  LocalContext env;

  Local<v8::FunctionTemplate> desc =
      v8::FunctionTemplate::New(0, v8::Handle<Value>());
  desc->InstanceTemplate()->MarkAsUndetectable();  // undetectable

  Local<v8::Object> obj = desc->GetFunction()->NewInstance();
  env->Global()->Set(v8_str("undetectable"), obj);

  ExpectString("undetectable.toString()", "[object Object]");
  ExpectString("typeof undetectable", "undefined");
  ExpectString("typeof(undetectable)", "undefined");
  ExpectBoolean("typeof undetectable == 'undefined'", true);
  ExpectBoolean("typeof undetectable == 'object'", false);
  ExpectBoolean("if (undetectable) { true; } else { false; }", false);
  ExpectBoolean("!undetectable", true);

  ExpectObject("true&&undetectable", obj);
  ExpectBoolean("false&&undetectable", false);
  ExpectBoolean("true||undetectable", true);
  ExpectObject("false||undetectable", obj);

  ExpectObject("undetectable&&true", obj);
  ExpectObject("undetectable&&false", obj);
  ExpectBoolean("undetectable||true", true);
  ExpectBoolean("undetectable||false", false);

  ExpectBoolean("undetectable==null", true);
  ExpectBoolean("null==undetectable", true);
  ExpectBoolean("undetectable==undefined", true);
  ExpectBoolean("undefined==undetectable", true);
  ExpectBoolean("undetectable==undetectable", true);


  ExpectBoolean("undetectable===null", false);
  ExpectBoolean("null===undetectable", false);
  ExpectBoolean("undetectable===undefined", false);
  ExpectBoolean("undefined===undetectable", false);
  ExpectBoolean("undetectable===undetectable", true);
}

// from test-api.cc:3664
void
test_ExtensibleOnUndetectable()
{
  v8::HandleScope scope;
  LocalContext env;

  Local<v8::FunctionTemplate> desc =
      v8::FunctionTemplate::New(0, v8::Handle<Value>());
  desc->InstanceTemplate()->MarkAsUndetectable();  // undetectable

  Local<v8::Object> obj = desc->GetFunction()->NewInstance();
  env->Global()->Set(v8_str("undetectable"), obj);

  Local<String> source = v8_str("undetectable.x = 42;"
                                "undetectable.x");

  Local<Script> script = Script::Compile(source);

  CHECK_EQ(v8::Integer::New(42), script->Run());

  ExpectBoolean("Object.isExtensible(undetectable)", true);

  source = v8_str("Object.preventExtensions(undetectable);");
  script = Script::Compile(source);
  script->Run();
  ExpectBoolean("Object.isExtensible(undetectable)", false);

  source = v8_str("undetectable.y = 2000;");
  script = Script::Compile(source);
  v8::TryCatch try_catch;
  Local<Value> result = script->Run();
  CHECK(result.IsEmpty());
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:3699
void
test_UndetectableString()
{ }

// from test-api.cc:3764
void
test_GlobalObjectTemplate()
{
  v8::HandleScope handle_scope;
  Local<ObjectTemplate> global_template = ObjectTemplate::New();
  global_template->Set(v8_str("JSNI_Log"),
                       v8::FunctionTemplate::New(HandleLogDelegator));
  v8::Persistent<Context> context = Context::New(0, global_template);
  Context::Scope context_scope(context);
  Script::Compile(v8_str("JSNI_Log('LOG')"))->Run();
  context.Dispose();
}

// from test-api.cc:3782
void
test_SimpleExtensions()
{ }

// from test-api.cc:3811
void
test_UseEvalFromExtension()
{ }

// from test-api.cc:3844
void
test_UseWithFromExtension()
{ }

// from test-api.cc:3859
void
test_AutoExtensions()
{ }

// from test-api.cc:3877
void
test_SyntaxErrorExtensions()
{ }

// from test-api.cc:3894
void
test_ExceptionExtensions()
{ }

// from test-api.cc:3915
void
test_NativeCallInExtensions()
{ }

// from test-api.cc:3943
void
test_ExtensionDependency()
{ }

// from test-api.cc:4010
void
test_FunctionLookup()
{ }

// from test-api.cc:4023
void
test_NativeFunctionConstructCall()
{ }

// from test-api.cc:4055
void
test_ErrorReporting()
{ }

// from test-api.cc:4082
void
test_RegexpOutOfMemory()
{ }

// from test-api.cc:4106
void
test_ErrorWithMissingScriptInfo()
{ }

// from test-api.cc:4169
void
test_WeakReference()
{ }

// from test-api.cc:4222
void
test_NoWeakRefCallbacksInScavenge()
{ }

// from test-api.cc:4268
void
test_Arguments()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> global = ObjectTemplate::New();
  global->Set(v8_str("f"), v8::FunctionTemplate::New(ArgumentsTestCallback));
  LocalContext context(NULL, global);
  args_fun = context->Global()->Get(v8_str("f")).As<Function>();
  v8_compile("f(1, 2, 3)")->Run();
}

// from test-api.cc:4309
void
test_Deleter()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> obj = ObjectTemplate::New();
  obj->SetNamedPropertyHandler(NoBlockGetterX, NULL, NULL, PDeleter, NULL);
  obj->SetIndexedPropertyHandler(NoBlockGetterI, NULL, NULL, IDeleter, NULL);
  LocalContext context;
  context->Global()->Set(v8_str("k"), obj->NewInstance());
  CompileRun(
    "k.foo = 'foo';"
    "k.bar = 'bar';"
    "k[2] = 2;"
    "k[4] = 4;");
  CHECK(v8_compile("delete k.foo")->Run()->IsFalse());
  CHECK(v8_compile("delete k.bar")->Run()->IsTrue());

  CHECK_EQ(v8_compile("k.foo")->Run(), v8_str("foo"));
  CHECK(v8_compile("k.bar")->Run()->IsUndefined());

  CHECK(v8_compile("delete k[2]")->Run()->IsFalse());
  CHECK(v8_compile("delete k[4]")->Run()->IsTrue());

  CHECK_EQ(v8_compile("k[2]")->Run(), v8_num(2));
  CHECK(v8_compile("k[4]")->Run()->IsUndefined());
}

// from test-api.cc:4372
void
test_Enumerators()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> obj = ObjectTemplate::New();
  obj->SetNamedPropertyHandler(GetK, NULL, NULL, NULL, NamedEnum);
  obj->SetIndexedPropertyHandler(IndexedGetK, NULL, NULL, NULL, IndexedEnum);
  LocalContext context;
  context->Global()->Set(v8_str("k"), obj->NewInstance());
  v8::Handle<v8::Array> result = v8::Handle<v8::Array>::Cast(CompileRun(
    "k[10] = 0;"
    "k.a = 0;"
    "k[5] = 0;"
    "k.b = 0;"
    "k[4294967295] = 0;"
    "k.c = 0;"
    "k[4294967296] = 0;"
    "k.d = 0;"
    "k[140000] = 0;"
    "k.e = 0;"
    "k[30000000000] = 0;"
    "k.f = 0;"
    "var result = [];"
    "for (var prop in k) {"
    "  result.push(prop);"
    "}"
    "result"));
  // Check that we get all the property names returned including the
  // ones from the enumerators in the right order: indexed properties
  // in numerical order, indexed interceptor properties, named
  // properties in insertion order, named interceptor properties.
  // This order is not mandated by the spec, so this test is just
  // documenting our behavior.
  CHECK_EQ(17, result->Length());
  // Indexed properties in numerical order.
  CHECK_EQ(v8_str("5"), result->Get(v8::Integer::New(0)));
  CHECK_EQ(v8_str("10"), result->Get(v8::Integer::New(1)));
  CHECK_EQ(v8_str("140000"), result->Get(v8::Integer::New(2)));
  CHECK_EQ(v8_str("4294967295"), result->Get(v8::Integer::New(3)));
  // Indexed interceptor properties in the order they are returned
  // from the enumerator interceptor.
  CHECK_EQ(v8_str("0"), result->Get(v8::Integer::New(4)));
  CHECK_EQ(v8_str("1"), result->Get(v8::Integer::New(5)));
  // Named properties in insertion order.
  CHECK_EQ(v8_str("a"), result->Get(v8::Integer::New(6)));
  CHECK_EQ(v8_str("b"), result->Get(v8::Integer::New(7)));
  CHECK_EQ(v8_str("c"), result->Get(v8::Integer::New(8)));
  CHECK_EQ(v8_str("4294967296"), result->Get(v8::Integer::New(9)));
  CHECK_EQ(v8_str("d"), result->Get(v8::Integer::New(10)));
  CHECK_EQ(v8_str("e"), result->Get(v8::Integer::New(11)));
  CHECK_EQ(v8_str("30000000000"), result->Get(v8::Integer::New(12)));
  CHECK_EQ(v8_str("f"), result->Get(v8::Integer::New(13)));
  // Named interceptor properties.
  CHECK_EQ(v8_str("foo"), result->Get(v8::Integer::New(14)));
  CHECK_EQ(v8_str("bar"), result->Get(v8::Integer::New(15)));
  CHECK_EQ(v8_str("baz"), result->Get(v8::Integer::New(16)));
}

// from test-api.cc:4486
void
test_GetterHolders()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> obj = ObjectTemplate::New();
  obj->SetAccessor(v8_str("p1"), PGetter);
  obj->SetAccessor(v8_str("p2"), PGetter);
  obj->SetAccessor(v8_str("p3"), PGetter);
  obj->SetAccessor(v8_str("p4"), PGetter);
  p_getter_count = 0;
  RunHolderTest(obj);
  CHECK_EQ(40, p_getter_count);
}

// from test-api.cc:4499
void
test_PreInterceptorHolders()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> obj = ObjectTemplate::New();
  obj->SetNamedPropertyHandler(PGetter2);
  p_getter_count2 = 0;
  RunHolderTest(obj);
  CHECK_EQ(40, p_getter_count2);
}

// from test-api.cc:4509
void
test_ObjectInstantiation()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("t"), PGetter2);
  LocalContext context;
  context->Global()->Set(v8_str("o"), templ->NewInstance());
  for (int i = 0; i < 100; i++) {
    v8::HandleScope inner_scope;
    v8::Handle<v8::Object> obj = templ->NewInstance();
    CHECK_NE(obj, context->Global()->Get(v8_str("o")));
    context->Global()->Set(v8_str("o2"), obj);
    v8::Handle<Value> value =
        Script::Compile(v8_str("o.__proto__ === o2.__proto__"))->Run();
    CHECK_EQ(v8::True(), value);
    context->Global()->Set(v8_str("o"), obj);
  }
}

// from test-api.cc:4549
void
test_StringWrite()
{
  v8::HandleScope scope;
  v8::Handle<String> str = v8_str("abcde");
  // abc<Icelandic eth><Unicode snowman>.
  v8::Handle<String> str2 = v8_str("abc\303\260\342\230\203");

  CHECK_EQ(5, str2->Length());

  char buf[100];
  char utf8buf[100];
  uint16_t wbuf[100];
  int len;
  int charlen;

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, sizeof(utf8buf), &charlen);
  CHECK_EQ(9, len);
  CHECK_EQ(5, charlen);
  CHECK_EQ(0, strcmp(utf8buf, "abc\303\260\342\230\203"));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 8, &charlen);
  CHECK_EQ(8, len);
  CHECK_EQ(5, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\303\260\342\230\203\1", 9));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 7, &charlen);
  CHECK_EQ(5, len);
  CHECK_EQ(4, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\303\260\1", 5));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 6, &charlen);
  CHECK_EQ(5, len);
  CHECK_EQ(4, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\303\260\1", 5));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 5, &charlen);
  CHECK_EQ(5, len);
  CHECK_EQ(4, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\303\260\1", 5));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 4, &charlen);
  CHECK_EQ(3, len);
  CHECK_EQ(3, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\1", 4));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 3, &charlen);
  CHECK_EQ(3, len);
  CHECK_EQ(3, charlen);
  CHECK_EQ(0, strncmp(utf8buf, "abc\1", 4));

  memset(utf8buf, 0x1, sizeof(utf8buf));
  len = str2->WriteUtf8(utf8buf, 2, &charlen);
  CHECK_EQ(2, len);
  CHECK_EQ(2, charlen);
  CHECK_EQ(2, strncmp(utf8buf, "ab\1", 3));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf);
  CHECK_EQ(5, len);
  len = str->Write(wbuf);
  CHECK_EQ(5, len);
  CHECK_EQ(0, strcmp("abcde", buf));
  uint16_t answer1[] = {'a', 'b', 'c', 'd', 'e', '\0'};
  CHECK_EQ(0, StrCmp16(answer1, wbuf));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 0, 4);
  CHECK_EQ(4, len);
  len = str->Write(wbuf, 0, 4);
  CHECK_EQ(4, len);
  CHECK_EQ(0, strncmp("abcd\1", buf, 5));
  uint16_t answer2[] = {'a', 'b', 'c', 'd', 0x101};
  CHECK_EQ(0, StrNCmp16(answer2, wbuf, 5));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 0, 5);
  CHECK_EQ(5, len);
  len = str->Write(wbuf, 0, 5);
  CHECK_EQ(5, len);
  CHECK_EQ(0, strncmp("abcde\1", buf, 6));
  uint16_t answer3[] = {'a', 'b', 'c', 'd', 'e', 0x101};
  CHECK_EQ(0, StrNCmp16(answer3, wbuf, 6));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 0, 6);
  CHECK_EQ(5, len);
  len = str->Write(wbuf, 0, 6);
  CHECK_EQ(5, len);
  CHECK_EQ(0, strcmp("abcde", buf));
  uint16_t answer4[] = {'a', 'b', 'c', 'd', 'e', '\0'};
  CHECK_EQ(0, StrCmp16(answer4, wbuf));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 4, -1);
  CHECK_EQ(1, len);
  len = str->Write(wbuf, 4, -1);
  CHECK_EQ(1, len);
  CHECK_EQ(0, strcmp("e", buf));
  uint16_t answer5[] = {'e', '\0'};
  CHECK_EQ(0, StrCmp16(answer5, wbuf));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 4, 6);
  CHECK_EQ(1, len);
  len = str->Write(wbuf, 4, 6);
  CHECK_EQ(1, len);
  CHECK_EQ(0, strcmp("e", buf));
  CHECK_EQ(0, StrCmp16(answer5, wbuf));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 4, 1);
  CHECK_EQ(1, len);
  len = str->Write(wbuf, 4, 1);
  CHECK_EQ(1, len);
  CHECK_EQ(0, strncmp("e\1", buf, 2));
  uint16_t answer6[] = {'e', 0x101};
  CHECK_EQ(0, StrNCmp16(answer6, wbuf, 2));

  memset(buf, 0x1, sizeof(buf));
  memset(wbuf, 0x1, sizeof(wbuf));
  len = str->WriteAscii(buf, 3, 1);
  CHECK_EQ(1, len);
  len = str->Write(wbuf, 3, 1);
  CHECK_EQ(1, len);
  CHECK_EQ(1, strncmp("d\1", buf, 2));
  uint16_t answer7[] = {'d', 0x101};
  CHECK_EQ(0, StrNCmp16(answer7, wbuf, 2));
}

// from test-api.cc:4692
void
test_ToArrayIndex()
{
  v8::HandleScope scope;
  LocalContext context;

  v8::Handle<String> str = v8_str("42");
  v8::Handle<v8::Uint32> index = str->ToArrayIndex();
  CHECK(!index.IsEmpty());
  CHECK_EQ(42.0, index->Uint32Value());
  str = v8_str("42asdf");
  index = str->ToArrayIndex();
  CHECK(index.IsEmpty());
  str = v8_str("-42");
  index = str->ToArrayIndex();
  CHECK(index.IsEmpty());
  str = v8_str("4294967295");
  index = str->ToArrayIndex();
  CHECK(!index.IsEmpty());
  CHECK_EQ(4294967295.0, index->Uint32Value());
  v8::Handle<v8::Number> num = v8::Number::New(1);
  index = num->ToArrayIndex();
  CHECK(!index.IsEmpty());
  CHECK_EQ(1.0, index->Uint32Value());
  num = v8::Number::New(-1);
  index = num->ToArrayIndex();
  CHECK(index.IsEmpty());
  v8::Handle<v8::Object> obj = v8::Object::New();
  index = obj->ToArrayIndex();
  CHECK(index.IsEmpty());
}

// from test-api.cc:4723
void
test_ErrorConstruction()
{
  v8::HandleScope scope;
  LocalContext context;

  v8::Handle<String> foo = v8_str("foo");
  v8::Handle<String> message = v8_str("message");
  v8::Handle<Value> range_error = v8::Exception::RangeError(foo);
  CHECK(range_error->IsObject());
  v8::Handle<v8::Object> range_obj = range_error.As<v8::Object>();
  CHECK(range_error.As<v8::Object>()->Get(message)->Equals(foo));
  v8::Handle<Value> reference_error = v8::Exception::ReferenceError(foo);
  CHECK(reference_error->IsObject());
  CHECK(reference_error.As<v8::Object>()->Get(message)->Equals(foo));
  v8::Handle<Value> syntax_error = v8::Exception::SyntaxError(foo);
  CHECK(syntax_error->IsObject());
  CHECK(syntax_error.As<v8::Object>()->Get(message)->Equals(foo));
  v8::Handle<Value> type_error = v8::Exception::TypeError(foo);
  CHECK(type_error->IsObject());
  CHECK(type_error.As<v8::Object>()->Get(message)->Equals(foo));
  v8::Handle<Value> error = v8::Exception::Error(foo);
  CHECK(error->IsObject());
  CHECK(error.As<v8::Object>()->Get(message)->Equals(foo));
}

// from test-api.cc:4764
void
test_DeleteAccessor()
{
  v8::HandleScope scope;
  v8::Handle<v8::ObjectTemplate> obj = ObjectTemplate::New();
  obj->SetAccessor(v8_str("y"), YGetter, YSetter);
  LocalContext context;
  v8::Handle<v8::Object> holder = obj->NewInstance();
  context->Global()->Set(v8_str("holder"), holder);
  v8::Handle<Value> result = CompileRun(
      "holder.y = 11; holder.y = 12; holder.y");
  CHECK_EQ(12, result->Uint32Value());
}

// from test-api.cc:4777
void
test_TypeSwitch()
{ }

// from test-api.cc:4854
void
test_ApiUncaughtException()
{ }

// from test-api.cc:4894
void
test_ExceptionInNativeScript()
{ }

// from test-api.cc:4914
void
test_CompilationErrorUsingTryCatchHandler()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::TryCatch try_catch;
  Script::Compile(v8_str("This doesn't &*&@#$&*^ compile."));
  CHECK_NE(NULL, *try_catch.Exception());
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:4924
void
test_TryCatchFinallyUsingTryCatchHandler()
{
  v8::HandleScope scope;
  LocalContext env;
  v8::TryCatch try_catch;
  Script::Compile(v8_str("try { throw ''; } catch (e) {}"))->Run();
  CHECK(!try_catch.HasCaught());
  Script::Compile(v8_str("try { throw ''; } finally {}"))->Run();
  CHECK(try_catch.HasCaught());
  try_catch.Reset();
  Script::Compile(v8_str("(function() {"
                         "try { throw ''; } finally { return; }"
                         "})()"))->Run();
  CHECK(!try_catch.HasCaught());
  Script::Compile(v8_str("(function()"
                         "  { try { throw ''; } finally { throw 0; }"
                         "})()"))->Run();
  CHECK(try_catch.HasCaught());
}

// from test-api.cc:4945
void
test_SecurityHandler()
{ }

// from test-api.cc:5007
void
test_SecurityChecks()
{ }

// from test-api.cc:5052
void
test_SecurityChecksForPrototypeChain()
{ }

// from test-api.cc:5120
void
test_CrossDomainDelete()
{ }

// from test-api.cc:5153
void
test_CrossDomainIsPropertyEnumerable()
{ }

// from test-api.cc:5188
void
test_CrossDomainForIn()
{ }

// from test-api.cc:5221
void
test_ContextDetachGlobal()
{ }

// from test-api.cc:5285
void
test_DetachAndReattachGlobal()
{ }

// from test-api.cc:5411
void
test_AccessControl()
{ }

// from test-api.cc:5655
void
test_AccessControlES5()
{ }

// from test-api.cc:5721
void
test_AccessControlGetOwnPropertyNames()
{ }

// from test-api.cc:5771
void
test_GetOwnPropertyNamesWithInterceptor()
{ }

// from test-api.cc:5795
void
test_CrossDomainAccessors()
{ }

// from test-api.cc:5870
void
test_AccessControlIC()
{ }

// from test-api.cc:6019
void
test_AccessControlFlatten()
{ }

// from test-api.cc:6083
void
test_AccessControlInterceptorIC()
{ }

// from test-api.cc:6150
void
test_Version()
{
  v8::V8::GetVersion();
}

// from test-api.cc:6161
void
test_InstanceProperties()
{
  v8::HandleScope handle_scope;
  LocalContext context;

  Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  Local<ObjectTemplate> instance = t->InstanceTemplate();

  instance->Set(v8_str("x"), v8_num(42));
  instance->Set(v8_str("f"),
                v8::FunctionTemplate::New(InstanceFunctionCallback));

  Local<Value> o = t->GetFunction()->NewInstance();

  context->Global()->Set(v8_str("i"), o);
  Local<Value> value = Script::Compile(v8_str("i.x"))->Run();
  CHECK_EQ(42, value->Int32Value());

  value = Script::Compile(v8_str("i.f()"))->Run();
  CHECK_EQ(12, value->Int32Value());
}

// from test-api.cc:6190
void
test_GlobalObjectInstanceProperties()
{
  v8::HandleScope handle_scope;

  Local<Value> global_object;

  Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  t->InstanceTemplate()->SetNamedPropertyHandler(
      GlobalObjectInstancePropertiesGet);
  Local<ObjectTemplate> instance_template = t->InstanceTemplate();
  instance_template->Set(v8_str("x"), v8_num(42));
  instance_template->Set(v8_str("f"),
                         v8::FunctionTemplate::New(InstanceFunctionCallback));

  // The script to check how Crankshaft compiles missing global function
  // invocations.  function g is not defined and should throw on call.
  const char* script =
      "function wrapper(call) {"
      "  var x = 0, y = 1;"
      "  for (var i = 0; i < 1000; i++) {"
      "    x += i * 100;"
      "    y += i * 100;"
      "  }"
      "  if (call) g();"
      "}"
      "for (var i = 0; i < 17; i++) wrapper(false);"
      "var thrown = 0;"
      "try { wrapper(true); } catch (e) { thrown = 1; };"
      "thrown";

  {
    LocalContext env(NULL, instance_template);
    // Hold on to the global object so it can be used again in another
    // environment initialization.
    global_object = env->Global();

    Local<Value> value = Script::Compile(v8_str("x"))->Run();
    CHECK_EQ(42, value->Int32Value());
    value = Script::Compile(v8_str("f()"))->Run();
    CHECK_EQ(12, value->Int32Value());
    value = Script::Compile(v8_str(script))->Run();
    CHECK_EQ(1, value->Int32Value());
  }

  {
    // Create new environment reusing the global object.
    LocalContext env(NULL, instance_template, global_object);
    Local<Value> value = Script::Compile(v8_str("x"))->Run();
    CHECK_EQ(42, value->Int32Value());
    value = Script::Compile(v8_str("f()"))->Run();
    CHECK_EQ(12, value->Int32Value());
    value = Script::Compile(v8_str(script))->Run();
    CHECK_EQ(1, value->Int32Value());
  }
}

// from test-api.cc:6246
void
test_CallKnownGlobalReceiver()
{
  v8::HandleScope handle_scope;

  Local<Value> global_object;

  Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  Local<ObjectTemplate> instance_template = t->InstanceTemplate();

  // The script to check that we leave global object not
  // global object proxy on stack when we deoptimize from inside
  // arguments evaluation.
  // To provoke error we need to both force deoptimization
  // from arguments evaluation and to force CallIC to take
  // CallIC_Miss code path that can't cope with global proxy.
  const char* script =
      "function bar(x, y) { try { } finally { } }"
      "function baz(x) { try { } finally { } }"
      "function bom(x) { try { } finally { } }"
      "function foo(x) { bar([x], bom(2)); }"
      "for (var i = 0; i < 10000; i++) foo(1);"
      "foo";

  Local<Value> foo;
  {
    LocalContext env(NULL, instance_template);
    // Hold on to the global object so it can be used again in another
    // environment initialization.
    global_object = env->Global();
    foo = Script::Compile(v8_str(script))->Run();
  }

  {
    // Create new environment reusing the global object.
    LocalContext env(NULL, instance_template, global_object);
    env->Global()->Set(v8_str("foo"), foo);
    Local<Value> value = Script::Compile(v8_str("foo()"))->Run();
  }
}

// from test-api.cc:6323
void
test_ShadowObject()
{
  shadow_y = shadow_y_setter_call_count = shadow_y_getter_call_count = 0;
  v8::HandleScope handle_scope;

  Local<ObjectTemplate> global_template = v8::ObjectTemplate::New();
  LocalContext context(NULL, global_template);

  Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  t->InstanceTemplate()->SetNamedPropertyHandler(ShadowNamedGet);
  t->InstanceTemplate()->SetIndexedPropertyHandler(ShadowIndexedGet);
  Local<ObjectTemplate> proto = t->PrototypeTemplate();
  Local<ObjectTemplate> instance = t->InstanceTemplate();

  // Only allow calls of f on instances of t.
  Local<v8::Signature> signature = v8::Signature::New(t);
  proto->Set(v8_str("f"),
             v8::FunctionTemplate::New(ShadowFunctionCallback,
                                       Local<Value>(),
                                       signature));
  proto->Set(v8_str("x"), v8_num(12));

  instance->SetAccessor(v8_str("y"), ShadowYGetter, ShadowYSetter);

  Local<Value> o = t->GetFunction()->NewInstance();
  context->Global()->Set(v8_str("__proto__"), o);

  Local<Value> value =
      Script::Compile(v8_str("propertyIsEnumerable(0)"))->Run();
  CHECK(value->IsBoolean());
  CHECK(!value->BooleanValue());

  value = Script::Compile(v8_str("x"))->Run();
  CHECK_EQ(12, value->Int32Value());

  value = Script::Compile(v8_str("f()"))->Run();
  CHECK_EQ(42, value->Int32Value());

  Script::Compile(v8_str("y = 42"))->Run();
  CHECK_EQ(1, shadow_y_setter_call_count);
  value = Script::Compile(v8_str("y"))->Run();
  CHECK_EQ(1, shadow_y_getter_call_count);
  CHECK_EQ(42, value->Int32Value());
}

// from test-api.cc:6368
void
test_HiddenPrototype()
{
  v8::HandleScope handle_scope;
  LocalContext context;

  Local<v8::FunctionTemplate> t0 = v8::FunctionTemplate::New();
  t0->InstanceTemplate()->Set(v8_str("x"), v8_num(0));
  Local<v8::FunctionTemplate> t1 = v8::FunctionTemplate::New();
  t1->SetHiddenPrototype(true);
  t1->InstanceTemplate()->Set(v8_str("y"), v8_num(1));
  Local<v8::FunctionTemplate> t2 = v8::FunctionTemplate::New();
  t2->SetHiddenPrototype(true);
  t2->InstanceTemplate()->Set(v8_str("z"), v8_num(2));
  Local<v8::FunctionTemplate> t3 = v8::FunctionTemplate::New();
  t3->InstanceTemplate()->Set(v8_str("u"), v8_num(3));

  Local<v8::Object> o0 = t0->GetFunction()->NewInstance();
  Local<v8::Object> o1 = t1->GetFunction()->NewInstance();
  Local<v8::Object> o2 = t2->GetFunction()->NewInstance();
  Local<v8::Object> o3 = t3->GetFunction()->NewInstance();

  // Setting the prototype on an object skips hidden prototypes.
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  o0->Set(v8_str("__proto__"), o1);
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  o0->Set(v8_str("__proto__"), o2);
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  CHECK_EQ(2, o0->Get(v8_str("z"))->Int32Value());
  o0->Set(v8_str("__proto__"), o3);
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  CHECK_EQ(2, o0->Get(v8_str("z"))->Int32Value());
  CHECK_EQ(3, o0->Get(v8_str("u"))->Int32Value());

  // Getting the prototype of o0 should get the first visible one
  // which is o3.  Therefore, z should not be defined on the prototype
  // object.
  Local<Value> proto = o0->Get(v8_str("__proto__"));
  CHECK(proto->IsObject());
  CHECK(proto.As<v8::Object>()->Get(v8_str("z"))->IsUndefined());
}

// from test-api.cc:6412
void
test_SetPrototype()
{
  v8::HandleScope handle_scope;
  LocalContext context;

  Local<v8::FunctionTemplate> t0 = v8::FunctionTemplate::New();
  t0->InstanceTemplate()->Set(v8_str("x"), v8_num(0));
  Local<v8::FunctionTemplate> t1 = v8::FunctionTemplate::New();
  t1->SetHiddenPrototype(true);
  t1->InstanceTemplate()->Set(v8_str("y"), v8_num(1));
  Local<v8::FunctionTemplate> t2 = v8::FunctionTemplate::New();
  t2->SetHiddenPrototype(true);
  t2->InstanceTemplate()->Set(v8_str("z"), v8_num(2));
  Local<v8::FunctionTemplate> t3 = v8::FunctionTemplate::New();
  t3->InstanceTemplate()->Set(v8_str("u"), v8_num(3));

  Local<v8::Object> o0 = t0->GetFunction()->NewInstance();
  Local<v8::Object> o1 = t1->GetFunction()->NewInstance();
  Local<v8::Object> o2 = t2->GetFunction()->NewInstance();
  Local<v8::Object> o3 = t3->GetFunction()->NewInstance();

  // Setting the prototype on an object does not skip hidden prototypes.
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK(o0->SetPrototype(o1));
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  CHECK(o1->SetPrototype(o2));
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  CHECK_EQ(2, o0->Get(v8_str("z"))->Int32Value());
  CHECK(o2->SetPrototype(o3));
  CHECK_EQ(0, o0->Get(v8_str("x"))->Int32Value());
  CHECK_EQ(1, o0->Get(v8_str("y"))->Int32Value());
  CHECK_EQ(2, o0->Get(v8_str("z"))->Int32Value());
  CHECK_EQ(3, o0->Get(v8_str("u"))->Int32Value());

  // Getting the prototype of o0 should get the first visible one
  // which is o3.  Therefore, z should not be defined on the prototype
  // object.
  Local<Value> proto = o0->Get(v8_str("__proto__"));
  CHECK(proto->IsObject());
  CHECK_EQ(proto.As<v8::Object>(), o3);

  // However, Object::GetPrototype ignores hidden prototype.
  Local<Value> proto0 = o0->GetPrototype();
  CHECK(proto0->IsObject());
  CHECK_EQ(proto0.As<v8::Object>(), o1);

  Local<Value> proto1 = o1->GetPrototype();
  CHECK(proto1->IsObject());
  CHECK_EQ(proto1.As<v8::Object>(), o2);

  Local<Value> proto2 = o2->GetPrototype();
  CHECK(proto2->IsObject());
  CHECK_EQ(proto2.As<v8::Object>(), o3);
}

// from test-api.cc:6469
void
test_SetPrototypeThrows()
{ }

// from test-api.cc:6490
void
test_GetterSetterExceptions()
{
  v8::HandleScope handle_scope;
  LocalContext context;
  CompileRun(
    "function Foo() { };"
    "function Throw() { throw 5; };"
    "var x = { };"
    "x.__defineSetter__('set', Throw);"
    "x.__defineGetter__('get', Throw);");
  Local<v8::Object> x =
      Local<v8::Object>::Cast(context->Global()->Get(v8_str("x")));
  v8::TryCatch try_catch;
  x->Set(v8_str("set"), v8::Integer::New(8));
  x->Get(v8_str("get"));
  x->Set(v8_str("set"), v8::Integer::New(8));
  x->Get(v8_str("get"));
  x->Set(v8_str("set"), v8::Integer::New(8));
  x->Get(v8_str("get"));
  x->Set(v8_str("set"), v8::Integer::New(8));
  x->Get(v8_str("get"));
}

// from test-api.cc:6513
void
test_Constructor()
{ }

// from test-api.cc:6526
void
test_FunctionDescriptorException()
{
  v8::HandleScope handle_scope;
  LocalContext context;
  Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  templ->SetClassName(v8_str("Fun"));
  Local<Function> cons = templ->GetFunction();
  context->Global()->Set(v8_str("Fun"), cons);
  Local<Value> value = CompileRun(
    "function test() {"
    "  try {"
    "    (new Fun()).blah()"
    "  } catch (e) {"
    "    var str = String(e);"
    "    if (str.indexOf('TypeError') == -1) return 1;"
    "    if (str.indexOf('[object Fun]') != -1) return 2;"
    "    if (str.indexOf('#<Fun>') == -1) return 3;"
    "    return 0;"
    "  }"
    "  return 4;"
    "}"
    "test();");
  CHECK_EQ(0, value->Int32Value());
}

// from test-api.cc:6551
void
test_EvalAliasedDynamic()
{ }

// from test-api.cc:6585
void
test_CrossEval()
{ }

// from test-api.cc:6668
void
test_EvalInDetachedGlobal()
{ }

// from test-api.cc:6703
void
test_CrossLazyLoad()
{ }

// from test-api.cc:6738
void
test_CallAsFunction()
{ }

// from test-api.cc:6804
void
test_HandleIteration()
{ }

// from test-api.cc:6839
void
test_InterceptorHasOwnProperty()
{ }

// from test-api.cc:6871
void
test_InterceptorHasOwnPropertyCausingGC()
{ }

// from test-api.cc:6927
void
test_InterceptorLoadIC()
{ }

// from test-api.cc:6949
void
test_InterceptorLoadICWithFieldOnHolder()
{ }

// from test-api.cc:6960
void
test_InterceptorLoadICWithSubstitutedProto()
{ }

// from test-api.cc:6971
void
test_InterceptorLoadICWithPropertyOnProto()
{ }

// from test-api.cc:6982
void
test_InterceptorLoadICUndefined()
{ }

// from test-api.cc:6992
void
test_InterceptorLoadICWithOverride()
{ }

// from test-api.cc:7012
void
test_InterceptorLoadICFieldNotNeeded()
{ }

// from test-api.cc:7032
void
test_InterceptorLoadICInvalidatedField()
{ }

// from test-api.cc:7063
void
test_InterceptorLoadICPostInterceptor()
{ }

// from test-api.cc:7088
void
test_InterceptorLoadICInvalidatedFieldViaGlobal()
{ }

// from test-api.cc:7113
void
test_InterceptorLoadICWithCallbackOnHolder()
{ }

// from test-api.cc:7142
void
test_InterceptorLoadICWithCallbackOnProto()
{ }

// from test-api.cc:7175
void
test_InterceptorLoadICForCallbackWithOverride()
{ }

// from test-api.cc:7203
void
test_InterceptorLoadICCallbackNotNeeded()
{ }

// from test-api.cc:7231
void
test_InterceptorLoadICInvalidatedCallback()
{ }

// from test-api.cc:7263
void
test_InterceptorLoadICInvalidatedCallbackViaGlobal()
{ }

// from test-api.cc:7299
void
test_InterceptorReturningZero()
{ }

// from test-api.cc:7315
void
test_InterceptorStoreIC()
{ }

// from test-api.cc:7330
void
test_InterceptorStoreICWithNoSetter()
{ }

// from test-api.cc:7360
void
test_InterceptorCallIC()
{ }

// from test-api.cc:7379
void
test_InterceptorCallICSeesOthers()
{ }

// from test-api.cc:7407
void
test_InterceptorCallICCacheableNotNeeded()
{ }

// from test-api.cc:7427
void
test_InterceptorCallICInvalidatedCacheable()
{ }

// from test-api.cc:7465
void
test_InterceptorCallICConstantFunctionUsed()
{ }

// from test-api.cc:7486
void
test_InterceptorCallICConstantFunctionNotNeeded()
{ }

// from test-api.cc:7508
void
test_InterceptorCallICInvalidatedConstantFunction()
{ }

// from test-api.cc:7538
void
test_InterceptorCallICInvalidatedConstantFunctionViaGlobal()
{ }

// from test-api.cc:7563
void
test_InterceptorCallICCachedFromGlobal()
{ }

// from test-api.cc:7641
void
test_CallICFastApi_DirectCall_GCMoveStub()
{ }

// from test-api.cc:7665
void
test_CallICFastApi_DirectCall_Throw()
{ }

// from test-api.cc:7696
void
test_LoadICFastApi_DirectCall_GCMoveStub()
{ }

// from test-api.cc:7718
void
test_LoadICFastApi_DirectCall_Throw()
{ }

// from test-api.cc:7734
void
test_InterceptorCallICFastApi_TrivialSignature()
{ }

// from test-api.cc:7761
void
test_InterceptorCallICFastApi_SimpleSignature()
{ }

// from test-api.cc:7791
void
test_InterceptorCallICFastApi_SimpleSignature_Miss1()
{ }

// from test-api.cc:7827
void
test_InterceptorCallICFastApi_SimpleSignature_Miss2()
{ }

// from test-api.cc:7863
void
test_InterceptorCallICFastApi_SimpleSignature_Miss3()
{ }

// from test-api.cc:7902
void
test_InterceptorCallICFastApi_SimpleSignature_TypeError()
{ }

// from test-api.cc:7941
void
test_CallICFastApi_TrivialSignature()
{ }

// from test-api.cc:7964
void
test_CallICFastApi_SimpleSignature()
{ }

// from test-api.cc:7990
void
test_CallICFastApi_SimpleSignature_Miss1()
{ }

// from test-api.cc:8021
void
test_CallICFastApi_SimpleSignature_Miss2()
{ }

// from test-api.cc:8070
void
test_InterceptorKeyedCallICKeyChange1()
{ }

// from test-api.cc:8094
void
test_InterceptorKeyedCallICKeyChange2()
{ }

// from test-api.cc:8121
void
test_InterceptorKeyedCallICKeyChangeOnGlobal()
{ }

// from test-api.cc:8146
void
test_InterceptorKeyedCallICFromGlobal()
{ }

// from test-api.cc:8170
void
test_InterceptorKeyedCallICMapChangeBefore()
{ }

// from test-api.cc:8192
void
test_InterceptorKeyedCallICMapChangeAfter()
{ }

// from test-api.cc:8228
void
test_InterceptorICReferenceErrors()
{ }

// from test-api.cc:8274
void
test_InterceptorICGetterExceptions()
{ }

// from test-api.cc:8317
void
test_InterceptorICSetterExceptions()
{ }

// from test-api.cc:8336
void
test_NullNamedInterceptor()
{ }

// from test-api.cc:8351
void
test_NullIndexedInterceptor()
{ }

// from test-api.cc:8365
void
test_NamedPropertyHandlerGetterAttributes()
{ }

// from test-api.cc:8391
void
test_Overriding()
{ }

// from test-api.cc:8457
void
test_IsConstructCall()
{ }

// from test-api.cc:8474
void
test_ObjectProtoToString()
{
  v8::HandleScope scope;
  Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New();
  templ->SetClassName(v8_str("MyClass"));

  LocalContext context;

  Local<String> customized_tostring = v8_str("customized toString");

  // Replace Object.prototype.toString
  v8_compile("Object.prototype.toString = function() {"
                  "  return 'customized toString';"
                  "}")->Run();

  // Normal ToString call should call replaced Object.prototype.toString
  Local<v8::Object> instance = templ->GetFunction()->NewInstance();
  Local<String> value = instance->ToString();
  CHECK(value->IsString() && value->Equals(customized_tostring));

  // ObjectProtoToString should not call replace toString function.
  value = instance->ObjectProtoToString();
  CHECK(value->IsString() && value->Equals(v8_str("[object MyClass]")));

  // Check global
  value = context->Global()->ObjectProtoToString();
  CHECK(value->IsString() && value->Equals(v8_str("[object global]")));

  // Check ordinary object
  Local<Value> object = v8_compile("new Object()")->Run();
  value = object.As<v8::Object>()->ObjectProtoToString();
  CHECK(value->IsString() && value->Equals(v8_str("[object Object]")));
}

// from test-api.cc:8508
void
test_ObjectGetConstructorName()
{
  v8::HandleScope scope;
  LocalContext context;
  v8_compile("function Parent() {};"
             "function Child() {};"
             "Child.prototype = new Parent();"
             "var outer = { inner: function() { } };" //XXXzpao naming this function properly stops the crash
             "var p = new Parent();"
             "var c = new Child();"
             "var x = new outer.inner();")->Run();

  Local<v8::Value> p = context->Global()->Get(v8_str("p"));
  CHECK(p->IsObject() && p->ToObject()->GetConstructorName()->Equals(
      v8_str("Parent")));

  Local<v8::Value> c = context->Global()->Get(v8_str("c"));
  //XXXzpao These checks were split into 2 separate ones each to better determine the failure.
  //        When fixed, delete the lines marked XXX and uncomment the other lines.
  /*CHECK(c->IsObject() && c->ToObject()->GetConstructorName()->Equals(
      v8_str("Child")));*/
  CHECK(c->IsObject()); //XXX
  CHECK_EQ(v8_str("Child"), c->ToObject()->GetConstructorName()); //XXX

  Local<v8::Value> x = context->Global()->Get(v8_str("x"));
  /*CHECK(x->IsObject() && x->ToObject()->GetConstructorName()->Equals(
      v8_str("outer.inner")));*/
  CHECK(x->IsObject()); //XXX
  CHECK_EQ(v8_str("outer.inner"), x->ToObject()->GetConstructorName()); //XXX
}

// from test-api.cc:8659
void
test_Threading()
{ }

// from test-api.cc:8665
void
test_Threading2()
{ }

// from test-api.cc:8721
void
test_NestedLockers()
{ }

// from test-api.cc:8743
void
test_NestedLockersNoTryCatch()
{ }

// from test-api.cc:8763
void
test_RecursiveLocking()
{ }

// from test-api.cc:8779
void
test_LockUnlockLock()
{ }

// from test-api.cc:8836
void
test_DontLeakGlobalObjects()
{ }

// from test-api.cc:8880
void
test_NewPersistentHandleFromWeakCallback()
{
  LocalContext context;

  v8::Persistent<v8::Object> handle1, handle2;
  {
    v8::HandleScope scope;
    some_object = v8::Persistent<v8::Object>::New(v8::Object::New());
    handle1 = v8::Persistent<v8::Object>::New(v8::Object::New());
    handle2 = v8::Persistent<v8::Object>::New(v8::Object::New());
  }
  // Note: order is implementation dependent alas: currently
  // global handle nodes are processed by PostGarbageCollectionProcessing
  // in reverse allocation order, so if second allocated handle is deleted,
  // weak callback of the first handle would be able to 'reallocate' it.
  handle1.MakeWeak(NULL, NewPersistentHandleCallback);
  handle2.Dispose();
  i::Heap::CollectAllGarbage(false);
}

// from test-api.cc:8909
void
test_DoNotUseDeletedNodesInSecondLevelGc()
{
  LocalContext context;

  v8::Persistent<v8::Object> handle1, handle2;
  {
    v8::HandleScope scope;
    handle1 = v8::Persistent<v8::Object>::New(v8::Object::New());
    handle2 = v8::Persistent<v8::Object>::New(v8::Object::New());
  }
  handle1.MakeWeak(NULL, DisposeAndForceGcCallback);
  to_be_disposed = handle2;
  i::Heap::CollectAllGarbage(false);
}

// from test-api.cc:8934
void
test_NoGlobalHandlesOrphaningDueToWeakCallback()
{
  LocalContext context;

  v8::Persistent<v8::Object> handle1, handle2, handle3;
  {
    v8::HandleScope scope;
    handle3 = v8::Persistent<v8::Object>::New(v8::Object::New());
    handle2 = v8::Persistent<v8::Object>::New(v8::Object::New());
    handle1 = v8::Persistent<v8::Object>::New(v8::Object::New());
  }
  handle2.MakeWeak(NULL, DisposingCallback);
  handle3.MakeWeak(NULL, HandleCreatingCallback);
  i::Heap::CollectAllGarbage(false);
}

// from test-api.cc:8950
void
test_CheckForCrossContextObjectLiterals()
{
  v8::V8::Initialize();

  const int nof = 2;
  const char* sources[nof] = {
    "try { [ 2, 3, 4 ].forEach(5); } catch(e) { e.toString(); }",
    "Object()"
  };

  for (int i = 0; i < nof; i++) {
    const char* source = sources[i];
    { v8::HandleScope scope;
      LocalContext context;
      CompileRun(source);
    }
    { v8::HandleScope scope;
      LocalContext context;
      CompileRun(source);
    }
  }
}

// from test-api.cc:8983
void
test_NestedHandleScopeAndContexts()
{ }

// from test-api.cc:8994
void
test_ExternalAllocatedMemory()
{ }

// from test-api.cc:9003
void
test_DisposeEnteredContext()
{ }

// from test-api.cc:9018
void
test_Regress54()
{ }

// from test-api.cc:9035
void
test_CatchStackOverflow()
{ }

// from test-api.cc:9072
void
test_TryCatchSourceInfo()
{ }

// from test-api.cc:9108
void
test_CompilationCache()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::Handle<v8::String> source0 = v8::String::New("1234");
  v8::Handle<v8::String> source1 = v8::String::New("1234");
  v8::Handle<v8::Script> script0 =
      v8::Script::Compile(source0, v8::String::New("test.js"));
  v8::Handle<v8::Script> script1 =
      v8::Script::Compile(source1, v8::String::New("test.js"));
  v8::Handle<v8::Script> script2 =
      v8::Script::Compile(source0);  // different origin
  CHECK_EQ(1234, script0->Run()->Int32Value());
  CHECK_EQ(1234, script1->Run()->Int32Value());
  CHECK_EQ(1234, script2->Run()->Int32Value());
}


// from test-api.cc:9131
void
test_CallbackFunctionName()
{
  v8::HandleScope scope;
  LocalContext context;
  Local<ObjectTemplate> t = ObjectTemplate::New();
  t->Set(v8_str("asdf"), v8::FunctionTemplate::New(FunctionNameCallback));
  context->Global()->Set(v8_str("obj"), t->NewInstance());
  v8::Handle<v8::Value> value = CompileRun("obj.asdf.name");
  CHECK(value->IsString());
  v8::String::AsciiValue name(value);
  CHECK_EQ("asdf", *name);
}

// from test-api.cc:9144
void
test_DateAccess()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::Handle<v8::Value> date = v8::Date::New(1224744689038.0);
  CHECK(date->IsDate());
  CHECK_EQ(1224744689038.0, date.As<v8::Date>()->NumberValue());
}

// from test-api.cc:9164
void
test_PropertyEnumeration()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::Handle<v8::Value> obj = v8::Script::Compile(v8::String::New(
      "var result = [];"
      "result[0] = {};"
      "result[1] = {a: 1, b: 2};"
      "result[2] = [1, 2, 3];"
      "var proto = {x: 1, y: 2, z: 3};"
      "var x = { __proto__: proto, w: 0, z: 1 };"
      "result[3] = x;"
      "result;"))->Run();
  v8::Handle<v8::Array> elms = obj.As<v8::Array>();
  CHECK_EQ(4, elms->Length());
  int elmc0 = 0;
  const char** elmv0 = NULL;
  CheckProperties(elms->Get(v8::Integer::New(0)), elmc0, elmv0);
  int elmc1 = 2;
  const char* elmv1[] = {"a", "b"};
  CheckProperties(elms->Get(v8::Integer::New(1)), elmc1, elmv1);
  int elmc2 = 3;
  const char* elmv2[] = {"0", "1", "2"};
  CheckProperties(elms->Get(v8::Integer::New(2)), elmc2, elmv2);
  int elmc3 = 4;
  const char* elmv3[] = {"w", "z", "x", "y"};
  CheckProperties(elms->Get(v8::Integer::New(3)), elmc3, elmv3);
}

// from test-api.cc:9209
void
test_DisableAccessChecksWhileConfiguring()
{ }

// from test-api.cc:9240
void
test_AccessChecksReenabledCorrectly()
{ }

// from test-api.cc:9279
void
test_AccessControlRepeatedContextCreation()
{ }

// from test-api.cc:9295
void
test_TurnOnAccessCheck()
{ }

// from test-api.cc:9371
void
test_TurnOnAccessCheckAndRecompile()
{ }

// from test-api.cc:9462
void
test_PreCompile()
{
  // TODO(155): This test would break without the initialization of V8. This is
  // a workaround for now to make this test not fail.
  v8::V8::Initialize();
  const char* script = "function foo(a) { return a+1; }";
  v8::ScriptData* sd =
      v8::ScriptData::PreCompile(script, i::StrLength(script));
  CHECK_NE(sd->Length(), 0);
  CHECK_NE(sd->Data(), NULL);
  CHECK(!sd->HasError());
  delete sd;
}

// from test-api.cc:9476
void
test_PreCompileWithError()
{
  v8::V8::Initialize();
  const char* script = "function foo(a) { return 1 * * 2; }";
  v8::ScriptData* sd =
      v8::ScriptData::PreCompile(script, i::StrLength(script));
  CHECK(sd->HasError());
  delete sd;
}

// from test-api.cc:9486
void
test_Regress31661()
{
  v8::V8::Initialize();
  const char* script = " The Definintive Guide";
  v8::ScriptData* sd =
      v8::ScriptData::PreCompile(script, i::StrLength(script));
  CHECK(sd->HasError());
  delete sd;
}

// from test-api.cc:9497
void
test_PreCompileSerialization()
{
  v8::V8::Initialize();
  const char* script = "function foo(a) { return a+1; }";
  v8::ScriptData* sd =
      v8::ScriptData::PreCompile(script, i::StrLength(script));

  // Serialize.
  int serialized_data_length = sd->Length();
  char* serialized_data = i::NewArray<char>(serialized_data_length);
  memcpy(serialized_data, sd->Data(), serialized_data_length);

  // Deserialize.
  v8::ScriptData* deserialized_sd =
      v8::ScriptData::New(serialized_data, serialized_data_length);

  // Verify that the original is the same as the deserialized.
  CHECK_EQ(sd->Length(), deserialized_sd->Length());
  CHECK_EQ(0, memcmp(sd->Data(), deserialized_sd->Data(), sd->Length()));
  CHECK_EQ(sd->HasError(), deserialized_sd->HasError());

  delete sd;
  delete deserialized_sd;
}

// from test-api.cc:9523
void
test_PreCompileDeserializationError()
{
  v8::V8::Initialize();
  const char* data = "DONT CARE";
  int invalid_size = 3;
  v8::ScriptData* sd = v8::ScriptData::New(data, invalid_size);

  CHECK_EQ(0, sd->Length());

  delete sd;
}

// from test-api.cc:9536
void
test_PreCompileInvalidPreparseDataError()
{ }

// from test-api.cc:9584
void
test_PreCompileAPIVariationsAreSame()
{ }

// from test-api.cc:9622
void
test_DictionaryICLoadedFunction()
{ }

// from test-api.cc:9643
void
test_CrossContextNew()
{ }

// from test-api.cc:9775
void
test_RegExpInterruption()
{ }

// from test-api.cc:9883
void
test_ApplyInterruption()
{ }

// from test-api.cc:9905
void
test_ObjectClone()
{
  v8::HandleScope scope;
  LocalContext env;

  const char* sample =
    "var rv = {};"      \
    "rv.alpha = 'hello';" \
    "rv.beta = 123;"     \
    "rv;";

  // Create an object, verify basics.
  Local<Value> val = CompileRun(sample);
  CHECK(val->IsObject());
  Local<v8::Object> obj = val.As<v8::Object>();
  obj->Set(v8_str("gamma"), v8_str("cloneme"));

  CHECK_EQ(v8_str("hello"), obj->Get(v8_str("alpha")));
  CHECK_EQ(v8::Integer::New(123), obj->Get(v8_str("beta")));
  CHECK_EQ(v8_str("cloneme"), obj->Get(v8_str("gamma")));

  // Clone it.
  Local<v8::Object> clone = obj->Clone();
  CHECK_EQ(v8_str("hello"), clone->Get(v8_str("alpha")));
  CHECK_EQ(v8::Integer::New(123), clone->Get(v8_str("beta")));
  CHECK_EQ(v8_str("cloneme"), clone->Get(v8_str("gamma")));

  // Set a property on the clone, verify each object.
  clone->Set(v8_str("beta"), v8::Integer::New(456));
  CHECK_EQ(v8::Integer::New(123), obj->Get(v8_str("beta")));
  CHECK_EQ(v8::Integer::New(456), clone->Get(v8_str("beta")));
}

// from test-api.cc:9988
void
test_MorphCompositeStringTest()
{ }

// from test-api.cc:10041
void
test_CompileExternalTwoByteSource()
{ }

// from test-api.cc:10181
void
test_RegExpStringModification()
{ }

// from test-api.cc:10204
void
test_ReadOnlyPropertyInGlobalProto()
{ }

// from test-api.cc:10252
void
test_ForceSet()
{ }

// from test-api.cc:10293
void
test_ForceSetWithInterceptor()
{ }

// from test-api.cc:10340
void
test_ForceDelete()
{ }

// from test-api.cc:10375
void
test_ForceDeleteWithInterceptor()
{ }

// from test-api.cc:10410
void
test_ForceDeleteIC()
{ }

// from test-api.cc:10447
void
test_GetCallingContext()
{ }

// from test-api.cc:10500
void
test_InitGlobalVarInProtoChain()
{ }

// from test-api.cc:10516
void
test_ReplaceConstantFunction()
{ }

// from test-api.cc:10530
void
test_Regress16276()
{ }

// from test-api.cc:10547
void
test_PixelArray()
{ }

// from test-api.cc:10921
void
test_PixelArrayInfo()
{ }

// from test-api.cc:10953
void
test_PixelArrayWithInterceptor()
{ }

// from test-api.cc:11361
void
test_ExternalByteArray()
{ }

// from test-api.cc:11369
void
test_ExternalUnsignedByteArray()
{ }

// from test-api.cc:11377
void
test_ExternalShortArray()
{ }

// from test-api.cc:11385
void
test_ExternalUnsignedShortArray()
{ }

// from test-api.cc:11393
void
test_ExternalIntArray()
{ }

// from test-api.cc:11401
void
test_ExternalUnsignedIntArray()
{ }

// from test-api.cc:11409
void
test_ExternalFloatArray()
{ }

// from test-api.cc:11417
void
test_ExternalArrays()
{ }

// from test-api.cc:11446
void
test_ExternalArrayInfo()
{ }

// from test-api.cc:11457
void
test_ScriptContextDependence()
{
  v8::HandleScope scope;
  LocalContext c1;
  const char *source = "foo";
  v8::Handle<v8::Script> dep = v8::Script::Compile(v8::String::New(source));
  v8::Handle<v8::Script> indep = v8::Script::New(v8::String::New(source));
  c1->Global()->Set(v8::String::New("foo"), v8::Integer::New(100));
  CHECK_EQ(dep->Run()->Int32Value(), 100);
  CHECK_EQ(indep->Run()->Int32Value(), 100);
  LocalContext c2;
  c2->Global()->Set(v8::String::New("foo"), v8::Integer::New(101));
  CHECK_EQ(dep->Run()->Int32Value(), 100);
  CHECK_EQ(indep->Run()->Int32Value(), 101);
}

// from test-api.cc:11473
void
test_StackTrace()
{
  v8::HandleScope scope;
  LocalContext context;
  v8::TryCatch try_catch;
  const char *source = "function foo() { FAIL.FAIL; }; foo();";
  v8::Handle<v8::String> src = v8::String::New(source);
  v8::Handle<v8::String> origin = v8::String::New("stack-trace-test");
  v8::Script::New(src, origin)->Run();
  CHECK(try_catch.HasCaught());
  v8::String::Utf8Value stack(try_catch.StackTrace());
  CHECK(strstr(*stack, "at foo (stack-trace-test") != NULL);
}

// from test-api.cc:11562
void
test_CaptureStackTrace()
{ }

// from test-api.cc:11620
void
test_CaptureStackTraceForUncaughtException()
{ }

// from test-api.cc:11643
void
test_CaptureStackTraceForUncaughtExceptionAndSetters()
{ }

// from test-api.cc:11679
void
test_SourceURLInStackTrace()
{ }

// from test-api.cc:11703
void
test_IdleNotification()
{
  bool rv = false;
  for (int i = 0; i < 100; i++) {
    rv = v8::V8::IdleNotification();
    if (rv)
      break;
  }
  CHECK(rv == true);
}

// from test-api.cc:11736
void
test_SetResourceConstraints()
{ }

// from test-api.cc:11758
void
test_SetResourceConstraintsInThread()
{ }

// from test-api.cc:11788
void
test_GetHeapStatistics()
{ }

// from test-api.cc:11843
void
test_QuietSignalingNaNs()
{ }

// from test-api.cc:11927
void
test_SpaghettiStackReThrow()
{ }

// from test-api.cc:11953
void
test_Regress528()
{ }

// from test-api.cc:12042
void
test_ScriptOrigin()
{ }

// from test-api.cc:12064
void
test_ScriptLineNumber()
{ }

// from test-api.cc:12093
void
test_SetterOnConstructorPrototype()
{
  v8::HandleScope scope;
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"),
                     GetterWhichReturns42,
                     SetterWhichSetsYOnThisTo23);
  LocalContext context;
  context->Global()->Set(v8_str("P"), templ->NewInstance());
  CompileRun("function C1() {"
             "  this.x = 23;"
             "};"
             "C1.prototype = P;"
             "function C2() {"
             "  this.x = 23"
             "};"
             "C2.prototype = { };"
             "C2.prototype.__proto__ = P;");

  v8::Local<v8::Script> script;
  script = v8::Script::Compile(v8_str("new C1();"));
  for (int i = 0; i < 10; i++) {
    v8::Handle<v8::Object> c1 = v8::Handle<v8::Object>::Cast(script->Run());
    CHECK_EQ(42, c1->Get(v8_str("x"))->Int32Value());
    CHECK_EQ(23, c1->Get(v8_str("y"))->Int32Value());
  }

  script = v8::Script::Compile(v8_str("new C2();"));
  for (int i = 0; i < 10; i++) {
    v8::Handle<v8::Object> c2 = v8::Handle<v8::Object>::Cast(script->Run());
    CHECK_EQ(42, c2->Get(v8_str("x"))->Int32Value());
    CHECK_EQ(23, c2->Get(v8_str("y"))->Int32Value());
  }
}

// from test-api.cc:12143
void
test_InterceptorOnConstructorPrototype()
{ }

// from test-api.cc:12177
void
test_Bug618()
{
  const char* source = "function C1() {"
                       "  this.x = 23;"
                       "};"
                       "C1.prototype = P;";

  v8::HandleScope scope;
  LocalContext context;
  v8::Local<v8::Script> script;

  // Use a simple object as prototype.
  v8::Local<v8::Object> prototype = v8::Object::New();
  prototype->Set(v8_str("y"), v8_num(42));
  context->Global()->Set(v8_str("P"), prototype);

  // This compile will add the code to the compilation cache.
  CompileRun(source);

  script = v8::Script::Compile(v8_str("new C1();"));
  // Allow enough iterations for the inobject slack tracking logic
  // to finalize instance size and install the fast construct stub.
  for (int i = 0; i < 256; i++) {
    v8::Handle<v8::Object> c1 = v8::Handle<v8::Object>::Cast(script->Run());
    CHECK_EQ(23, c1->Get(v8_str("x"))->Int32Value());
    CHECK_EQ(42, c1->Get(v8_str("y"))->Int32Value());
  }

  // Use an API object with accessors as prototype.
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetAccessor(v8_str("x"),
                     GetterWhichReturns42,
                     SetterWhichSetsYOnThisTo23);
  context->Global()->Set(v8_str("P"), templ->NewInstance());

  // This compile will get the code from the compilation cache.
  CompileRun(source);

  script = v8::Script::Compile(v8_str("new C1();"));
  for (int i = 0; i < 10; i++) {
    v8::Handle<v8::Object> c1 = v8::Handle<v8::Object>::Cast(script->Run());
    CHECK_EQ(42, c1->Get(v8_str("x"))->Int32Value());
    CHECK_EQ(23, c1->Get(v8_str("y"))->Int32Value());
  }
}

// from test-api.cc:12243
void
test_GCCallbacks()
{ }

// from test-api.cc:12277
void
test_AddToJSFunctionResultCache()
{ }

// from test-api.cc:12304
void
test_FillJSFunctionResultCache()
{ }

// from test-api.cc:12326
void
test_RoundRobinGetFromCache()
{ }

// from test-api.cc:12351
void
test_ReverseGetFromCache()
{ }

// from test-api.cc:12376
void
test_TestEviction()
{ }

// from test-api.cc:12394
void
test_TwoByteStringInAsciiCons()
{ }

// from test-api.cc:12483
void
test_GCInFailedAccessCheckCallback()
{ }

// from test-api.cc:12561
void
test_StringCheckMultipleContexts()
{ }

// from test-api.cc:12584
void
test_NumberCheckMultipleContexts()
{ }

// from test-api.cc:12607
void
test_BooleanCheckMultipleContexts()
{ }

// from test-api.cc:12630
void
test_DontDeleteCellLoadIC()
{ }

// from test-api.cc:12670
void
test_DontDeleteCellLoadICForceDelete()
{ }

// from test-api.cc:12698
void
test_DontDeleteCellLoadICAPI()
{ }

// from test-api.cc:12726
void
test_GlobalLoadICGC()
{ }

// from test-api.cc:12768
void
test_RegExp()
{
  v8::HandleScope scope;
  LocalContext context;

  v8::Handle<v8::RegExp> re = v8::RegExp::New(v8_str("foo"), v8::RegExp::kNone);
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("foo")));
  CHECK_EQ(re->GetFlags(), v8::RegExp::kNone);

  re = v8::RegExp::New(v8_str("bar"),
                       static_cast<v8::RegExp::Flags>(v8::RegExp::kIgnoreCase |
                                                      v8::RegExp::kGlobal));
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("bar")));
  CHECK_EQ(static_cast<int>(re->GetFlags()),
           v8::RegExp::kIgnoreCase | v8::RegExp::kGlobal);

  re = v8::RegExp::New(v8_str("baz"),
                       static_cast<v8::RegExp::Flags>(v8::RegExp::kIgnoreCase |
                                                      v8::RegExp::kMultiline));
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("baz")));
  CHECK_EQ(static_cast<int>(re->GetFlags()),
           v8::RegExp::kIgnoreCase | v8::RegExp::kMultiline);

  re = CompileRun("/quux/").As<v8::RegExp>();
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("quux")));
  CHECK_EQ(re->GetFlags(), v8::RegExp::kNone);

  re = CompileRun("/quux/gm").As<v8::RegExp>();
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("quux")));
  CHECK_EQ(static_cast<int>(re->GetFlags()),
           v8::RegExp::kGlobal | v8::RegExp::kMultiline);

  // Override the RegExp constructor and check the API constructor
  // still works.
  CompileRun("RegExp = function() {}");

  re = v8::RegExp::New(v8_str("foobar"), v8::RegExp::kNone);
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("foobar")));
  CHECK_EQ(re->GetFlags(), v8::RegExp::kNone);

  re = v8::RegExp::New(v8_str("foobarbaz"),
                       static_cast<v8::RegExp::Flags>(v8::RegExp::kIgnoreCase |
                                                      v8::RegExp::kMultiline));
  CHECK(re->IsRegExp());
  CHECK(re->GetSource()->Equals(v8_str("foobarbaz")));
  CHECK_EQ(static_cast<int>(re->GetFlags()),
           v8::RegExp::kIgnoreCase | v8::RegExp::kMultiline);

  context->Global()->Set(v8_str("re"), re);
  ExpectTrue("re.test('FoobarbaZ')");

  v8::TryCatch try_catch;
  re = v8::RegExp::New(v8_str("foo["), v8::RegExp::kNone);
  CHECK(re.IsEmpty());
  CHECK(try_catch.HasCaught());
  context->Global()->Set(v8_str("ex"), try_catch.Exception());
  ExpectTrue("ex instanceof SyntaxError");
}

// from test-api.cc:12833
void
test_Equals()
{
  v8::HandleScope handleScope;
  LocalContext localContext;

  v8::Handle<v8::Object> globalProxy = localContext->Global();
  v8::Handle<Value> global = globalProxy->GetPrototype();

  CHECK(global->StrictEquals(global));
  CHECK(!global->StrictEquals(globalProxy));
  CHECK(!globalProxy->StrictEquals(global));
  CHECK(globalProxy->StrictEquals(globalProxy));

  CHECK(global->Equals(global));
  CHECK(!global->Equals(globalProxy));
  CHECK(!globalProxy->Equals(global));
  CHECK(globalProxy->Equals(globalProxy));
}

// from test-api.cc:12865
void
test_NamedEnumeratorAndForIn()
{ }

// from test-api.cc:12880
void
test_DefinePropertyPostDetach()
{ }


////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_Handles),
  DISABLED_TEST(test_ReceiverSignature, 79),
  DISABLED_TEST(test_ArgumentSignature, 79),
  TEST(test_HulIgennem),
  TEST(test_Access),
  TEST(test_AccessElement),
  TEST(test_Script),
  UNIMPLEMENTED_TEST(test_ScriptUsingStringResource),
  UNIMPLEMENTED_TEST(test_ScriptUsingAsciiStringResource),
  UNIMPLEMENTED_TEST(test_ScriptMakingExternalString),
  UNIMPLEMENTED_TEST(test_ScriptMakingExternalAsciiString),
  UNIMPLEMENTED_TEST(test_MakingExternalStringConditions),
  UNIMPLEMENTED_TEST(test_MakingExternalAsciiStringConditions),
  UNIMPLEMENTED_TEST(test_UsingExternalString),
  UNIMPLEMENTED_TEST(test_UsingExternalAsciiString),
  UNIMPLEMENTED_TEST(test_ScavengeExternalString),
  UNIMPLEMENTED_TEST(test_ScavengeExternalAsciiString),
  UNIMPLEMENTED_TEST(test_ExternalStringWithDisposeHandling),
  UNIMPLEMENTED_TEST(test_StringConcat),
  TEST(test_GlobalProperties),
  TEST(test_FunctionTemplate),
  UNIMPLEMENTED_TEST(test_ExternalWrap),
  UNIMPLEMENTED_TEST(test_FindInstanceInPrototypeChain),
  TEST(test_TinyInteger),
  UNIMPLEMENTED_TEST(test_BigSmiInteger),
  UNIMPLEMENTED_TEST(test_BigInteger),
  TEST(test_TinyUnsignedInteger),
  UNIMPLEMENTED_TEST(test_BigUnsignedSmiInteger),
  UNIMPLEMENTED_TEST(test_BigUnsignedInteger),
  TEST(test_OutOfSignedRangeUnsignedInteger),
  TEST(test_Number),
  TEST(test_ToNumber),
  TEST(test_Date),
  TEST(test_Boolean),
  UNIMPLEMENTED_TEST(test_GlobalPrototype),
  TEST(test_ObjectTemplate),
  DISABLED_TEST(test_DescriptorInheritance, 91),
  DISABLED_TEST(test_NamedPropertyHandlerGetter, 92),
  TEST(test_IndexedPropertyHandlerGetter),
  TEST(test_PropertyHandlerInPrototype),
  UNIMPLEMENTED_TEST(test_PrePropertyHandler),
  TEST(test_UndefinedIsNotEnumerable),
  UNIMPLEMENTED_TEST(test_DeepCrossLanguageRecursion),
  UNIMPLEMENTED_TEST(test_CallbackExceptionRegression),
  TEST(test_FunctionPrototype),
  TEST(test_InternalFields),
  UNIMPLEMENTED_TEST(test_GlobalObjectInternalFields),
  UNIMPLEMENTED_TEST(test_InternalFieldsNativePointers),
  UNIMPLEMENTED_TEST(test_InternalFieldsNativePointersAndExternal),
  UNIMPLEMENTED_TEST(test_IdentityHash),
  DISABLED_TEST(test_HiddenProperties, 31),
  UNIMPLEMENTED_TEST(test_HiddenPropertiesWithInterceptors),
  UNIMPLEMENTED_TEST(test_External),
  TEST(test_GlobalHandle),
  TEST(test_ScriptException),
  UNIMPLEMENTED_TEST(test_MessageHandlerData),
  TEST(test_GetSetProperty),
  TEST(test_PropertyAttributes),
  TEST(test_Array),
  TEST(test_Vector),
  TEST(test_FunctionCall),
  UNIMPLEMENTED_TEST(test_OutOfMemory),
  UNIMPLEMENTED_TEST(test_OutOfMemoryNested),
  UNIMPLEMENTED_TEST(test_HugeConsStringOutOfMemory),
  UNIMPLEMENTED_TEST(test_ConstructCall),
  TEST(test_ConversionNumber),
  TEST(test_isNumberType),
  TEST(test_ConversionException),
  UNIMPLEMENTED_TEST(test_APICatch),
  UNIMPLEMENTED_TEST(test_APIThrowTryCatch),
  UNIMPLEMENTED_TEST(test_TryCatchInTryFinally),
  UNIMPLEMENTED_TEST(test_APIThrowMessageOverwrittenToString),
  UNIMPLEMENTED_TEST(test_APIThrowMessage),
  UNIMPLEMENTED_TEST(test_APIThrowMessageAndVerboseTryCatch),
  UNIMPLEMENTED_TEST(test_ExternalScriptException),
  TEST(test_EvalInTryFinally),
  UNIMPLEMENTED_TEST(test_ExceptionOrder),
  TEST(test_ThrowValues),
  TEST(test_CatchZero),
  TEST(test_CatchExceptionFromWith),
  TEST(test_TryCatchAndFinallyHidingException),
  TEST(test_TryCatchAndFinally),
  TEST(test_Equality),
  TEST_THROWS(test_MultiRun),
  TEST(test_SimplePropertyRead),
  DISABLED_TEST(test_DefinePropertyOnAPIAccessor, 83),
  DISABLED_TEST(test_DefinePropertyOnDefineGetterSetter, 84),
  DISABLED_TEST(test_DefineAPIAccessorOnObject, 66),
  UNIMPLEMENTED_TEST(test_DontDeleteAPIAccessorsCannotBeOverriden),
  TEST(test_ElementAPIAccessor),
  TEST(test_SimplePropertyWrite),
  TEST(test_NamedInterceptorPropertyRead),
  TEST(test_NamedInterceptorDictionaryIC),
  UNIMPLEMENTED_TEST(test_NamedInterceptorDictionaryICMultipleContext),
  UNIMPLEMENTED_TEST(test_NamedInterceptorMapTransitionRead),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorWithIndexedAccessor),
  DISABLED_TEST(test_IndexedInterceptorWithGetOwnPropertyDescriptor, 85),
  TEST(test_IndexedInterceptorWithNoSetter),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorWithAccessorCheck),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorWithAccessorCheckSwitchedOn),
  TEST(test_IndexedInterceptorWithDifferentIndices),
  TEST(test_IndexedInterceptorWithNegativeIndices),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorWithNotSmiLookup),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorGoingMegamorphic),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorReceiverTurningSmi),
  UNIMPLEMENTED_TEST(test_IndexedInterceptorOnProto),
  UNIMPLEMENTED_TEST(test_MultiContexts),
  UNIMPLEMENTED_TEST(test_FunctionPrototypeAcrossContexts),
  UNIMPLEMENTED_TEST(test_Regress892105),
  DISABLED_TEST(test_UndetectableObject, 98),
  DISABLED_TEST(test_ExtensibleOnUndetectable, 98),
  UNIMPLEMENTED_TEST(test_UndetectableString),
  TEST(test_GlobalObjectTemplate),
  UNIMPLEMENTED_TEST(test_SimpleExtensions),
  UNIMPLEMENTED_TEST(test_UseEvalFromExtension),
  UNIMPLEMENTED_TEST(test_UseWithFromExtension),
  UNIMPLEMENTED_TEST(test_AutoExtensions),
  UNIMPLEMENTED_TEST(test_SyntaxErrorExtensions),
  UNIMPLEMENTED_TEST(test_ExceptionExtensions),
  UNIMPLEMENTED_TEST(test_NativeCallInExtensions),
  UNIMPLEMENTED_TEST(test_ExtensionDependency),
  UNIMPLEMENTED_TEST(test_FunctionLookup),
  UNIMPLEMENTED_TEST(test_NativeFunctionConstructCall),
  UNIMPLEMENTED_TEST(test_ErrorReporting),
  UNIMPLEMENTED_TEST(test_RegexpOutOfMemory),
  UNIMPLEMENTED_TEST(test_ErrorWithMissingScriptInfo),
  UNIMPLEMENTED_TEST(test_WeakReference),
  UNIMPLEMENTED_TEST(test_NoWeakRefCallbacksInScavenge),
  TEST(test_Arguments),
  DISABLED_TEST(test_Deleter, 70),
  UNWANTED_TEST(test_Enumerators),
  TEST(test_GetterHolders),
  DISABLED_TEST(test_PreInterceptorHolders, 97),
  TEST(test_ObjectInstantiation),
  DISABLED_TEST(test_StringWrite, 16),
  TEST(test_ToArrayIndex),
  TEST(test_ErrorConstruction),
  TEST(test_DeleteAccessor),
  UNIMPLEMENTED_TEST(test_TypeSwitch),
  UNIMPLEMENTED_TEST(test_ApiUncaughtException),
  UNIMPLEMENTED_TEST(test_ExceptionInNativeScript),
  TEST(test_CompilationErrorUsingTryCatchHandler),
  TEST(test_TryCatchFinallyUsingTryCatchHandler),
  UNIMPLEMENTED_TEST(test_SecurityHandler),
  UNIMPLEMENTED_TEST(test_SecurityChecks),
  UNIMPLEMENTED_TEST(test_SecurityChecksForPrototypeChain),
  UNIMPLEMENTED_TEST(test_CrossDomainDelete),
  UNIMPLEMENTED_TEST(test_CrossDomainIsPropertyEnumerable),
  UNIMPLEMENTED_TEST(test_CrossDomainForIn),
  UNIMPLEMENTED_TEST(test_ContextDetachGlobal),
  UNIMPLEMENTED_TEST(test_DetachAndReattachGlobal),
  UNIMPLEMENTED_TEST(test_AccessControl),
  UNIMPLEMENTED_TEST(test_AccessControlES5),
  UNIMPLEMENTED_TEST(test_AccessControlGetOwnPropertyNames),
  UNIMPLEMENTED_TEST(test_GetOwnPropertyNamesWithInterceptor),
  UNIMPLEMENTED_TEST(test_CrossDomainAccessors),
  UNIMPLEMENTED_TEST(test_AccessControlIC),
  UNIMPLEMENTED_TEST(test_AccessControlFlatten),
  UNIMPLEMENTED_TEST(test_AccessControlInterceptorIC),
  TEST(test_Version),
  TEST(test_InstanceProperties),
  DISABLED_TEST(test_GlobalObjectInstanceProperties, 666),
  DISABLED_TEST(test_CallKnownGlobalReceiver, 666),
  DISABLED_TEST(test_ShadowObject, 666),
  DISABLED_TEST(test_HiddenPrototype, 666),
  DISABLED_TEST(test_SetPrototype, 666),
  UNIMPLEMENTED_TEST(test_SetPrototypeThrows),
  DISABLED_TEST(test_GetterSetterExceptions, 666),
  UNIMPLEMENTED_TEST(test_Constructor),
  DISABLED_TEST(test_FunctionDescriptorException, 666),
  UNIMPLEMENTED_TEST(test_EvalAliasedDynamic),
  UNIMPLEMENTED_TEST(test_CrossEval),
  UNIMPLEMENTED_TEST(test_EvalInDetachedGlobal),
  UNIMPLEMENTED_TEST(test_CrossLazyLoad),
  UNIMPLEMENTED_TEST(test_CallAsFunction),
  UNIMPLEMENTED_TEST(test_HandleIteration),
  UNIMPLEMENTED_TEST(test_InterceptorHasOwnProperty),
  UNIMPLEMENTED_TEST(test_InterceptorHasOwnPropertyCausingGC),
  UNIMPLEMENTED_TEST(test_InterceptorLoadIC),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithFieldOnHolder),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithSubstitutedProto),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithPropertyOnProto),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICUndefined),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithOverride),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICFieldNotNeeded),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICInvalidatedField),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICPostInterceptor),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICInvalidatedFieldViaGlobal),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithCallbackOnHolder),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICWithCallbackOnProto),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICForCallbackWithOverride),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICCallbackNotNeeded),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICInvalidatedCallback),
  UNIMPLEMENTED_TEST(test_InterceptorLoadICInvalidatedCallbackViaGlobal),
  UNIMPLEMENTED_TEST(test_InterceptorReturningZero),
  UNIMPLEMENTED_TEST(test_InterceptorStoreIC),
  UNIMPLEMENTED_TEST(test_InterceptorStoreICWithNoSetter),
  UNIMPLEMENTED_TEST(test_InterceptorCallIC),
  UNIMPLEMENTED_TEST(test_InterceptorCallICSeesOthers),
  UNIMPLEMENTED_TEST(test_InterceptorCallICCacheableNotNeeded),
  UNIMPLEMENTED_TEST(test_InterceptorCallICInvalidatedCacheable),
  UNIMPLEMENTED_TEST(test_InterceptorCallICConstantFunctionUsed),
  UNIMPLEMENTED_TEST(test_InterceptorCallICConstantFunctionNotNeeded),
  UNIMPLEMENTED_TEST(test_InterceptorCallICInvalidatedConstantFunction),
  UNIMPLEMENTED_TEST(test_InterceptorCallICInvalidatedConstantFunctionViaGlobal),
  UNIMPLEMENTED_TEST(test_InterceptorCallICCachedFromGlobal),
  UNIMPLEMENTED_TEST(test_CallICFastApi_DirectCall_GCMoveStub),
  UNIMPLEMENTED_TEST(test_CallICFastApi_DirectCall_Throw),
  UNIMPLEMENTED_TEST(test_LoadICFastApi_DirectCall_GCMoveStub),
  UNIMPLEMENTED_TEST(test_LoadICFastApi_DirectCall_Throw),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_TrivialSignature),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_SimpleSignature),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_SimpleSignature_Miss1),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_SimpleSignature_Miss2),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_SimpleSignature_Miss3),
  UNIMPLEMENTED_TEST(test_InterceptorCallICFastApi_SimpleSignature_TypeError),
  UNIMPLEMENTED_TEST(test_CallICFastApi_TrivialSignature),
  UNIMPLEMENTED_TEST(test_CallICFastApi_SimpleSignature),
  UNIMPLEMENTED_TEST(test_CallICFastApi_SimpleSignature_Miss1),
  UNIMPLEMENTED_TEST(test_CallICFastApi_SimpleSignature_Miss2),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICKeyChange1),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICKeyChange2),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICKeyChangeOnGlobal),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICFromGlobal),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICMapChangeBefore),
  UNIMPLEMENTED_TEST(test_InterceptorKeyedCallICMapChangeAfter),
  UNIMPLEMENTED_TEST(test_InterceptorICReferenceErrors),
  UNIMPLEMENTED_TEST(test_InterceptorICGetterExceptions),
  UNIMPLEMENTED_TEST(test_InterceptorICSetterExceptions),
  UNIMPLEMENTED_TEST(test_NullNamedInterceptor),
  UNIMPLEMENTED_TEST(test_NullIndexedInterceptor),
  UNIMPLEMENTED_TEST(test_NamedPropertyHandlerGetterAttributes),
  UNIMPLEMENTED_TEST(test_Overriding),
  UNIMPLEMENTED_TEST(test_IsConstructCall),
  DISABLED_TEST(test_ObjectProtoToString, 52),
  DISABLED_TEST(test_ObjectGetConstructorName, 87), //XXXzpao test has local mods
  UNIMPLEMENTED_TEST(test_Threading),
  UNIMPLEMENTED_TEST(test_Threading2),
  UNIMPLEMENTED_TEST(test_NestedLockers),
  UNIMPLEMENTED_TEST(test_NestedLockersNoTryCatch),
  UNIMPLEMENTED_TEST(test_RecursiveLocking),
  UNIMPLEMENTED_TEST(test_LockUnlockLock),
  UNIMPLEMENTED_TEST(test_DontLeakGlobalObjects),
  UNWANTED_TEST(test_NewPersistentHandleFromWeakCallback),
  UNWANTED_TEST(test_DoNotUseDeletedNodesInSecondLevelGc),
  UNWANTED_TEST(test_NoGlobalHandlesOrphaningDueToWeakCallback),
  TEST(test_CheckForCrossContextObjectLiterals),
  UNIMPLEMENTED_TEST(test_NestedHandleScopeAndContexts),
  UNIMPLEMENTED_TEST(test_ExternalAllocatedMemory),
  UNIMPLEMENTED_TEST(test_DisposeEnteredContext),
  UNIMPLEMENTED_TEST(test_Regress54),
  UNIMPLEMENTED_TEST(test_CatchStackOverflow),
  UNIMPLEMENTED_TEST(test_TryCatchSourceInfo),
  TEST(test_CompilationCache),
  DISABLED_TEST(test_CallbackFunctionName, 86),
  DISABLED_TEST(test_DateAccess, 20),
  DISABLED_TEST(test_PropertyEnumeration, 22),
  UNIMPLEMENTED_TEST(test_DisableAccessChecksWhileConfiguring),
  UNIMPLEMENTED_TEST(test_AccessChecksReenabledCorrectly),
  UNIMPLEMENTED_TEST(test_AccessControlRepeatedContextCreation),
  UNIMPLEMENTED_TEST(test_TurnOnAccessCheck),
  UNIMPLEMENTED_TEST(test_TurnOnAccessCheckAndRecompile),
  TEST(test_PreCompile),
  TEST(test_PreCompileWithError),
  TEST(test_Regress31661),
  DISABLED_TEST(test_PreCompileSerialization, 60),
  TEST(test_PreCompileDeserializationError),
  UNIMPLEMENTED_TEST(test_PreCompileInvalidPreparseDataError),
  UNIMPLEMENTED_TEST(test_PreCompileAPIVariationsAreSame),
  UNIMPLEMENTED_TEST(test_DictionaryICLoadedFunction),
  UNIMPLEMENTED_TEST(test_CrossContextNew),
  UNIMPLEMENTED_TEST(test_RegExpInterruption),
  UNIMPLEMENTED_TEST(test_ApplyInterruption),
  TEST(test_ObjectClone),
  UNIMPLEMENTED_TEST(test_MorphCompositeStringTest),
  UNIMPLEMENTED_TEST(test_CompileExternalTwoByteSource),
  UNIMPLEMENTED_TEST(test_RegExpStringModification),
  UNIMPLEMENTED_TEST(test_ReadOnlyPropertyInGlobalProto),
  UNIMPLEMENTED_TEST(test_ForceSet),
  UNIMPLEMENTED_TEST(test_ForceSetWithInterceptor),
  UNIMPLEMENTED_TEST(test_ForceDelete),
  UNIMPLEMENTED_TEST(test_ForceDeleteWithInterceptor),
  UNIMPLEMENTED_TEST(test_ForceDeleteIC),
  UNIMPLEMENTED_TEST(test_GetCallingContext),
  UNIMPLEMENTED_TEST(test_InitGlobalVarInProtoChain),
  UNIMPLEMENTED_TEST(test_ReplaceConstantFunction),
  UNIMPLEMENTED_TEST(test_Regress16276),
  UNIMPLEMENTED_TEST(test_PixelArray),
  UNIMPLEMENTED_TEST(test_PixelArrayInfo),
  UNIMPLEMENTED_TEST(test_PixelArrayWithInterceptor),
  UNIMPLEMENTED_TEST(test_ExternalByteArray),
  UNIMPLEMENTED_TEST(test_ExternalUnsignedByteArray),
  UNIMPLEMENTED_TEST(test_ExternalShortArray),
  UNIMPLEMENTED_TEST(test_ExternalUnsignedShortArray),
  UNIMPLEMENTED_TEST(test_ExternalIntArray),
  UNIMPLEMENTED_TEST(test_ExternalUnsignedIntArray),
  UNIMPLEMENTED_TEST(test_ExternalFloatArray),
  UNIMPLEMENTED_TEST(test_ExternalArrays),
  UNIMPLEMENTED_TEST(test_ExternalArrayInfo),
  TEST(test_ScriptContextDependence),
  DISABLED_TEST(test_StackTrace, 99),
  UNIMPLEMENTED_TEST(test_CaptureStackTrace),
  UNIMPLEMENTED_TEST(test_CaptureStackTraceForUncaughtException),
  UNIMPLEMENTED_TEST(test_CaptureStackTraceForUncaughtExceptionAndSetters),
  UNIMPLEMENTED_TEST(test_SourceURLInStackTrace),
  DISABLED_TEST(test_IdleNotification, 24),
  UNIMPLEMENTED_TEST(test_SetResourceConstraints),
  UNIMPLEMENTED_TEST(test_SetResourceConstraintsInThread),
  UNIMPLEMENTED_TEST(test_GetHeapStatistics),
  UNIMPLEMENTED_TEST(test_QuietSignalingNaNs),
  UNIMPLEMENTED_TEST(test_SpaghettiStackReThrow),
  UNIMPLEMENTED_TEST(test_Regress528),
  UNIMPLEMENTED_TEST(test_ScriptOrigin),
  UNIMPLEMENTED_TEST(test_ScriptLineNumber),
  TEST(test_SetterOnConstructorPrototype),
  UNIMPLEMENTED_TEST(test_InterceptorOnConstructorPrototype),
  TEST(test_Bug618),
  UNIMPLEMENTED_TEST(test_GCCallbacks),
  UNIMPLEMENTED_TEST(test_AddToJSFunctionResultCache),
  UNIMPLEMENTED_TEST(test_FillJSFunctionResultCache),
  UNIMPLEMENTED_TEST(test_RoundRobinGetFromCache),
  UNIMPLEMENTED_TEST(test_ReverseGetFromCache),
  UNIMPLEMENTED_TEST(test_TestEviction),
  UNIMPLEMENTED_TEST(test_TwoByteStringInAsciiCons),
  UNIMPLEMENTED_TEST(test_GCInFailedAccessCheckCallback),
  UNIMPLEMENTED_TEST(test_StringCheckMultipleContexts),
  UNIMPLEMENTED_TEST(test_NumberCheckMultipleContexts),
  UNIMPLEMENTED_TEST(test_BooleanCheckMultipleContexts),
  UNIMPLEMENTED_TEST(test_DontDeleteCellLoadIC),
  UNIMPLEMENTED_TEST(test_DontDeleteCellLoadICForceDelete),
  UNIMPLEMENTED_TEST(test_DontDeleteCellLoadICAPI),
  UNIMPLEMENTED_TEST(test_GlobalLoadICGC),
  DISABLED_TEST(test_RegExp, 666),
  TEST(test_Equals),
  UNIMPLEMENTED_TEST(test_NamedEnumeratorAndForIn),
  UNIMPLEMENTED_TEST(test_DefinePropertyPostDetach),
};

const char* file = __FILE__;
#define TEST_NAME "V8 API"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
