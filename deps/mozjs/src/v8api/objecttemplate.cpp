#include "v8-internal.h"

namespace v8 {
using namespace internal;

namespace {

struct PrivateData
{
  PrivateData() :
    namedGetter(NULL),
    namedSetter(NULL),
    namedQuery(NULL),
    namedDeleter(NULL),
    namedEnumerator(NULL),
    indexedGetter(NULL),
    indexedSetter(NULL),
    indexedQuery(NULL),
    indexedDeleter(NULL),
    indexedEnumerator(NULL)
  {
  }

  static PrivateData* Get(JSContext* cx,
                          JSObject* obj)
  {
    return static_cast<PrivateData*>(JS_GetPrivate(cx, obj));
  }
  static PrivateData* Get(JSObject* obj)
  {
    return static_cast<PrivateData*>(JS_GetPrivate(cx(), obj));
  }
  static PrivateData* Get(Handle<ObjectTemplate> ot)
  {
    Object* obj = reinterpret_cast<Object*>(*ot);
    return PrivateData::Get(*obj);
  }

  // Named Property Handler storage.
  NamedPropertyGetter namedGetter;
  NamedPropertySetter namedSetter;
  NamedPropertyQuery namedQuery;
  NamedPropertyDeleter namedDeleter;
  NamedPropertyEnumerator namedEnumerator;
  Persistent<Value> namedData;

  // Indexed Property Handler storage.
  IndexedPropertyGetter indexedGetter;
  IndexedPropertySetter indexedSetter;
  IndexedPropertyQuery indexedQuery;
  IndexedPropertyDeleter indexedDeleter;
  IndexedPropertyEnumerator indexedEnumerator;
  Persistent<Value> indexedData;
};

struct ObjectTemplateHandle
{
  ObjectTemplateHandle(ObjectTemplate* ot) :
    objectTemplate(ot)
  {
  }
  static ObjectTemplateHandle* Get(JSContext* cx,
                                   JSObject* obj)
  {
    return static_cast<ObjectTemplateHandle*>(JS_GetPrivate(cx, obj));
  }

  static Local<ObjectTemplate> GetHandle(JSContext* cx,
                                         JSObject* obj)
  {
    ObjectTemplateHandle* h =
      static_cast<ObjectTemplateHandle*>(JS_GetPrivate(cx, obj));
    return Local<ObjectTemplate>::New(h->objectTemplate);
  }

  Persistent<ObjectTemplate> objectTemplate;
};

JSBool
o_DeleteProperty(JSContext* cx,
                 JSObject* obj,
                 jsid id,
                 jsval* vp)
{
  Local<ObjectTemplate> ot = ObjectTemplateHandle::GetHandle(cx, obj);
  PrivateData* pd = PrivateData::Get(ot);
  if (JSID_IS_INT(id) && pd->indexedDeleter) {
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->indexedData, obj);
    Handle<Boolean> ret = pd->indexedDeleter(JSID_TO_INT(id), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }
  else if (JSID_IS_STRING(id) && pd->namedDeleter) {
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->namedData, obj);
    Handle<Boolean> ret = pd->namedDeleter(String::FromJSID(id), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }

  return JS_PropertyStub(cx, obj, id, vp);
}

JSBool
o_GetProperty(JSContext* cx,
              JSObject* obj,
              jsid id,
              jsval* vp)
{
  Local<ObjectTemplate> ot = ObjectTemplateHandle::GetHandle(cx, obj);
  PrivateData* pd = PrivateData::Get(ot);
  if (JSID_IS_INT(id) && pd->indexedGetter) {
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->indexedData, obj);
    Handle<Value> ret = pd->indexedGetter(JSID_TO_INT(id), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }
  else if (JSID_IS_STRING(id) && pd->namedGetter) {
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->namedData, obj);
    Handle<Value> ret = pd->namedGetter(String::FromJSID(id), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }

  return JS_PropertyStub(cx, obj, id, vp);
}

JSBool
o_SetProperty(JSContext* cx,
              JSObject* obj,
              jsid id,
              JSBool strict,
              jsval* vp)
{
  Local<ObjectTemplate> ot = ObjectTemplateHandle::GetHandle(cx, obj);
  PrivateData* pd = PrivateData::Get(ot);
  if (JSID_IS_INT(id) && pd->indexedSetter) {
    Value val(*vp);
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->indexedData, obj);
    JSUint32 idx = JSID_TO_INT(id);
    Handle<Value> ret = pd->indexedSetter(idx, Local<Value>(&val), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }
  else if (JSID_IS_STRING(id) && pd->namedSetter) {
    Value val(*vp);
    const AccessorInfo info =
      AccessorInfo::MakeAccessorInfo(pd->namedData, obj);
    Handle<Value> ret =
      pd->namedSetter(String::FromJSID(id), Local<Value>(&val), info);
    if (!ret.IsEmpty()) {
      *vp = ret->native();
      return JS_TRUE;
    }
  }

  return JS_StrictPropertyStub(cx, obj, id, strict, vp);
}

void
o_finalize(JSContext* cx,
           JSObject* obj)
{
  ObjectTemplateHandle* data = ObjectTemplateHandle::Get(cx, obj);
  delete data;
}

JSClass gNewInstanceClass = {
  NULL, // name
  JSCLASS_HAS_PRIVATE, // flags
  JS_PropertyStub, // addProperty
  o_DeleteProperty, // delProperty
  o_GetProperty, // getProperty
  o_SetProperty, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  o_finalize, // finalize
  NULL, // getObjectOps
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // mark
  NULL, // reservedSlots
};

void
ot_finalize(JSContext* cx,
            JSObject* obj)
{
  PrivateData* data = PrivateData::Get(cx, obj);
  delete data;
}

JSClass gObjectTemplateClass = {
  "ObjectTemplate", // name
  JSCLASS_HAS_PRIVATE, // flags
  JS_PropertyStub, // addProperty
  JS_PropertyStub, // delProperty
  JS_PropertyStub, // getProperty
  JS_StrictPropertyStub, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  ot_finalize, // finalize
  NULL, // getObjectOps
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // mark
  NULL, // reservedSlots
};

} // anonymous namespace

ObjectTemplate::ObjectTemplate() :
  Template(&gObjectTemplateClass)
{
  PrivateData* data = new PrivateData();
  (void)JS_SetPrivate(cx(), InternalObject(), data);
}

// static
Local<ObjectTemplate>
ObjectTemplate::New()
{
  UNIMPLEMENTEDAPI(NULL);
}

Local<Object>
ObjectTemplate::NewInstance()
{
  // We've set everything we care about on our SecretObject, so we can assign
  // that to the prototype of our new object.
  JSObject* obj = JS_NewObject(cx(), &gNewInstanceClass, InternalObject(), NULL);
  if (!obj) {
    return NULL;
  }

  ObjectTemplateHandle* handle = new ObjectTemplateHandle(this);
  if (!JS_SetPrivate(cx(), obj, handle)) {
    delete handle;
    // TODO handle error better
    return NULL;
  }

  Object o(obj);
  return Local<Object>::New(&o);
}

void
ObjectTemplate::SetAccessor(Handle<String> name,
                            AccessorGetter getter,
                            AccessorSetter setter,
                            Handle<Value> data,
                            AccessControl settings,
                            PropertyAttribute attribute)
{
  (void)InternalObject().SetAccessor(name, getter, setter, data, settings,
                                     attribute);
}

void
ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetter getter,
                                        NamedPropertySetter setter,
                                        NamedPropertyQuery query,
                                        NamedPropertyDeleter deleter,
                                        NamedPropertyEnumerator enumerator,
                                        Handle<Value> data)
{
  PrivateData* pd = PrivateData::Get(InternalObject());
  pd->namedGetter = getter;
  pd->namedSetter = setter;
  pd->namedQuery = query;
  pd->namedDeleter = deleter;
  pd->namedEnumerator = enumerator;
  pd->namedData = Persistent<Value>::New(data);
}

void
ObjectTemplate::SetIndexedPropertyHandler(IndexedPropertyGetter getter,
                                          IndexedPropertySetter setter,
                                          IndexedPropertyQuery query,
                                          IndexedPropertyDeleter deleter,
                                          IndexedPropertyEnumerator enumerator,
                                          Handle<Value> data)
{
  PrivateData* pd = PrivateData::Get(InternalObject());
  pd->indexedGetter = getter;
  pd->indexedSetter = setter;
  pd->indexedQuery = query;
  pd->indexedDeleter = deleter;
  pd->indexedEnumerator = enumerator;
  pd->indexedData = Persistent<Value>::New(data);
}

void
ObjectTemplate::SetCallAsFunctionHandler(InvocationCallback callback,
                                         Handle<Value> data)
{
  UNIMPLEMENTEDAPI();
}

void
ObjectTemplate::MarkAsUndetectable()
{
  UNIMPLEMENTEDAPI();
}

void
ObjectTemplate::SetAccessCheckCallbacks(NamedSecurityCallback named_handler,
                                        IndexedSecurityCallback indexed_handler,
                                        Handle<Value> data,
                                        bool turned_on_by_default)
{
  UNIMPLEMENTEDAPI();
}

int
ObjectTemplate::InternalFieldCount()
{
  UNIMPLEMENTEDAPI(0);
}

void
ObjectTemplate::SetInternalFieldCount(int value)
{
  UNIMPLEMENTEDAPI();
}

} // namespace v8
