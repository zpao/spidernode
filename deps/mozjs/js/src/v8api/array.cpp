#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(Array) == sizeof(GCReference));

uint32_t
Array::Length() const
{
  jsuint length;
  if (JS_GetArrayLength(cx(), *this, &length)) {
    return length;
  }

  return 0;
}

Local<Object>
Array::CloneElementAt(uint32_t index)
{
  Local<Value> toBeCloned = Get(index);
  if (!toBeCloned->IsObject()) {
    return Local<Object>();
  }
  return toBeCloned->ToObject()->Clone();
}

// static
Local<Array>
Array::New(int length)
{
  JSObject *obj = JS_NewArrayObject(cx(), length, NULL);
  Array a(obj);
  return Local<Array>::New(&a);
}

} // namespace v8
