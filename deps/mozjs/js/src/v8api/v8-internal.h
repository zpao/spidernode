#ifndef v8_v8_internal_h__
#define v8_v8_internal_h__
#include "v8.h"
#include "jsapi.h"
#include "jsfriendapi.h"

namespace v8 {
namespace internal {

// Core
JSRuntime *rt();
void shutdown();
void reportError(JSContext *ctx, const char *message, JSErrorReport *report);
extern JSClass global_class;

JSContext *cx();
bool disposed();

class ApiExceptionBoundary {
public:
  ApiExceptionBoundary();
  ~ApiExceptionBoundary();

  bool noExceptionOccured();
};

void TraceObjectInternals(JSTracer* tracer, void*);
void DestroyObjectInternals();

////////////////////////////////////////////////////////////////////////////////
//// Tracing and memory management helpers

void traceValue(JSTracer* tracer, jsval val);
void* malloc_(size_t nbytes);
void free_(void* p);

JS_DECLARE_NEW_METHODS(malloc_, extern JS_ALWAYS_INLINE)
JS_DECLARE_DELETE_METHODS(free_, extern JS_ALWAYS_INLINE)

template <typename T>
class Traced {
public:
  Traced() : mPtr(NULL) {}
  Traced(Handle<T> h) :
    mPtr(NULL) {
    *this = h;
  }
  Traced(const Traced<T> &t) :
    mPtr(t.mPtr ? new_<T>(*t.mPtr) : NULL)
  {
  }
  ~Traced() {
    delete_(mPtr);
  }

  operator bool() const {
    return mPtr != NULL;
  }

  T* operator->() {
    return mPtr;
  }

  Traced<T> &operator=(const Traced<T> &other) {
    if (!other.mPtr) {
      delete_(mPtr);
      mPtr = NULL;
    } else if (mPtr) {
      *mPtr = *other.mPtr;
    } else {
      mPtr = new_<T>(*other.mPtr);
    }
    return *this;
  }
  void operator=(Handle<T> h) {
    delete_(mPtr);
    if (h.IsEmpty()) {
      mPtr = NULL;
    } else {
      mPtr = new_<T>(**h);
    }
  }
  Local<T> get() {
    return Local<T>::New(mPtr);
  }

  void trace(JSTracer* tracer) {
    if (mPtr)
      traceValue(tracer, mPtr->native());
  }
private:
  T* mPtr;
};

bool IsFunctionTemplate(Handle<Value> v);
bool IsObjectTemplate(Handle<Value> v);

////////////////////////////////////////////////////////////////////////////////
//// Accessor Storage

class AccessorStorage
{
public:
  struct PropertyData
  {
    AccessorGetter getter;
    AccessorSetter setter;
    Traced<Value> data;
    PropertyAttribute attribute;
  };
private:
  typedef js::HashMap<jsid, PropertyData, JSIDHashPolicy, js::SystemAllocPolicy> AccessorTable;

public:
  AccessorStorage();
  void addAccessor(jsid name, AccessorGetter getter,
                   AccessorSetter setter, Handle<Value> data,
                   PropertyAttribute attribute);
  PropertyData get(jsid name) const;

  typedef AccessorTable::Entry Entry;
  typedef AccessorTable::Range Range;
  Range all() const;

  void trace(JSTracer* tracer);
private:
  AccessorTable mStore;
};

class AttributeStorage
{
  typedef js::HashMap<jsid, Traced<Value>, JSIDHashPolicy, js::SystemAllocPolicy> AttributeTable;

public:
  AttributeStorage();
  void addAttribute(jsid name, Handle<Value> value);

  typedef AttributeTable::Entry Entry;
  typedef AttributeTable::Range Range;
  Range all() const;

  void trace(JSTracer* tracer);
private:
  AttributeTable mStore;
};

////////////////////////////////////////////////////////////////////////////////
//// Debug Helpers

#ifdef DEBUG
void Dump(Handle<Object> obj);
void Dump(Object& obj);
void Dump(Handle<String> str);
void Dump(String& str);
#endif // DEBUG

} // namespace internal
} // namespace v8

#endif // v8_v8_internal_h__
