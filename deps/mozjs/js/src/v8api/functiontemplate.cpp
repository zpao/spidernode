#include "v8-internal.h"

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(FunctionTemplate) == sizeof(GCReference));

namespace {
const int kInstanceSlot = 0;
const int kDataSlot = 1;
const int kCallbackSlot = 2;
const int kCallbackParitySlot = 3;
const int kFunctionTemplateSlots = 4;
} // anonymous namespace

JSClass FunctionTemplate::sFunctionTemplateClass = {
  "FunctionTemplate", // name
  JSCLASS_HAS_RESERVED_SLOTS(kFunctionTemplateSlots), // flags
  JS_PropertyStub, // addProperty
  JS_PropertyStub, // delProperty
  JS_PropertyStub, // getProperty
  JS_StrictPropertyStub, // setProperty
  JS_EnumerateStub, // enumerate
  JS_ResolveStub, // resolve
  JS_ConvertStub, // convert
  JS_FinalizeStub, // finalize
  NULL, // unused
  NULL, // checkAccess
  NULL, // call
  NULL, // construct
  NULL, // xdrObject
  NULL, // hasInstance
  NULL, // trace
};

FunctionTemplate::FunctionTemplate() :
  Template(JS_GetFunctionObject(JS_NewFunction(cx(), CallCallback, 0, JSFUN_CONSTRUCTOR, 0, 0)))
{
  Handle<ObjectTemplate> protoTemplate = ObjectTemplate::New();
  Set("prototype", protoTemplate);
  // Instance template
  Local<ObjectTemplate> instanceTemplate = ObjectTemplate::New();
  instanceTemplate->InternalObject().SetPrototype(Handle<Object>(&protoTemplate->InternalObject()));

  Object ftData(JS_NewObject(cx(), &sFunctionTemplateClass, NULL, NULL));
  ftData.SetInternalField(kInstanceSlot, Handle<Object>(&instanceTemplate->InternalObject()));
  ftData.SetInternalField(kDataSlot, v8::Undefined());
  ftData.SetPointerInInternalField(kCallbackSlot, NULL);
  ftData.SetPointerInInternalField(kCallbackParitySlot, NULL);
  InternalObject().SetInternalField(0, Handle<Object>(&ftData));
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
  } else {
    thiz = JS_THIS_OBJECT(cx, vp);
  }
  Arguments args(cx, thiz, argc, vp, data);
  Handle<Value> ret;
  if (callback) {
    ret = callback(args);
  }
  else {
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
FunctionTemplate::GetFunction()
{
  Function *fn = reinterpret_cast<Function*>(&InternalObject());
  return Local<Function>::New(fn);
}

void
FunctionTemplate::SetCallHandler(InvocationCallback callback,
                                 Handle<Value> data)
{
  Handle<Object> ftData = InternalObject().GetInternalField(0).As<Object>();
  JSIntPtr ptrcallback = reinterpret_cast<JSIntPtr>(callback);
  ftData->SetPointerInInternalField(kCallbackSlot, reinterpret_cast<void*>(ptrcallback & ~0x1));
  ftData->SetPointerInInternalField(kCallbackParitySlot, reinterpret_cast<void*>((ptrcallback & 0x1) << 1));

  if (data.IsEmpty()) {
    // XXX: this is not correct
    ftData->SetInternalField(kDataSlot, v8::Undefined());
  } else {
    ftData->SetInternalField(kDataSlot, data);
  }
}

Local<ObjectTemplate>
FunctionTemplate::InstanceTemplate()
{
  Handle<Object> ftData = InternalObject().GetInternalField(0).As<Object>();
  Local<Value> instance = ftData->GetInternalField(kInstanceSlot);
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
  return Local<ObjectTemplate>(reinterpret_cast<ObjectTemplate*>(*proto));
}

void
FunctionTemplate::SetClassName(Handle<String> name)
{
  UNIMPLEMENTEDAPI();
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
