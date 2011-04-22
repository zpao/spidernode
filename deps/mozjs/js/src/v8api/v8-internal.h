#ifndef v8_v8_internal_h__
#define v8_v8_internal_h__
#include "v8.h"
#include "jshashtable.h"

namespace v8 {
namespace internal {

// Core
JSRuntime *rt();
void shutdown();
void reportError(JSContext *ctx, const char *message, JSErrorReport *report);
extern JSClass global_class;

JSContext *cx();

////////////////////////////////////////////////////////////////////////////////
//// Accessor Storage

class AccessorStorage
{
public:
  struct PropertyData
  {
    AccessorGetter getter;
    AccessorSetter setter;
    Persistent<Value> data;
    PropertyAttribute attribute;
  };

  AccessorStorage();
  void addAccessor(jsid name, AccessorGetter getter,
                   AccessorSetter setter, Handle<Value> data,
                   PropertyAttribute attribute);
  PropertyData get(jsid name);
private:
  typedef js::HashMap<jsid, PropertyData, JSIDHashPolicy, js::SystemAllocPolicy> AccessorTable;

  AccessorTable mStore;
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
