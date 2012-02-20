#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(Function) == sizeof(GCReference));

Function::operator JSFunction*() const {
  return JS_ValueToFunction(cx(), mVal);
}

Local<Object> Function::NewInstance() const {
  Object o(JS_New(cx(), JSVAL_TO_OBJECT(mVal), 0, NULL));
  if (!o) {
    TryCatch::CheckForException();
    return Local<Object>();
  } else {
    return Local<Object>::New(&o);
  }
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const {
  jsval *values = array_new<jsval>(argc);
  for (int i = 0; i < argc; i++)
    values[i] = argv[i]->native();
  Object o(JS_New(cx(), JSVAL_TO_OBJECT(mVal), argc, values));
  array_delete(values);
  if (!o) {
    TryCatch::CheckForException();
    return Local<Object>();
  } else {
    return Local<Object>::New(&o);
  }
}

Local<Value> Function::Call(Handle<Object> recv, int argc, Handle<Value> argv[]) const {
  jsval *values = array_new<jsval>(argc);
  for (int i = 0; i < argc; i++)
    values[i] = argv[i]->native();
  jsval rval;
  if (!JS_CallFunctionValue(cx(), **recv, mVal, argc, values, &rval)) {
    TryCatch::CheckForException();
    array_delete(values);
    return Local<Value>();
  }
  Value v(rval);
  array_delete(values);
  return Local<Value>::New(&v);
}
void Function::SetName(Handle<String> name) {
  UNIMPLEMENTEDAPI();
}

Handle<String> Function::GetName() const {
  // JS_GetFunctionId will return null if the function is anonymous, so we need
  // to guard against that. We'll return an empty string (not handle) in that case.
  // Unfortunately SpiderMonkey treats more functions as anonymous than V8 does.
  // For example: outer.inner in the below code is anonymous in SpiderMonkey, but not V8
  // var outer = { inner: function() { } };
  JSString *jss = JS_GetFunctionId(*this);
  if (jss) {
    String s(jss);
    return Local<String>::New(&s);
  }

  return String::New("");
}

int Function::GetScriptLineNumber() const {
  UNIMPLEMENTEDAPI(kLineOffsetNotFound);
}

const int Function::kLineOffsetNotFound = -1;
}
