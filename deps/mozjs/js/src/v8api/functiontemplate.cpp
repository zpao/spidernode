#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(FunctionTemplate) == sizeof(GCReference));

namespace {
const int kInstanceSlot = 0;
const int kDataSlot = 1;
const int kCallbackSlot = 2;
const int kCallbackParitySlot = 3;
const int kCachedFunction = 4;
const int kFunctionName = 5;
const int kFunctionTemplateSlots = 6;

struct PrivateData {
  AttributeStorage attributes;

  static PrivateData* Get(JSContext* cx, JSObject* obj) {
    return reinterpret_cast<PrivateData*>(JS_GetPrivate(cx, obj));
  }
};

JSBool
ft_SetProperty(JSContext* cx,
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

  // Allow prototype to actually be set
  if (JSID_IS_STRING(id)) {
    JSString* str = JSID_TO_STRING(id);
    JSBool equal;
    if (JS_StringEqualsAscii(cx, str, "prototype", &equal)) {
      // Note: intentionally done on a separate line to avoid
      // order-of-evaluation issues in aggressive compilers.
      if (equal) {
        return JS_StrictPropertyStub(cx, obj, id, strict, vp);
      }
    }
  }

  // We do not actually set anything on our internal object!
  *vp = JSVAL_VOID;
  return JS_TRUE;
}

void
ft_finalize(JSContext* cx,
            JSObject* obj)
{
  PrivateData* data = PrivateData::Get(cx, obj);
  delete data;
}


} // anonymous namespace

JSClass FunctionTemplate::sFunctionTemplateClass = {
  "FunctionTemplate", // name
  JSCLASS_HAS_PRIVATE |
  JSCLASS_HAS_RESERVED_SLOTS(kFunctionTemplateSlots), // flags
  JS_PropertyStub, // addProperty
  JS_PropertyStub, // delProperty
  JS_PropertyStub, // getProperty
  ft_SetProperty, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  ft_finalize, // finalize
  NULL, // unused
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // trace
};

FunctionTemplate::FunctionTemplate() :
  Template(&sFunctionTemplateClass)
{
  JS_SetPrivate(cx(), JSVAL_TO_OBJECT(mVal), new PrivateData);

  Handle<ObjectTemplate> protoTemplate = ObjectTemplate::New();
  Set("prototype", protoTemplate);
  // Instance template
  Local<ObjectTemplate> instanceTemplate = ObjectTemplate::New();
  instanceTemplate->SetPrototype(protoTemplate);

  InternalObject().SetInternalField(kInstanceSlot, Handle<Object>(&instanceTemplate->InternalObject()));
  InternalObject().SetInternalField(kDataSlot, v8::Undefined());
  InternalObject().SetPointerInInternalField(kCallbackSlot, NULL);
  InternalObject().SetPointerInInternalField(kCallbackParitySlot, NULL);
}

JSBool
FunctionTemplate::CallCallback(JSContext* cx,
                               uintN argc,
                               jsval* vp)
{
  Object fnTemplateObj(JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
  Handle<Object> ftData = fnTemplateObj.GetInternalField(0).As<Object>();
  Handle<ObjectTemplate> instanceTemplate =
    reinterpret_cast<ObjectTemplate*>(*ftData->GetInternalField(kInstanceSlot));
  JSIntPtr maskedCallback = reinterpret_cast<JSIntPtr>(
    ftData->GetPointerFromInternalField(kCallbackSlot)
  );
  JSIntPtr residual = reinterpret_cast<JSIntPtr>(
    ftData->GetPointerFromInternalField(kCallbackParitySlot)
  );
  InvocationCallback callback = reinterpret_cast<InvocationCallback>(
    maskedCallback | (residual >> 1)
  );
  Local<Value> data = ftData->GetInternalField(kDataSlot);

  JSObject* thiz = NULL;
  bool isConstructing = JS_IsConstructing(cx, vp);
  if (isConstructing) {
    thiz = **instanceTemplate->NewInstance();
    Handle<Value> proto = fnTemplateObj.Get(String::NewSymbol("prototype"));
    if (!proto.IsEmpty() && proto->IsObject()) {
      Handle<Object> o = proto->ToObject();
      (void) JS_SetPrototype(cx, thiz, **o);
    }
  } else {
    thiz = JS_THIS_OBJECT(cx, vp);
  }
  Arguments args(cx, thiz, argc, vp, data);
  Handle<Value> ret;
  if (callback) {
    ret = callback(args);
  }
  if (ret.IsEmpty()) {
    ret = v8::Undefined();
  }
  if (isConstructing) {
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(thiz));
  }
  else {
    JS_SET_RVAL(cx, vp, ret->native());
  }
  return !JS_IsExceptionPending(cx);
}

bool FunctionTemplate::IsFunctionTemplate(Handle<Value> v) {
  if (v.IsEmpty())
    return false;
  Handle<Object> o = v->ToObject();
  if (o.IsEmpty())
    return false;
  JSObject *obj = **o;
  return &sFunctionTemplateClass == JS_GET_CLASS(cx(), obj);
}

// static
Local<FunctionTemplate>
FunctionTemplate::New(InvocationCallback callback,
                      Handle<Value> data,
                      Handle<Signature>)
{
  FunctionTemplate ft;
  if (callback) {
    ft.SetCallHandler(callback, data);
  }
  return Local<FunctionTemplate>::New(&ft);
}

Local<Function>
FunctionTemplate::GetFunction(JSObject* parent)
{
  Local<Function> fn = InternalObject().GetInternalField(kCachedFunction).As<Function>();
  if (!fn.IsEmpty()) {
    return Local<Function>::New(fn);
  }
  Handle<Value> name = InternalObject().GetInternalField(kFunctionName);
  if (name.IsEmpty())
    name = String::Empty();
  String::AsciiValue fnName(name);
  JSFunction* func =
    JS_NewFunction(cx(), CallCallback, 0, JSFUN_CONSTRUCTOR, NULL, *fnName);
  JSObject* obj = JS_GetFunctionObject(func);
  Object o(obj);
  Local<String> prototypeStr = String::NewSymbol("prototype");
  (void)o.Set(prototypeStr, PrototypeTemplate()->NewInstance(parent));
  Local<Value> thiz = Local<Value>::New(&InternalObject());
  o.SetInternalField(0, thiz);
  fn = Local<Function>::New(reinterpret_cast<Function*>(&o));
  InternalObject().SetInternalField(kCachedFunction, fn);

  PrivateData* pd = PrivateData::Get(cx(), InternalObject());
  // Set all attributes that were added with Template::Set on the object.
  AttributeStorage::Range attributes = pd->attributes.all();
  while (!attributes.empty()) {
    AttributeStorage::Entry& entry = attributes.front();
    Handle<Value> v = entry.value;
    if (FunctionTemplate::IsFunctionTemplate(v)) {
      FunctionTemplate *tmpl = reinterpret_cast<FunctionTemplate*>(*v);
      v = tmpl->GetFunction(parent);
    } else if (ObjectTemplate::IsObjectTemplate(v)) {
      ObjectTemplate *tmpl = reinterpret_cast<ObjectTemplate*>(*v);
      v = tmpl->NewInstance(parent);
    }
    (void)o.Set(String::FromJSID(entry.key), v);
    attributes.popFront();
  }

  return fn;
}

void
FunctionTemplate::SetCallHandler(InvocationCallback callback,
                                 Handle<Value> data)
{
  JSIntPtr ptrcallback = reinterpret_cast<JSIntPtr>(callback);
  InternalObject().SetPointerInInternalField(kCallbackSlot, reinterpret_cast<void*>(ptrcallback & ~0x1));
  InternalObject().SetPointerInInternalField(kCallbackParitySlot, reinterpret_cast<void*>((ptrcallback & 0x1) << 1));

  if (data.IsEmpty()) {
    // XXX: this is not correct
    InternalObject().SetInternalField(kDataSlot, v8::Undefined());
  } else {
    InternalObject().SetInternalField(kDataSlot, data);
  }
}

Local<ObjectTemplate>
FunctionTemplate::InstanceTemplate()
{
  Local<Value> instance = InternalObject().GetInternalField(kInstanceSlot);
  if (instance.IsEmpty())
    return Local<ObjectTemplate>();
  return Local<ObjectTemplate>(reinterpret_cast<ObjectTemplate*>(*instance));
}

void
FunctionTemplate::Inherit(Handle<FunctionTemplate> parent)
{
  (void)JS_SetPrototype(cx(), InternalObject(), parent->InternalObject());
}

Local<ObjectTemplate>
FunctionTemplate::PrototypeTemplate()
{
  Local<Value> proto = InternalObject().Get(String::New("prototype"));
  if (proto.IsEmpty())
    return Local<ObjectTemplate>();
  JS_ASSERT(proto->IsObject());
  return Local<ObjectTemplate>(reinterpret_cast<ObjectTemplate*>(*proto));
}

void
FunctionTemplate::SetClassName(Handle<String> name)
{
  InternalObject().SetInternalField(kFunctionName, name);
}

void
FunctionTemplate::SetHiddenPrototype(bool value)
{
  UNIMPLEMENTEDAPI();
}

bool
FunctionTemplate::HasInstance(Handle<Value> v)
{
  Handle<Object> object = v->ToObject();
  if (object.IsEmpty())
    return false;
  Handle<Object> proto = object->GetPrototype().As<Object>();
  if (proto.IsEmpty()) {
    return false;
  }
  return InternalObject().Equals(proto);
}

} // namespace v8
