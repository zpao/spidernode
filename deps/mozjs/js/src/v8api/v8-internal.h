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
bool disposed();

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

private:
  AccessorTable mStore;
};

class AttributeStorage
{
  typedef js::HashMap<jsid, Persistent<Value>, JSIDHashPolicy, js::SystemAllocPolicy> AttributeTable;

public:
  AttributeStorage();
  void addAttribute(jsid name, Handle<Value> value);

  typedef AttributeTable::Entry Entry;
  typedef AttributeTable::Range Range;
  Range all() const;

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
