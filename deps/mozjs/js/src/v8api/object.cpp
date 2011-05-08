#include "v8-internal.h"
#include "jstl.h"
#include "jshashtable.h"
#include "jsobj.h"
#include "jstypedarray.h"
#include "jsproxy.h"
#include "mozilla/Util.h"
#include <limits>
using namespace mozilla;

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(Object) == sizeof(GCReference));

struct Object::PrivateData {
  PrivateData() :
    hiddenValues(Persistent<Object>::New(Object::New()))
  {
  }

  ~PrivateData()
  {
    JS_ASSERT(!hiddenValues.IsEmpty());
    hiddenValues.Dispose();
  }

  Persistent<Object> hiddenValues;
  AccessorStorage properties;
};

typedef js::HashMap<JSObject*,Object::PrivateData*, js::DefaultHasher<JSObject*>, js::SystemAllocPolicy> ObjectPrivateDataMap;

static ObjectPrivateDataMap *gPrivateDataMap = 0;
static ObjectPrivateDataMap& privateDataMap() {
  if (!gPrivateDataMap) {
    gPrivateDataMap = cx()->new_<ObjectPrivateDataMap>();
    gPrivateDataMap->init(11);
  }
  return *gPrivateDataMap;
}

void
internal::TraceObjectInternals(JSTracer* tracer,
                               void*)
{
  if (!gPrivateDataMap) {
    return;
  }
  ObjectPrivateDataMap::Range r = gPrivateDataMap->all();
  while (!r.empty()) {
    r.front().value->properties.trace(tracer);
    r.popFront();
  }
}

void
internal::DestroyObjectInternals()
{
  if (!gPrivateDataMap) {
    return;
  }
  ObjectPrivateDataMap::Range r = gPrivateDataMap->all();
  while (!r.empty()) {
    JS_ASSERT(!r.front().value->hiddenValues.IsEmpty());
    r.front().value->hiddenValues.Dispose();
    r.popFront();
  }

  cx()->delete_(gPrivateDataMap);
}

JSBool Object::JSAPIPropertyGetter(JSContext* cx, uintN argc, jsval* vp) {
  HandleScope scope;
  JSObject* fnObj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  jsval accessorOwner;
  (void) JS_GetReservedSlot(cx, fnObj, 0, &accessorOwner);
  JS_ASSERT(JSVAL_IS_OBJECT(accessorOwner));
  Object o(JSVAL_TO_OBJECT(accessorOwner));

  jsval name;
  (void) JS_GetReservedSlot(cx, fnObj, 1, &name);
  jsid id;
  (void) JS_ValueToId(cx, name, &id);

  AccessorStorage::PropertyData data = o.GetHiddenStore().properties.get(id);
  AccessorInfo info(data.data.get(), JS_THIS_OBJECT(cx, vp));
  Handle<Value> result = data.getter(String::FromJSID(id), info);
  JS_SET_RVAL(cx, vp, result->native());
  // XXX: this is usually correct
  return JS_TRUE;
}

JSBool Object::JSAPIPropertySetter(JSContext* cx, uintN argc, jsval* vp) {
  HandleScope scope;
  JSObject* fnObj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  jsval accessorOwner;
  (void) JS_GetReservedSlot(cx, fnObj, 0, &accessorOwner);
  JS_ASSERT(JSVAL_IS_OBJECT(accessorOwner));
  Object o(JSVAL_TO_OBJECT(accessorOwner));

  jsval name;
  (void) JS_GetReservedSlot(cx, fnObj, 1, &name);
  jsid id;
  (void) JS_ValueToId(cx, name, &id);

  AccessorStorage::PropertyData data = o.GetHiddenStore().properties.get(id);
  AccessorInfo info(data.data.get(), JS_THIS_OBJECT(cx, vp));
  Value value(*JS_ARGV(cx, vp));
  data.setter(String::FromJSID(id), &value, info);
  // XXX: this is usually correct
  return JS_TRUE;
}

bool
Object::Set(Handle<Value> key,
            Handle<Value> value,
            PropertyAttribute attribs) {
  // TODO: use String::Value
  String::AsciiValue k(key);
  jsval v = value->native();

  if (!JS_SetProperty(cx(), *this, *k, &v)) {
    TryCatch::CheckForException();
    return false;
  }

  // Can't set attributes on indexed properties
  if (!key->ToUint32().IsEmpty())
    return true;

  uintN js_attribs = 0;
  if (attribs & ReadOnly) {
    js_attribs |= JSPROP_READONLY;
  }
  if (!(attribs & DontEnum)) {
    js_attribs |= JSPROP_ENUMERATE;
  }
  if (attribs & DontDelete) {
    js_attribs |= JSPROP_PERMANENT;
  }

  JSBool wasFound;
  return JS_SetPropertyAttributes(cx(), *this, *k, js_attribs, &wasFound);
}

bool
Object::Set(JSUint32 index,
            Handle<Value> value)
{
  if (index > JSUint32(std::numeric_limits<jsint>::max())) {
    return false;
  }

  jsval& val = value->native();
  if (!JS_SetElement(cx(), *this, index, &val)) {
    TryCatch::CheckForException();
    return false;
  }
  return true;
}

bool
Object::ForceSet(Handle<Value> key,
                 Handle<Value> value,
                 PropertyAttribute attribute)
{
  // TODO implement this with JS_DefineProperty
  UNIMPLEMENTEDAPI(false);
}

Local<Value>
Object::Get(Handle<Value> key) {
  // TODO: use String::Value
  String::AsciiValue k(key);
  Value v(JSVAL_VOID);

  if (JS_GetProperty(cx(), *this, *k, &v.native())) {
    return Local<Value>::New(&v);
  }
  TryCatch::CheckForException();
  return Local<Value>();
}

Local<Value>
Object::Get(JSUint32 index)
{
  if (index > JSUint32(std::numeric_limits<jsint>::max())) {
    return Local<Value>();
  }

  Value v;
  if (JS_GetElement(cx(), *this, index, &v.native())) {
    return Local<Value>::New(&v);
  }
  TryCatch::CheckForException();
  return Local<Value>();
}

bool
Object::Has(Handle<String> key)
{
  // TODO: use String::Value
  String::AsciiValue k(key);

  JSBool found;
  if (JS_HasProperty(cx(), *this, *k, &found)) {
    return !!found;
  }
  TryCatch::CheckForException();
  return false;
}

bool
Object::Has(JSUint32 index)
{
  if (index > JSUint32(std::numeric_limits<jsint>::max())) {
    return false;
  }

  jsval val;
  if (JS_LookupElement(cx(), *this, index, &val)) {
    return !JSVAL_IS_VOID(val);
  }
  TryCatch::CheckForException();
  return false;
}

bool
Object::Delete(Handle<String> key)
{
  // TODO: use String::Value
  String::AsciiValue k(key);

  jsval val;
  if (JS_DeleteProperty2(cx(), *this, *k, &val)) {
    return val == JSVAL_TRUE;
  }
  TryCatch::CheckForException();
  return false;
}

bool
Object::Delete(JSUint32 index)
{
  if (index > JSUint32(std::numeric_limits<jsint>::max())) {
    return false;
  }

  jsval val;
  if (JS_DeleteElement2(cx(), *this, index, &val)) {
    return val == JSVAL_TRUE;
  }
  TryCatch::CheckForException();
  return false;
}

bool
Object::ForceDelete(Handle<String> key)
{
  UNIMPLEMENTEDAPI(false);
}

bool
Object::SetAccessor(Handle<String> name,
                    AccessorGetter getter,
                    AccessorSetter setter,
                    Handle<Value> data,
                    AccessControl settings,
                    PropertyAttribute attribs)
{
  if (settings != 0) {
    // We only currently support the default settings.
    UNIMPLEMENTEDAPI(false);
  }

  jsid propid;
  JS_ValueToId(cx(), name->native(), &propid);
  JSFunction* getterFn = JS_NewFunction(cx(), JSAPIPropertyGetter, 0, 0, NULL, NULL);
  if (!getterFn)
    return false;
  JSObject *getterObj = JS_GetFunctionObject(getterFn);

  JSFunction* setterFn = JS_NewFunction(cx(), JSAPIPropertySetter, 1, 0, NULL, NULL);
  if (!setterFn)
    return false;
  JSObject *setterObj = JS_GetFunctionObject(setterFn);

  JS_SetReservedSlot(cx(), getterObj, 0, native());
  JS_SetReservedSlot(cx(), getterObj, 1, name->native());
  JS_SetReservedSlot(cx(), setterObj, 0, native());
  JS_SetReservedSlot(cx(), setterObj, 1, name->native());

  uintN attributes = JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
  if (!JS_DefinePropertyById(cx(), *this, propid,
        JSVAL_VOID,
        (JSPropertyOp)getterObj,
        (JSStrictPropertyOp)setterObj,
        attributes)) {
    TryCatch::CheckForException();
    return false;
  }
  PrivateData &store = GetHiddenStore();
  store.properties.addAccessor(propid, getter, setter, data, attribs);
  return true;
}

Local<Array>
Object::GetPropertyNames()
{
  // XXX: this doesn't cover non-enumerable properties
  JSIdArray *ids = JS_Enumerate(cx(), *this);
  Local<Array> array = Array::New(ids->length);
  for (jsint i = 0; i < ids->length; i++) {
    jsid id = ids->vector[i];
    jsval val;
    if (!JS_IdToValue(cx(), id, &val)) {
      return Local<Array>();
    }
    Value v(val);
    array->Set(i, &v);
  }
  JS_DestroyIdArray(cx(), ids);
  return array;
}

Local<Value>
Object::GetPrototype()
{
  Value v(OBJECT_TO_JSVAL(JS_GetPrototype(cx(), *this)));
  return Local<Value>::New(&v);
}

bool
Object::SetPrototype(Handle<Value> prototype)
{
  Handle<Object> h(prototype.As<Object>());
  if (h.IsEmpty()) {
    // XXX: indicate error? The V8API is unclear here
    // zpao: is false enough? it seems to make sense
    return false;
  }
  return JS_SetPrototype(cx(), *this, **h) == JS_TRUE;
}

Local<Object>
Object::FindInstanceInPrototypeChain(Handle<FunctionTemplate> tmpl)
{
  UNIMPLEMENTEDAPI(Local<Object>());
}

Local<String>
Object::ObjectProtoToString()
{
  Object proto(JS_GetPrototype(cx(), *this));
  Handle<Function> toString = proto.Get(String::New("toString")).As<Function>();
  if (toString.IsEmpty()) {
    TryCatch::CheckForException();
    return Local<String>();
  }
  return toString->Call(this, 0, NULL).As<String>();
}

Local<String>
Object::GetConstructorName()
{
  Function f(JS_GetConstructor(cx(), *this));
  return Local<String>::New(*f.GetName());
}

int
Object::InternalFieldCount()
{
  JSClass *cls = JS_GET_CLASS(cx(), *this);
  if (!cls)
    return -1;
  return JSCLASS_RESERVED_SLOTS(cls);
}

Local<Value>
Object::GetInternalField(int index)
{
  Value v;
  if (!JS_GetReservedSlot(cx(), *this, index, &v.native())) {
    return Local<Value>();
  }
  return Local<Value>::New(&v);
}

void
Object::SetInternalField(int index,
                         Handle<Value> value)
{
  (void) JS_SetReservedSlot(cx(), *this, index, value->native());
}

void*
Object::GetPointerFromInternalField(int index)
{
  jsval v;
  if (!JS_GetReservedSlot(cx(), *this, index, &v)) {
    return NULL;
  }
  // XXX: this assumes there was a ptr there
  return JSVAL_TO_PRIVATE(v);
}

void
Object::SetPointerInInternalField(int index,
                                  void* value)
{
  jsval v = PRIVATE_TO_JSVAL(value);
  (void) JS_SetReservedSlot(cx(), *this, index, v);
}

bool
Object::HasRealNamedProperty(Handle<String> key)
{
  // TODO: use String::Value
  String::AsciiValue k(key);

  JSBool found;
  if (JS_AlreadyHasOwnProperty(cx(), *this, *k, &found)) {
    return !!found;
  }
  return false;
}

bool
Object::HasRealIndexedProperty(JSUint32 index)
{
  if (index > JSUint32(std::numeric_limits<jsint>::max())) {
    return false;
  }

  JSBool found;
  if (JS_AlreadyHasOwnElement(cx(), *this, index, &found)) {
    return !!found;
  }
  return false;
}

bool
Object::HasRealNamedCallbackProperty(Handle<String> key)
{
  UNIMPLEMENTEDAPI(false);
}

Local<Value>
Object::GetRealNamedPropertyInPrototypeChain(Handle<String> key)
{
  Local<Object> proto = GetPrototype().As<Object>();
  return proto->GetRealNamedProperty(key);
}

Local<Value>
Object::GetRealNamedProperty(Handle<String> key)
{
  // TODO: this needs to bypass interceptors when they're implemented
  return Get(key);
}

bool
Object::HasNamedLookupInterceptor()
{
  UNIMPLEMENTEDAPI(false);
}

bool
Object::HasIndexedLookupInterceptor()
{
  UNIMPLEMENTEDAPI(false);
}

void
Object::TurnOnAccessCheck()
{
  UNIMPLEMENTEDAPI();
}

int
Object::GetIdentityHash()
{
  JSObject *obj = *this;
  return reinterpret_cast<JSIntPtr>(obj);
}

Object::PrivateData&
Object::GetHiddenStore()
{
  ObjectPrivateDataMap::Ptr p = privateDataMap().lookup(*this);
  PrivateData *pd;
  if (p.found()) {
    pd = p->value;
  } else {
    pd = cx()->new_<PrivateData>();
    ObjectPrivateDataMap::AddPtr slot = privateDataMap().lookupForAdd(*this);
    privateDataMap().add(slot, *this, pd);
  }
  return *pd;
}

bool
Object::SetHiddenValue(Handle<String> key,
                       Handle<Value> value)
{
  PrivateData& pd = GetHiddenStore();
  JS_ASSERT(!pd.hiddenValues.IsEmpty());
  return pd.hiddenValues->Set(key, value);
}

Local<Value>
Object::GetHiddenValue(Handle<String> key)
{
  PrivateData& pd = GetHiddenStore();
  JS_ASSERT(!pd.hiddenValues.IsEmpty());
  return pd.hiddenValues->Get(key);
}

bool
Object::DeleteHiddenValue(Handle<String> key)
{
  PrivateData& pd = GetHiddenStore();
  JS_ASSERT(!pd.hiddenValues.IsEmpty());
  return pd.hiddenValues->Delete(key);
}

bool
Object::IsDirty()
{
  // TODO: see if we have any properties not from our proto
  return true;
}

Local<Object>
Object::Clone()
{
  // XXX: this doesn't cover non-enumerable properties
  JSIdArray *ids = JS_Enumerate(cx(), *this);
  Local<Object> obj = Object::New();
  for (jsint i = 0; i < ids->length; i++) {
    jsid id = ids->vector[i];
    jsval val;
    if (!JS_IdToValue(cx(), id, &val)) {
      return Local<Object>();
    }
    Value v(val);
    obj->Set(&v, Get(v.ToString()));
  }
  JS_DestroyIdArray(cx(), ids);
  return obj;
}

void
Object::SetIndexedPropertiesToPixelData(JSUint8* data,
                                        int length)
{
  UNIMPLEMENTEDAPI();
}

bool
Object::HasIndexedPropertiesInPixelData()
{
  UNIMPLEMENTEDAPI(false);
}

JSUint8*
Object::GetIndexedPropertiesPixelData()
{
  UNIMPLEMENTEDAPI(NULL);
}

int
Object::GetIndexedPropertiesPixelDataLength()
{
  UNIMPLEMENTEDAPI(0);
}

static JSObject* grabTypedArray(JSObject* obj) {
  if (js_IsTypedArray(obj))
    return obj;
  if (!obj->isObjectProxy())
    return NULL;
  jsid name = INTERNED_STRING_TO_JSID(JS_InternString(cx(), "rawArray"));
  js::Value v;
  js::JSProxy::get(cx(), obj, obj, name, &v);
  if (v.isObjectOrNull())
    return v.toObjectOrNull();
  return NULL;
}

void
Object::SetIndexedPropertiesToExternalArrayData(void* data,
                                                ExternalArrayType array_type,
                                                int number_of_elements)
{
  JS_ASSERT(HasIndexedPropertiesInExternalArrayData());
  JS_ASSERT (array_type == GetIndexedPropertiesExternalArrayDataType());
  if (number_of_elements < 0)
    return;
  js::TypedArray* arr = js::TypedArray::fromJSObject(grabTypedArray(*this));
  // Hardcoded for bytes now
  size_t elemSize = arr->slotWidth();
  size_t bufferSize = elemSize * number_of_elements;
  js::ArrayBuffer* buffer = arr->buffer;
  buffer->freeStorage(cx());
  buffer->data = data;
  buffer->byteLength = bufferSize;
  buffer->isExternal = true;
}

bool
Object::HasIndexedPropertiesInExternalArrayData()
{
  return grabTypedArray(*this) != NULL;
}

void*
Object::GetIndexedPropertiesExternalArrayData()
{
  JS_ASSERT(HasIndexedPropertiesInExternalArrayData());
  js::TypedArray* arr = js::TypedArray::fromJSObject(grabTypedArray(*this));
  // XXX: take arr->byteOffset into account?
  return arr->data;

}

ExternalArrayType
Object::GetIndexedPropertiesExternalArrayDataType()
{
  JS_ASSERT(HasIndexedPropertiesInExternalArrayData());
  return kExternalUnsignedByteArray;
}

int
Object::GetIndexedPropertiesExternalArrayDataLength()
{
  JS_ASSERT(HasIndexedPropertiesInExternalArrayData());
  js::TypedArray* arr = js::TypedArray::fromJSObject(grabTypedArray(*this));
  return arr->byteLength;
}

Object::Object(JSObject *obj) :
  Value(OBJECT_TO_JSVAL(obj))
{
}

Local<Object>
Object::New() {
  JSObject *obj = JS_NewObject(cx(), NULL, NULL, NULL);
  if (!obj) {
    return Local<Object>();
  }
  Object o(obj);
  return Local<Object>::New(&o);
}

} /* namespace v8 */
