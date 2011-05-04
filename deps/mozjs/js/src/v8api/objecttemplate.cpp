#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(ObjectTemplate) == sizeof(GCReference));

namespace {

extern JSClass gNewInstanceClass;
JSBool
o_DeleteProperty(JSContext* cx,
                 JSObject* obj,
                 jsid id,
                 jsval* vp);

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
    indexedEnumerator(NULL),
    cls(gNewInstanceClass)
  {
  }
  ~PrivateData() {
    namedData.Dispose();
    indexedData.Dispose();
    prototype.Dispose();
  }

  static PrivateData* Get(JSContext* cx,
                          JSObject* obj)
  {
    return static_cast<PrivateData*>(JS_GetPrivate(cx, obj));
  }
  static PrivateData* Get(JSObject* obj)
  {
    return Get(cx(), obj);
  }
  static PrivateData* Get(Handle<ObjectTemplate> ot)
  {
    Object* obj = reinterpret_cast<Object*>(*ot);
    JS_ASSERT(obj);
    return PrivateData::Get(*obj);
  }

  AccessorStorage accessors;
  AttributeStorage attributes;

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

  Persistent<ObjectTemplate> prototype;

  JSClass cls;
};

struct ObjectTemplateHandle
{
  ObjectTemplateHandle(Handle<ObjectTemplate> ot) :
    objectTemplate(Persistent<ObjectTemplate>::New(ot))
  {
    JS_ASSERT(!ot.IsEmpty());
  }

  ~ObjectTemplateHandle()
  {
    JS_ASSERT(!objectTemplate.IsEmpty());
    objectTemplate.Dispose();
  }

  static ObjectTemplateHandle* Get(JSContext* cx,
                                   JSObject* obj)
  {
    ObjectTemplateHandle* data = NULL;
    // XXX: this doesn't work correctly if there are multiple
    // instantiated objects in the proto chain
    JSObject* o = obj;
    while (o != NULL) {
      JSClass* cls = JS_GET_CLASS(cx, o);
      if (cls->delProperty == o_DeleteProperty) {
        data = static_cast<ObjectTemplateHandle*>(JS_GetPrivate(cx, o));
        break;
      }
      o = JS_GetPrototype(cx, o);
    }
    JS_ASSERT(data);
    return data;
  }

  static Local<ObjectTemplate> GetHandle(JSContext* cx,
                                         JSObject* obj)
  {
    ObjectTemplateHandle* h = ObjectTemplateHandle::Get(cx, obj);
    JS_ASSERT(h && !h->objectTemplate.IsEmpty());
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
  JS_ASSERT(pd);
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
  JS_ASSERT(pd);
  if (JSID_IS_INT(id) && JSID_TO_INT(id) >= 0 && pd->indexedGetter) {
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
  JS_ASSERT(pd);
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
  "ObjectTemplate-Object", // name
  JSCLASS_HAS_PRIVATE, // flags
  JS_PropertyStub, // addProperty
  o_DeleteProperty, // delProperty
  o_GetProperty, // getProperty
  o_SetProperty, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  o_finalize, // finalize
  NULL, // unused
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // trace
};

JSBool
ot_SetProperty(JSContext* cx,
               JSObject* obj,
               jsid id,
               JSBool strict,
               jsval* vp)
{
  PrivateData* data = PrivateData::Get(cx, obj);
  JS_ASSERT(data);

  Value v(*vp);
  Local<Value> value = Local<Value>::New(&v);
  data->attributes.addAttribute(id, value);

  // We do not actually set anything on our internal object!
  *vp = JSVAL_VOID;
  return JS_TRUE;
}

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
  ot_SetProperty, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  ot_finalize, // finalize
  NULL, // unused
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // trace
};

} // anonymous namespace

ObjectTemplate::ObjectTemplate() :
  Template(&gObjectTemplateClass)
{
  PrivateData* data = new PrivateData();
  (void)JS_SetPrivate(cx(), InternalObject(), data);
}

// static
bool ObjectTemplate::IsObjectTemplate(Handle<Value> v) {
  if (v.IsEmpty())
    return false;
  Handle<Object> o = v->ToObject();
  if (o.IsEmpty())
    return false;
  JSObject *obj = **o;
  return &gObjectTemplateClass == JS_GET_CLASS(cx(), obj);
}

void ObjectTemplate::SetPrototype(Handle<ObjectTemplate> o) {
  PrivateData* pd = PrivateData::Get(InternalObject());
  JS_ASSERT(pd);
  pd->prototype = Persistent<ObjectTemplate>::New(o);
}

// static
Local<ObjectTemplate>
ObjectTemplate::New()
{
  ObjectTemplate d;
  return Local<ObjectTemplate>::New(&d);
}

Local<Object>
ObjectTemplate::NewInstance(JSObject* parent)
{
  PrivateData* pd = PrivateData::Get(InternalObject());
  JS_ASSERT(pd);

  // We've set everything we care about on our InternalObject, so we can assign
  // that to the prototype of our new object.
  JSClass* cls = &pd->cls;
  JSObject* proto = pd->prototype.IsEmpty()
                  ? NULL
                  : **pd->prototype->NewInstance();
  if (!parent)
    parent = **Context::GetCurrent()->Global();

  JSObject* obj = JS_NewObject(cx(), cls, proto, parent);
  if (!obj) {
    // wtf?
    UNIMPLEMENTEDAPI(Local<Object>());
  }

  ObjectTemplateHandle* handle = new ObjectTemplateHandle(this);
  if (!JS_SetPrivate(cx(), obj, handle)) {
    delete handle;
    // TODO handle error better
    UNIMPLEMENTEDAPI(Local<Object>());
  }

  Object o(obj);

  // Set all attributes that were added with Template::Set on the object.
  AttributeStorage::Range attributes = pd->attributes.all();
  while (!attributes.empty()) {
    AttributeStorage::Entry& entry = attributes.front();
    Handle<Value> v = entry.value;
    if (FunctionTemplate::IsFunctionTemplate(v)) {
      FunctionTemplate *tmpl = reinterpret_cast<FunctionTemplate*>(*v);
      v = tmpl->GetFunction(parent);
    } else if (IsObjectTemplate(v)) {
      ObjectTemplate *tmpl = reinterpret_cast<ObjectTemplate*>(*v);
      v = tmpl->NewInstance(parent);
    }
    (void)o.Set(String::FromJSID(entry.key), v);
    attributes.popFront();
  }

  // Set all things that were added with SetAccessor on the object.
  AccessorStorage::Range accessors = pd->accessors.all();
  while (!accessors.empty()) {
    AccessorStorage::Entry& entry = accessors.front();
    AccessorStorage::PropertyData& data = entry.value;
    (void)o.SetAccessor(String::FromJSID(entry.key), data.getter, data.setter,
                        data.data, DEFAULT, data.attribute);
    accessors.popFront();
  }

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
  PrivateData* pd = PrivateData::Get(InternalObject());
  JS_ASSERT(pd);
  jsid id;
  (void)JS_ValueToId(cx(), name->native(), &id);
  pd->accessors.addAccessor(id, getter, setter, data, attribute);
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
  JS_ASSERT(pd);
  pd->namedGetter = getter;
  pd->namedSetter = setter;
  pd->namedQuery = query;
  pd->namedDeleter = deleter;
  pd->namedEnumerator = enumerator;
  if (data.IsEmpty()) {
    pd->namedData = Persistent<Value>::New(Undefined());
  }
  else {
    pd->namedData = Persistent<Value>::New(data);
  }
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
  JS_ASSERT(pd);
  pd->indexedGetter = getter;
  pd->indexedSetter = setter;
  pd->indexedQuery = query;
  pd->indexedDeleter = deleter;
  pd->indexedEnumerator = enumerator;
  if (data.IsEmpty()) {
    pd->indexedData = Persistent<Value>::New(Undefined());
  }
  else {
    pd->indexedData = Persistent<Value>::New(data);
  }
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
  PrivateData* pd = PrivateData::Get(InternalObject());
  JS_ASSERT(pd);
  return JSCLASS_RESERVED_SLOTS(&pd->cls);
}

void
ObjectTemplate::SetInternalFieldCount(int value)
{
  JS_ASSERT(value >= 0 && value < (1 << JSCLASS_RESERVED_SLOTS_WIDTH));
  PrivateData* pd = PrivateData::Get(InternalObject());
  JS_ASSERT(pd);
  JSClass* clasp = &pd->cls;
  clasp->flags &= ~(JSCLASS_RESERVED_SLOTS_MASK << JSCLASS_RESERVED_SLOTS_SHIFT);
  clasp->flags |= JSCLASS_HAS_RESERVED_SLOTS(value);
}

} // namespace v8
