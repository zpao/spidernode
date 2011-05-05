#include <limits>
#include <algorithm>
#include <math.h>
#include "v8-internal.h"
#include "jstypedarray.h"
#include "mozilla/Util.h"
using namespace mozilla;

namespace v8 {
using namespace internal;

//////////////////////////////////////////////////////////////////////////////
//// TryCatch class
namespace internal {
  struct ExceptionHandlerChain {
    TryCatch *catcher;
    ExceptionHandlerChain *next;
  };
  static ExceptionHandlerChain *gExnChain = 0;
}
void TryCatch::ReportError(JSContext *ctx, const char *message, JSErrorReport *report) {
  if (!gExnChain) {
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
    return;
  }
  // TODO: figure out if we care about other types of warnings
  if (!JSREPORT_IS_EXCEPTION(report->flags))
    return;

  TryCatch *current = gExnChain->catcher;
  if (current->mCaptureMessage) {
    current->mFilename = report->filename ? strdup(report->filename) : NULL;
    current->mLineBuffer = report->linebuf ? strdup(report->linebuf) : NULL;
    current->mLineNo = report->lineno;
    current->mMessage = strdup(message);
  }
}

void TryCatch::CheckForException() {
  if (!JS_IsExceptionPending(cx())) {
    return;
  }

  // Make sure that we have a TryCatch somewhere on our stack, otherwise we will
  // crash very soon!
  DebugOnly<bool> TryCatchOnStack = !!gExnChain;
  JS_ASSERT(TryCatchOnStack);

  TryCatch *current = gExnChain->catcher;
  current->mHasCaught = true;

  Value exn;
  if (!JS_GetPendingException(cx(), &exn.native())) {
    // TODO: log warning?
    return;
  }
  current->mException = Persistent<Value>::New(&exn);
  if (current->mCaptureMessage) {
    JS_ReportPendingException(cx());
  }
}


TryCatch::TryCatch() :
  mHasCaught(false),
  mCaptureMessage(true),
  mRethrown(false),
  mFilename(NULL),
  mLineBuffer(NULL),
  mLineNo(0),
  mMessage(NULL)
{
  ExceptionHandlerChain *link = new ExceptionHandlerChain;
  link->catcher = this;
  link->next = gExnChain;
  gExnChain = link;
}

TryCatch::~TryCatch() {
  ExceptionHandlerChain *link = gExnChain;
  JS_ASSERT(link->catcher == this);
  gExnChain = gExnChain->next;
  delete link;

  if (mRethrown) {
    JS_SetPendingException(cx(), mException->native());
    CheckForException();
  } else if (mHasCaught) {
    JS_ClearPendingException(cx());
  }

  Reset();
}

Handle<Value> TryCatch::ReThrow() {
  if (!HasCaught()) {
    return Handle<Value>();
  }
  mRethrown = true;
  return Undefined();
}

Local<Value> TryCatch::StackTrace() const {
  Handle<Object> obj(mException.As<Object>());
  if (obj.IsEmpty())
    return Local<Value>();
  return obj->Get(String::New("stack"));
}

Local<v8::Message> TryCatch::Message() const {
  v8::Message msg(mMessage, mFilename, mLineBuffer, mLineNo);
  return Local<v8::Message>::New(&msg);
}

void TryCatch::Reset() {
  if (!mException.IsEmpty()) {
    mException.Dispose();
    mException.Clear();
  }
  if (mFilename) {
    free(mFilename);
  }
  if (mLineBuffer) {
    free(mLineBuffer);
  }
  if (mMessage) {  
    free(mMessage);
  }
  mFilename = mLineBuffer = mMessage = NULL;
  mLineNo = 0;

  mHasCaught = false;
}

void TryCatch::SetVerbose(bool value) {
  // TODO: figure out what to do with this
}

void TryCatch::SetCaptureMessage(bool value) {
  mCaptureMessage = value;
}

//////////////////////////////////////////////////////////////////////////////
//// Context class

namespace internal {
  struct ContextChain {
    Context* ctx;
    ContextChain *next;
  };
  static ContextChain *gContextChain = 0;
}

Context::Context(JSObject *global) :
  internal::SecretObject<internal::GCReference>(global)
{}

Local<Context> Context::GetEntered() {
  return Local<Context>::New(gContextChain->ctx);
}

Local<Context> Context::GetCurrent() {
  // XXX: This is probably not right
  if (gContextChain) {
    return Local<Context>::New(gContextChain->ctx);
  } else {
    return Local<Context>();
  }
}

void Context::Enter() {
  ContextChain *link = new ContextChain;
  link->next = gContextChain;
  link->ctx = this;
  JS_SetGlobalObject(cx(), InternalObject());
  gContextChain = link;
}

void Context::Exit() {
  // Sometimes a context scope can hang around after V8::Dispose is called
  if (v8::internal::disposed())
    return;
  ContextChain *link = gContextChain;
  gContextChain = gContextChain->next;
  delete link;
  JSObject *global = gContextChain ? gContextChain->ctx->InternalObject() : NULL;
  JS_SetGlobalObject(cx(), global);
}

Persistent<Context> Context::New(
      ExtensionConfiguration* config,
      Handle<ObjectTemplate> global_template,
      Handle<Value> global_object) {
  if (!global_object.IsEmpty())
    UNIMPLEMENTEDAPI(Persistent<Context>());
  JSObject *global = JS_NewGlobalObject(cx(), &global_class);

  JS_InitStandardClasses(cx(), global);
  (void)js_InitTypedArrayClasses(cx(), global);
  if (!global_template.IsEmpty()) {
    JS_SetPrototype(cx(), global, **global_template->NewInstance(global));
  }

  return Persistent<Context>(new Context(global));
}

//////////////////////////////////////////////////////////////////////////////
//// Resource Constraints

ResourceConstraints::ResourceConstraints() :
  mMaxYoungSpaceSize(0),
  mMaxOldSpaceSize(0),
  mMaxExecutableSize(0),
  mStackLimit(NULL)
{
}

bool SetResourceConstraints(ResourceConstraints *constraints) {
  // TODO: Pull the relevent information out that applies to SM and set some
  //       globals that would be used.
  return true;
}

//////////////////////////////////////////////////////////////////////////////
//// Value class

JS_STATIC_ASSERT(sizeof(Value) == sizeof(GCReference));

Local<Boolean> Value::ToBoolean() const {
  JSBool b;
  if (!JS_ValueToBoolean(cx(), mVal, &b)) {
    TryCatch::CheckForException();
    return Local<Boolean>();
  }

  Boolean value(b);
  return Local<Boolean>::New(&value);
}

Local<Number> Value::ToNumber() const {
  double d;
  if (!JS_ValueToNumber(cx(), mVal, &d)) {
    TryCatch::CheckForException();
    return Local<Number>();
  }
  return Number::New(d);
}

Local<String> Value::ToString() const {
  // TODO Allocate this in a way that doesn't leak
  JSString *str = JS_ValueToString(cx(), mVal);
  if (!str) {
    TryCatch::CheckForException();
    return Local<String>();
  }
  String s(str);
  return Local<String>::New(&s);
}

Local<Uint32> Value::ToUint32() const {
  JSUint32 i;
  if (!JS_ValueToECMAUint32(cx(), mVal, &i)) {
    TryCatch::CheckForException();
    return Local<Uint32>();
  }
  // XXX this is somehow still needed to make test_PropertyAttributes pass.
  if (!IsNumber()) {
    return Local<Uint32>();
  }
  Uint32 v(i);
  return Local<Uint32>::New(&v);
}

Local<Int32> Value::ToInt32() const {
  JSInt32 i;
  if (!JS_ValueToECMAInt32(cx(), mVal, &i)) {
    TryCatch::CheckForException();
    return Local<Int32>();
  }
  Int32 v(i);
  return Local<Int32>::New(&v);
}

Local<Integer>
Value::ToInteger() const
{
  // TODO should be supporting 64bit wide ints here
  JSInt32 i;
  if (!JS_ValueToECMAInt32(cx(), mVal, &i)) {
    TryCatch::CheckForException();
    return Local<Integer>();
  }
  return Integer::New(i);
}

Local<Object>
Value::ToObject() const
{
  if (JSVAL_IS_OBJECT(mVal)) {
    Value* val = const_cast<Value*>(this);
    return Local<Object>::New(reinterpret_cast<Object*>(val));
  }

  // 'undefined' can't be converted to an object. Throw a TypeError.
  if (JSVAL_IS_VOID(mVal)) {
    Handle<String> msg = String::New("undefined has no properties");
    Handle<Value> type_error = Exception::TypeError(msg);
    ThrowException(type_error);

    TryCatch::CheckForException();
    return Local<Object>();
  }

  JSObject *obj;
  if (!JS_ValueToObject(cx(), mVal, &obj)) {
    TryCatch::CheckForException();
    return Local<Object>();
  }
  Object o(obj);
  return Local<Object>::New(&o);
}

bool Value::IsFunction() const {
  if (!IsObject())
    return false;
  JSObject *obj = JSVAL_TO_OBJECT(mVal);
  return JS_ObjectIsFunction(cx(), obj);
}

bool Value::IsArray() const {
  if (!IsObject())
    return false;
  JSObject *obj = JSVAL_TO_OBJECT(mVal);
  return JS_IsArrayObject(cx(), obj);
}

bool Value::IsDate() const {
  if (!IsObject())
    return false;
  JSObject *obj = JSVAL_TO_OBJECT(mVal);
  return JS_ObjectIsDate(cx(), obj);
}

bool Value::IsUint32() const {
  if (!this->IsNumber())
    return false;

  double d = this->NumberValue();
  return d >= 0 &&
         (this->IsInt32() || (d <= std::numeric_limits<JSUint32>::max() && ceil(d) == floor(d)));
}

bool
Value::BooleanValue() const
{
  JSBool b;
  JS_ValueToBoolean(cx(), mVal, &b);
  return b == JS_TRUE;
}

double
Value::NumberValue() const
{
  Local<Number> n = ToNumber();
  if (n.IsEmpty()) {
    return JSVAL_TO_DOUBLE(JS_GetNaNValue(cx()));
  }
  return n->Value();
}

JSInt64
Value::IntegerValue() const
{
  // There are no 64 bit integers, so just return the 32bit one
  if (JSVAL_IS_INT(mVal) == JS_TRUE) {
    return JSVAL_TO_INT(mVal);
  }

  Local<Number> n = ToInteger();
  if (n.IsEmpty()) {
    return 0;
  }
  return n->Value();
}

JSUint32
Value::Uint32Value() const
{
  Local<Uint32> n = ToUint32();
  if (n.IsEmpty())
    return 0;
  return n->Value();
}

JSInt32
Value::Int32Value() const
{
  Local<Int32> n = ToInt32();
  if (n.IsEmpty())
    return 0;
  return n->Value();
}

bool
Value::Equals(Handle<Value> other) const
{
  JSBool equal;
  JS_LooselyEqual(cx(), mVal, other->native(), &equal);
  return equal == JS_TRUE;
}

bool
Value::StrictEquals(Handle<Value> other) const
{
  JSBool equal;
  // XXX: check for error. This can fail if they are ropes that fail to turn into strings
  (void) JS_StrictlyEqual(cx(), mVal, other->native(), &equal);
  return equal == JS_TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//// Boolean class

JS_STATIC_ASSERT(sizeof(Boolean) == sizeof(GCReference));

Handle<Boolean> Boolean::New(bool value) {
  static Boolean sTrue(true);
  static Boolean sFalse(false);
  return value ? &sTrue : &sFalse;
}

//////////////////////////////////////////////////////////////////////////////
//// Number class

JS_STATIC_ASSERT(sizeof(Number) == sizeof(GCReference));

Local<Number> Number::New(double d) {
  jsval val;
  if (!JS_NewNumberValue(cx(), d, &val))
    return Local<Number>();

  Number n(val);
  return Local<Number>::New(&n);
}

double Number::Value() const {
  double d;
  JS_ValueToNumber(cx(), mVal, &d);
  return d;
}


//////////////////////////////////////////////////////////////////////////////
//// Integer class

JS_STATIC_ASSERT(sizeof(Integer) == sizeof(GCReference));

Local<Integer> Integer::New(JSInt32 value) {
  jsval val = INT_TO_JSVAL(value);
  Integer i(val);
  return Local<Integer>::New(&i);
}
Local<Integer> Integer::NewFromUnsigned(JSUint32 value) {
  jsval val = UINT_TO_JSVAL(value);
  Integer i(val);
  return Local<Integer>::New(&i);
}
JSInt64 Integer::Value() const {
  // XXX We should keep track of mIsDouble or something, but that wasn't working...
  if (JSVAL_IS_INT(mVal)) {
    return static_cast<JSInt64>(JSVAL_TO_INT(mVal));
  }
  else {
    return static_cast<JSInt64>(JSVAL_TO_DOUBLE(mVal));
  }
}

////////////////////////////////////////////////////////////////////////////////
//// Int32 class

JS_STATIC_ASSERT(sizeof(Int32) == sizeof(GCReference));

JSInt32 Int32::Value() {
  return JSVAL_TO_INT(mVal);
}

////////////////////////////////////////////////////////////////////////////////
//// Uint32 class

JS_STATIC_ASSERT(sizeof(Uint32) == sizeof(GCReference));

JSUint32 Uint32::Value() {
  if (JSVAL_IS_INT(mVal)) {
    return static_cast<JSUint32>(JSVAL_TO_INT(mVal));
  }
  return static_cast<JSUint32>(JSVAL_TO_DOUBLE(mVal));
}


//////////////////////////////////////////////////////////////////////////////
//// Date class

JS_STATIC_ASSERT(sizeof(Date) == sizeof(GCReference));

Local<Value> Date::New(double time) {
  // We floor the value since anything after the decimal is not used.
  // This keeps us from having to specialize Value::NumberValue.
  JSObject *obj = JS_NewDateObjectMsec(cx(), floor(time));
  Value v(OBJECT_TO_JSVAL(obj));
  return Local<Value>::New(&v);
}


//////////////////////////////////////////////////////////////////////////////
//// Primitives & basic values

Handle<Primitive> Undefined() {
  static Primitive p(JSVAL_VOID);
  return Handle<Primitive>(&p);
}

Handle<Primitive> Null() {
  static Primitive p(JSVAL_NULL);
  return Handle<Primitive>(&p);
}

Handle<Boolean> True() {
  return Boolean::New(true);
}

Handle<Boolean> False() {
  return Boolean::New(false);
}



//////////////////////////////////////////////////////////////////////////////
//// ScriptOrigin class

ScriptOrigin::ScriptOrigin(Handle<Value> resourceName,
                           Handle<Integer> resourceLineOffset,
                           Handle<Integer> resourceColumnOffset) :
  mResourceName(resourceName),
  mResourceLineOffset(resourceLineOffset),
  mResourceColumnOffset(resourceColumnOffset)
{
}
Handle<Value> ScriptOrigin::ResourceName() const {
  return mResourceName;
}
Handle<Integer> ScriptOrigin::ResourceLineOffset() const {
  return mResourceLineOffset;
}
Handle<Integer> ScriptOrigin::ResourceColumnOffset() const {
  return mResourceColumnOffset;
}


//////////////////////////////////////////////////////////////////////////////
//// ScriptData class
ScriptData::~ScriptData() {
  if (mScript)
    JS_RemoveObjectRoot(cx(), &mScript);
  if (mXdr)
    JS_XDRDestroy(mXdr);
}

void ScriptData::SerializeScriptObject(JSObject *scriptObj) {
  mXdr = JS_XDRNewMem(cx(), JSXDR_ENCODE);
  if (!mXdr)
    return;

  if (!JS_XDRScriptObject(mXdr, &scriptObj)) {
    JS_XDRDestroy(mXdr);
    mXdr = NULL;
    return;
  }

  uint32 length;
  void *buf = JS_XDRMemGetData(mXdr, &length);
  if (!buf) {
    JS_XDRDestroy(mXdr);
    mXdr = NULL;
    return;
  }

  mData = static_cast<const char*>(buf);
  mLen = length;
  mError = false;
}

ScriptData* ScriptData::PreCompile(const char* input, int length) {
  JSObject *global = JS_GetGlobalObject(cx());
  ScriptData *sd = new ScriptData();
  if (!sd)
    return NULL;

  JSObject *scriptObj = JS_CompileScript(cx(), global,
                                         input, length, NULL, 0);
  if (!scriptObj)
    return sd;

  if (sd)
    sd->SerializeScriptObject(scriptObj);
  return sd;
}

ScriptData* ScriptData::PreCompile(Handle<String> source) {
  JS::Anchor<JSString*> anchor(JSVAL_TO_STRING(source->native()));
  const jschar* chars;
  size_t len;
  chars = JS_GetStringCharsAndLength(cx(),
                                     anchor.get(), &len);
  JSObject *global = JS_GetGlobalObject(cx());
  ScriptData *sd = new ScriptData();
  if (!sd)
    return NULL;

  JSObject *scriptObj = JS_CompileUCScript(cx(), global,
                                           chars, len, NULL, 0);
  if (!scriptObj)
    return sd;

  if (sd)
    sd->SerializeScriptObject(scriptObj);
  return sd;
}

ScriptData* ScriptData::New(const char* aData, int aLength) {
  ScriptData *sd = new ScriptData();
  if (!sd)
    return NULL;

  sd->mScript = sd->GenerateScriptObject((void *)aData, aLength);
  sd->mError = !sd->mScript;

  if (!sd->mScript)
    return sd;

  JS_AddObjectRoot(cx(), &sd->mScript);
  return sd;
}

int ScriptData::Length() {
  if (!mData && mScript)
    SerializeScriptObject(mScript);
  return mLen;
}

const char* ScriptData::Data() {
  if (!mData && mScript)
    SerializeScriptObject(mScript);
  return mData;
}

bool ScriptData::HasError() {
  return mError;
}

JSObject* ScriptData::ScriptObject() {
  return mScript;
}

JSObject* ScriptData::GenerateScriptObject(void *aData, int aLen) {
  mXdr = JS_XDRNewMem(cx(), JSXDR_DECODE);
  if (!mXdr)
    return NULL;

  // Caller reports error via HasError if the script fails to deserialize
  JSErrorReporter older = JS_SetErrorReporter(cx(), NULL);
  JS_XDRMemSetData(mXdr, aData, aLen);

  JSObject *scriptObj;
  JS_XDRScriptObject(mXdr, &scriptObj);

  JS_XDRMemSetData(mXdr, NULL, 0);
  JS_SetErrorReporter(cx(), older);
  JS_XDRDestroy(mXdr);
  mXdr = NULL;

  return scriptObj;
}

//////////////////////////////////////////////////////////////////////////////
//// Script class

JS_STATIC_ASSERT(sizeof(Script) == sizeof(GCReference));

Script::Script(JSObject *s)
{
  mVal = OBJECT_TO_JSVAL(s);
}

Script::operator JSObject *() {
  return JSVAL_TO_OBJECT(mVal);
}

Handle<Object> Script::InternalObject() {
  Object o(JSVAL_TO_OBJECT(mVal));
  return Local<Object>::New(&o);
}

Local<Script> Script::Create(Handle<String> source, ScriptOrigin *origin, ScriptData *preData, Handle<String> scriptData, bool bindToCurrentContext) {
  JSObject* s = NULL;

  if (preData)
    s = preData->ScriptObject();

  if (!s) {
    JS::Anchor<JSString*> anchor(JSVAL_TO_STRING(source->native()));
    const jschar* chars;
    size_t len;
    chars = JS_GetStringCharsAndLength(cx(),
                                       anchor.get(), &len);

    if (origin) {
      String::AsciiValue filename(origin->ResourceName()->ToString());
      uintN lineno = 0;
      Handle<Integer> number = origin->ResourceLineOffset();
      if (!number.IsEmpty()) {
        lineno = number->Int32Value();
      }
      s = JS_CompileUCScript(cx(), NULL,
                             chars, len, *filename, lineno);
    } else {
      s = JS_CompileUCScript(cx(), NULL,
                             chars, len, NULL, 0);
    }
  }

  if (!s) {
    TryCatch::CheckForException();
    return Local<Script>();
  }

  Script script(s);
  if (bindToCurrentContext) {
    script.InternalObject()->Set(String::New("global"), Context::GetCurrent()->Global());
  }
  return Local<Script>::New(&script);
}

Local<Script> Script::New(Handle<String> source, ScriptOrigin *origin,
                          ScriptData *preData, Handle<String> scriptData) {
  return Create(source, origin, preData, scriptData, false);
}

Local<Script> Script::New(Handle<String> source, Handle<Value> fileName) {
  ScriptOrigin origin(fileName);
  return New(source, &origin);
}

Local<Script> Script::Compile(Handle<String> source, ScriptOrigin *origin,
                              ScriptData *preData, Handle<String> scriptData) {
  return Create(source, origin, preData, scriptData, true);
}

Local<Script> Script::Compile(Handle<String> source, Handle<Value> fileName,
                              Handle<String> scriptData) {
  ScriptOrigin origin(fileName);
  return Compile(source, &origin);
}

Local<Value>
Script::Run() {
  Handle<Value> boundGlobalValue = InternalObject()->Get(String::New("global"));
  Handle<Object> global;
  JS_ASSERT(!boundGlobalValue.IsEmpty());
  if (boundGlobalValue->IsUndefined()) {
    global = Context::GetCurrent()->Global();
  }
  else {
    global = boundGlobalValue.As<Object>();
  }
  JS_ASSERT(!global.IsEmpty());

  jsval js_retval;
  if (!JS_ExecuteScript(cx(), **global, *this, &js_retval)) {
    TryCatch::CheckForException();
    return Local<Value>();
  }
  Value v(js_retval);
  return Local<Value>::New(&v);
}

Local<Value> Script::Id() {
  Value v(mVal);
  return Local<Value>::New(&v);
}

//////////////////////////////////////////////////////////////////////////////
//// Message class

JS_STATIC_ASSERT(sizeof(Message) == sizeof(GCReference));

namespace {
  JSClass message_class = {
    "v8::Message", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };
}

Message::Message(const char* message, const char* filename, const char* linebuf, int lineno) :
  SecretObject<internal::GCReference>(JS_NewObject(cx(), &message_class, NULL, NULL))
{
  Object &o = InternalObject();
  o.Set(String::New("message"), String::New(message ? message : ""));
  o.Set(String::New("filename"), String::New(filename ? filename : ""));
  o.Set(String::New("lineNumber"), Integer::New(lineno));
  o.Set(String::New("line"), String::New(linebuf ? linebuf : ""));
}

Local<String> Message::Get() const {
  return InternalObject().Get(String::New("message"))->ToString();
}

Local<String> Message::GetSourceLine() const {
  return InternalObject().Get(String::New("line"))->ToString();
}

Handle<Value> Message::GetScriptResourceName() const {
  return InternalObject().Get(String::New("filename"));
}

Handle<Value> Message::GetScriptData() const {
  UNIMPLEMENTEDAPI(NULL);
}

Handle<StackTrace> Message::GetStackTrace() const {
  UNIMPLEMENTEDAPI(NULL);
}

int Message::GetLineNumber() const {
  return InternalObject().Get(String::New("lineNumber"))->Int32Value();
}

int Message::GetStartPosition() const {
  UNIMPLEMENTEDAPI(0);
}

int Message::GetEndPosition() const {
  UNIMPLEMENTEDAPI(0);
}

int Message::GetStartColumn() const {
  UNIMPLEMENTEDAPI(kNoColumnInfo);
}
int Message::GetEndColumn() const {
  UNIMPLEMENTEDAPI(kNoColumnInfo);
}

void Message::PrintCurrentStackTrace(FILE*) {
  UNIMPLEMENTEDAPI();
}

//////////////////////////////////////////////////////////////////////////////
//// Arguments class

Arguments::Arguments(JSContext* cx,
                     JSObject* thisObj,
                     int nargs,
                     jsval* vp,
                     Handle<Value> data) :
  mCtx(cx),
  mVp(vp),
  mValues(JS_ARGV(cx, vp)),
  mThis(thisObj),
  mLength(nargs),
  mData(Local<Value>::New(*data))
{
}

Local<Value>
Arguments::operator[](int i) const
{
  if (i < 0 || i >= mLength) {
    return Local<Value>::New(Undefined());
  }
  Value v(mValues[i]);
  return Local<Value>::New(&v);
}

Local<Function>
Arguments::Callee() const
{
  jsval callee = JS_CALLEE(mCtx, mVp);
  Function f(JSVAL_TO_OBJECT(callee));
  return Local<Function>::New(&f);
}

bool
Arguments::IsConstructCall() const
{
  return JS_IsConstructing(mCtx, mVp);
}

AccessorInfo::AccessorInfo(Handle<Value> data, JSObject *obj) :
  mData(data), mObj(obj)
{}

Local<Object> AccessorInfo::This() const {
  Object o(mObj);
  return Local<Object>::New(&o);
}

const AccessorInfo
AccessorInfo::MakeAccessorInfo(Handle<Value> data,
                               JSObject* obj)
{
  return AccessorInfo(data, obj);
}

}
