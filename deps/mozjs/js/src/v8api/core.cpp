#include "v8-internal.h"

namespace v8 { namespace internal {

const int KB = 1024;
const int MB = 1024 * 1024;
JSRuntime *gRuntime = 0;
JSRuntime *rt() {
  if (!gRuntime) {
    V8::Initialize();
  }
  return gRuntime;
}

static JSContext *gRootContext = 0;
JSContext *cx() {
  if (!gRootContext) {
    V8::Initialize();
  }
  return gRootContext;
}

JSClass global_class = {
  "global", JSCLASS_GLOBAL_FLAGS,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

void notImplemented() {
  fprintf(stderr, "Calling an unimplemented API!\n");
}

}

using namespace internal;

bool V8::Initialize() {
  JS_SetCStringsAreUTF8();
  gRuntime = JS_NewRuntime(64 * MB);
  if(!gRuntime)
    return false;

  JSContext *ctx(JS_NewContext(gRuntime, 8192));
  if (!ctx)
    return false;
  // TODO: look into JSOPTION_NO_SCRIPT_RVAL
  JS_SetOptions(ctx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT | JSOPTION_DONT_REPORT_UNCAUGHT);
  JS_SetVersion(ctx, JSVERSION_LATEST);
  JS_SetErrorReporter(ctx, TryCatch::ReportError);

  JS_BeginRequest(ctx);

  gRootContext = ctx;
  return true;
}

// TODO: call this
bool V8::Dispose() {
  JS_EndRequest(gRootContext);
  if (gRuntime)
    JS_DestroyRuntime(gRuntime);
  JS_ShutDown();
  return true;
}

Handle<Value> V8::ThrowException(Handle<Value> exception) {
  jsval native = exception->native();
  JS_SetPendingException(cx(), native);
  JS_ReportPendingException(cx());

  return Undefined();
}

bool V8::IdleNotification() {
  // XXX Returning true here will tell Node to stop running the GC timer, so
  //     we're just going to do that for now.
  return true;
}

static Local<Value> ConstructError(const char *name, Handle<String> message) {
  // XXX: this probably isn't correct in all cases
  Handle<Context> ctx = Context::GetCurrent();
  Handle<Object> global = ctx->Global();
  Handle<Value> ctor = global->Get(String::New(name));
  if (ctor.IsEmpty() || !ctor->IsFunction())
    return Local<Value>();
  Handle<Function> fn = ctor.As<Function>();
  Handle<Value> args[] = { Handle<Value>(message) };
  return Local<Value>::New(fn->NewInstance(1, args));
}

Local<Value> Exception::RangeError(Handle<String> message) {
  return ConstructError("RangeError", message);
}

Local<Value> Exception::ReferenceError(Handle<String> message) {
  return ConstructError("ReferenceError", message);
}

Local<Value> Exception::SyntaxError(Handle<String> message) {
  return ConstructError("SyntaxError", message);
}

Local<Value> Exception::TypeError(Handle<String> message) {
  return ConstructError("TypeError", message);
}

Local<Value> Exception::Error(Handle<String> message) {
  return ConstructError("Error", message);
}

}
