#include "v8-internal.h"

namespace v8 {
using namespace internal;

FunctionTemplate::FunctionTemplate() :
  Template(NULL)
{
}

// static
Local<FunctionTemplate>
FunctionTemplate::New(InvocationCallback callback,
                      Handle<Value> data,
                      Handle<Signature>)
{
  if (callback) {
    UNIMPLEMENTEDAPI(NULL);
  }
  FunctionTemplate ft;
  return Local<FunctionTemplate>::New(&ft);
}

Local<Function>
FunctionTemplate::GetFunction()
{
  UNIMPLEMENTEDAPI(NULL);
}

void
FunctionTemplate::SetCallHandler(InvocationCallback callback,
                                 Handle<Value> data)
{
  UNIMPLEMENTEDAPI();
}

Local<ObjectTemplate>
FunctionTemplate::InstanceTemplate()
{
  UNIMPLEMENTEDAPI(NULL);
}

void
FunctionTemplate::Inherit(Handle<FunctionTemplate> parent)
{
  UNIMPLEMENTEDAPI();
}

Local<ObjectTemplate>
FunctionTemplate::PrototypeTemplate()
{
  UNIMPLEMENTEDAPI(NULL);
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
FunctionTemplate::HasInstance(Handle<Value> object)
{
  UNIMPLEMENTEDAPI(false);
}

} // namespace v8
