#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(Function) == sizeof(GCReference));

Function::operator JSFunction*() const {
  return JS_ValueToFunction(cx(), mVal);
}

Local<Object> Function::NewInstance() const {
  Object o(JS_New(cx(), JSVAL_TO_OBJECT(mVal), 0, NULL));
  return Local<Object>::New(&o);
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const {
  jsval *values = new jsval[argc];
  for (int i = 0; i < argc; i++)
    values[i] = argv[i]->native();
  Object o(JS_New(cx(), JSVAL_TO_OBJECT(mVal), argc, values));
  delete[] values;
  return Local<Object>::New(&o);
}

Local<Value> Function::Call(Handle<Object> recv, int argc, Handle<Value> argv[]) const {
  jsval *values = new jsval[argc];
  for (int i = 0; i < argc; i++)
    values[i] = argv[i]->native();
  jsval rval;
  if (!JS_CallFunctionValue(cx(), **recv, mVal, argc, values, &rval)) {
    return Local<Value>();
  }
  Value v(rval);
  delete[] values;
  return Local<Value>::New(&v);
}
void Function::SetName(Handle<String> name) {
  UNIMPLEMENTEDAPI();
}

Handle<String> Function::GetName() const {
  String s(JS_GetFunctionId(*this));
  return Local<String>::New(&s);
}

int Function::GetScriptLineNumber() const {
  UNIMPLEMENTEDAPI(kLineOffsetNotFound);
}

const int Function::kLineOffsetNotFound = -1;
}
