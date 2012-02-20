#include "v8-internal.h"
#include "jsprf.h"

namespace v8 {
namespace internal {

static
void
printTraceName(JSTracer* trc,
               char* buf,
               size_t bufsize)
{
  JS_snprintf(buf, bufsize, "TracedObject(%p)", trc->debugPrintArg);
}

void
traceValue(JSTracer* tracer,
           jsval val)
{
  JS_SET_TRACING_DETAILS(tracer, printTraceName, NULL, 0);
  if (JSVAL_IS_TRACEABLE(val)) {
    JSGCTraceKind kind = JSVAL_TRACE_KIND(val);
    JS_CallTracer(tracer, JSVAL_TO_TRACEABLE(val), kind);
  }
}

void* malloc_(size_t nbytes) { return JS_malloc(cx(), nbytes); }
void free_(void* p) { return JS_free(cx(), p); }

////////////////////////////////////////////////////////////////////////////////
//// Accessor Storage

AccessorStorage::AccessorStorage()
{
  mStore.init(10);
}

void
AccessorStorage::addAccessor(jsid name,
                             AccessorGetter getter,
                             AccessorSetter setter,
                             Handle<Value> data,
                             PropertyAttribute attribute)
{
  JS_ASSERT(mStore.initialized());

  PropertyData container = {
    getter,
    setter,
    data,
    attribute,
  };
  AccessorTable::AddPtr slot = mStore.lookupForAdd(name);
  if (slot.found()) {
    JS_ASSERT(slot->value.data);
    mStore.remove(slot);
    slot = mStore.lookupForAdd(name);
  }
  mStore.add(slot, name, container);
}

AccessorStorage::PropertyData
AccessorStorage::get(jsid name) const
{
  JS_ASSERT(mStore.initialized());

  AccessorTable::Ptr p = mStore.lookup(name);
  JS_ASSERT(p);
  return p->value;
}

AccessorStorage::Range
AccessorStorage::all() const
{
  JS_ASSERT(mStore.initialized());

  return mStore.all();
}

void
AccessorStorage::trace(JSTracer* tracer)
{
  Range r = mStore.all();
  while (!r.empty()) {
    r.front().value.data.trace(tracer);
    r.popFront();
  }
}

////////////////////////////////////////////////////////////////////////////////
//// Attribute Storage

AttributeStorage::AttributeStorage()
{
  mStore.init(10);
}

void
AttributeStorage::addAttribute(jsid name,
                               Handle<Value> data)
{
  JS_ASSERT(mStore.initialized());

  AttributeTable::AddPtr slot = mStore.lookupForAdd(name);
  if (slot.found()) {
    JS_ASSERT(slot->value);
    mStore.remove(slot);
    slot = mStore.lookupForAdd(name);
  }
  mStore.add(slot, name, data);
}

AttributeStorage::Range
AttributeStorage::all() const
{
  JS_ASSERT(mStore.initialized());

  return mStore.all();
}

void
AttributeStorage::trace(JSTracer* tracer)
{
  Range r = mStore.all();
  while (!r.empty()) {
    r.front().value.trace(tracer);
    r.popFront();
  }
}

////////////////////////////////////////////////////////////////////////////////
//// Debug Helpers

#ifdef DEBUG
void
Dump(Handle<Object> obj)
{
  js_DumpObject(JSVAL_TO_OBJECT(obj->native()));
}

void
Dump(Object& obj)
{
  Local<Object> o = Local<Object>::New(&obj);
  Dump(o);
}

void
Dump(Handle<String> str)
{
  jsval v = str->native();
  js_DumpString(JSVAL_TO_STRING(v));
}

void
Dump(String& str)
{
  Local<String> s = Local<String>::New(&str);
  Dump(s);
}
#endif // DEBUG

} // namespace internal
} // namespace v8
