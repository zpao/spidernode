#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(FunctionTemplate) == sizeof(GCReference));

namespace {

struct PrivateData {
  AttributeStorage attributes;
  Traced<ObjectTemplate> instanceTemplate;
  InvocationCallback callback;
  Traced<Value> callbackData;
  Traced<String> name;
  Traced<Function> cachedFunction;

  static PrivateData* Get(JSObject* obj) {
    return reinterpret_cast<PrivateData*>(JS_GetPrivate(obj));
  }
};

JSBool
ft_SetProperty(JSContext* cx,
               JSObject* obj,
               jsid id,
               JSBool strict,
               jsval* vp)
{
  PrivateData* data = PrivateData::Get(obj);
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
ft_Trace(JSTracer* tracer, JSObject* obj) {
  PrivateData* data = PrivateData::Get(obj);
  JS_ASSERT(data);
  data->attributes.trace(tracer);
  data->instanceTemplate.trace(tracer);
  data->callbackData.trace(tracer);
  data->name.trace(tracer);
  data->cachedFunction.trace(tracer);
}

void
ft_finalize(JSContext* cx, JSObject* obj)
{
  PrivateData* data = PrivateData::Get(obj);
  delete_(data);
}

JSClass gFunctionTemplateClass = {
  "FunctionTemplate", // name
  JSCLASS_HAS_PRIVATE, // flags
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
  ft_Trace, // trace
};


} // anonymous namespace

namespace internal {
bool IsFunctionTemplate(Handle<Value> v) {
  if (v.IsEmpty())
    return false;
  Handle<Object> o = v->ToObject();
  if (o.IsEmpty())
    return false;
  JSObject *obj = **o;
  return &gFunctionTemplateClass == JS_GetClass(obj);
}
}

FunctionTemplate::FunctionTemplate() :
  Template(&gFunctionTemplateClass)
{
  PrivateData* data = new_<PrivateData>();
  JS_SetPrivate(JSVAL_TO_OBJECT(mVal), data);

  Handle<ObjectTemplate> protoTemplate = ObjectTemplate::New();
  Set("prototype", protoTemplate);
  // Instance template
  Local<ObjectTemplate> instanceTemplate = ObjectTemplate::New();
  instanceTemplate->SetPrototype(protoTemplate);

  data->instanceTemplate = instanceTemplate;
  data->callback = NULL;
}

JSBool
FunctionTemplate::CallCallback(JSContext* cx,
                               uintN argc,
                               jsval* vp)
{
  ApiExceptionBoundary boundary;
  HandleScope scope;
  Object fn(JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
  PrivateData* pd = PrivateData::Get(**fn.GetInternalField(0).As<Object>());
  Handle<ObjectTemplate> instanceTemplate = pd->instanceTemplate.get();
  InvocationCallback callback = pd->callback;
  Local<Value> data = pd->callbackData.get();

  JSObject* thiz = NULL;
  bool isConstructing = JS_IsConstructing(cx, vp);
  if (isConstructing) {
    thiz = **instanceTemplate->NewInstance();
    Handle<Value> proto = fn.Get(String::NewSymbol("prototype"));
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
  return boundary.noExceptionOccured();
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
  HandleScope scope;
  PrivateData* pd = PrivateData::Get(InternalObject());
  Local<Function> fn = pd->cachedFunction.get();
  if (!fn.IsEmpty()) {
    return scope.Close(Local<Function>::New(fn));
  }
  Handle<Value> name = pd->name.get();
  if (name.IsEmpty())
    name = String::Empty();
  String::AsciiValue fnName(name);
  JSFunction* func =
      js::NewFunctionWithReserved(cx(), CallCallback, 0, JSFUN_CONSTRUCTOR,
                                  NULL, *fnName);
  JSObject* obj = JS_GetFunctionObject(func);
  Object o(obj);
  Local<String> prototypeStr = String::NewSymbol("prototype");
  (void)o.Set(prototypeStr, PrototypeTemplate()->NewInstance(parent));
  Local<Value> thiz = Local<Value>::New(&InternalObject());
  o.SetInternalField(0, thiz);
  fn = Local<Function>::New(reinterpret_cast<Function*>(&o));
  pd->cachedFunction = fn;

  // Set all attributes that were added with Template::Set on the object.
  AttributeStorage::Range attributes = pd->attributes.all();
  while (!attributes.empty()) {
    AttributeStorage::Entry& entry = attributes.front();
    Handle<Value> v = entry.value.get();
    if (IsFunctionTemplate(v)) {
      FunctionTemplate *tmpl = reinterpret_cast<FunctionTemplate*>(*v);
      v = tmpl->GetFunction(parent);
    } else if (IsObjectTemplate(v)) {
      ObjectTemplate *tmpl = reinterpret_cast<ObjectTemplate*>(*v);
      v = tmpl->NewInstance(parent);
    }
    (void)o.Set(String::FromJSID(entry.key), v);
    attributes.popFront();
  }

  return scope.Close(fn);
}

void
FunctionTemplate::SetCallHandler(InvocationCallback callback,
                                 Handle<Value> data)
{
  PrivateData* pd = PrivateData::Get(InternalObject());
  pd->callback = callback;
  pd->callbackData = data;
}

Local<ObjectTemplate>
FunctionTemplate::InstanceTemplate()
{
  PrivateData* pd = PrivateData::Get(InternalObject());
  return pd->instanceTemplate.get();
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
  PrivateData* pd = PrivateData::Get(InternalObject());
  pd->name = name;
  pd->instanceTemplate->SetObjectName(name);
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
