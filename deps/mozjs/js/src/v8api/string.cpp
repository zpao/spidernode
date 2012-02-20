#include "v8-internal.h"
#include <algorithm>

namespace v8 {
using namespace internal;

JS_STATIC_ASSERT(sizeof(String) == sizeof(GCReference));

Local<String>
String::New(const char* data,
            int length)
{
  JSString* str;
  if (length == -1) {
    str = JS_NewStringCopyZ(cx(), data);
  }
  else {
    str = JS_NewStringCopyN(cx(), data, length);
  }
  if (!str) {
    return Local<String>();
  }
  String s(str);
  return Local<String>::New(&s);
}

// static
Local<String>
String::New(const uint16_t* data,
            int length)
{
  JSString* str;
  if (length == -1) {
    str = JS_NewUCStringCopyZ(cx(), data);
  }
  else {
    str = JS_NewUCStringCopyN(cx(), data, length);
  }
  if (!str) {
    return Local<String>();
  }
  String s(str);
  return Local<String>::New(&s);
}

// static
Local<String>
String::NewSymbol(const char* data,
                  int length)
{
  JSString* str;
  if (length == -1) {
    str = JS_NewStringCopyZ(cx(), data);
  }
  else {
    str = JS_NewStringCopyN(cx(), data, length);
  }
  // jsids are atomized, so we create one in a roundabout way.
  jsid id;
  (void)JS_ValueToId(cx(), STRING_TO_JSVAL(str), &id);
  return String::FromJSID(id);
}

// static
Local<String>
String::Concat(Handle<String> left,
               Handle<String> right)
{
  JSString* str = JS_ConcatStrings(cx(), **left, **right);
  String s(str);
  return Local<String>::New(&s);
}

// static
Local<String>
String::FromJSID(jsid id)
{
  jsval v;
  if (!JS_IdToValue(cx(), id, &v)) {
    return Local<String>();
  }
  JSString* str = JS_ValueToString(cx(), v);
  if (!str) {
    return Local<String>();
  }
  String s(str);
  return Local<String>::New(&s);
}

// static
Local<String>
String::NewExternal(ExternalAsciiStringResource* external)
{

  // TODO (Issue #58) Do not copy the string here!
  Local<String> str = String::New(external->data(), external->length());
  external->Dispose();
  return Local<String>::New(str);
}

int
String::Length() const
{
  return JS_GetStringLength(*this);
}

int
String::Utf8Length() const
{
  size_t encodedLength = JS_GetStringEncodingLength(cx(),
                                                    *this);
  return static_cast<int>(encodedLength);
}

int
String::Write(uint16_t* buffer,
              int start,
              int length,
              WriteHints hints) const
{
  size_t internalLen;
  const jschar* chars =
    JS_GetStringCharsZAndLength(cx(), *this, &internalLen);
  if (!chars || internalLen < static_cast<size_t>(start)) {
    return 0;
  }
  size_t bytes = std::min<size_t>(length, internalLen - start) * sizeof(uint16_t);
  if (length == -1) {
    bytes = (internalLen - start) * sizeof(uint16_t);
  }
  (void)memcpy(buffer, &chars[start], bytes);

  return bytes / 2;
}

int
String::WriteAscii(char* buffer,
                   int start,
                   int length,
                   WriteHints hints) const
{
  size_t encodedLength = JS_GetStringEncodingLength(cx(),
                                                    *this);
  char* tmp = array_new<char>(encodedLength);
  int written = WriteUtf8(tmp, encodedLength);

  // No easy way to convert UTF-8 to ASCII, so just drop characters that are
  // not ASCII.
  int effectiveLength = length;
  if (effectiveLength == -1) {
    // Assume enough storage to fit the string + null
    effectiveLength = written + 1;
  }
  int end = start + encodedLength;
  // [start,end) is the range to read from
  // [0, effectiveLength) is the range to write to
  int i = 0;
  for (int idx = start; i < effectiveLength && idx < end; i++, idx++) {
    unsigned char first = static_cast<unsigned char>(tmp[idx]);
    if (first > 0x7F) {
      if (first < 0xE0) {
        char second = tmp[++idx] & ~0x80;
        unsigned int ch = (static_cast<unsigned int>(first & ~0xC0) << 6) | second;
        if (ch == (0xff & ch)) {
          buffer[i] = ch;
          continue;
        }
        // Doesn't fit into a single byte, fall through
      }
      i--;
      continue;
    } else {
      buffer[i] = tmp[idx];
    }
  }
  // If we have enough space for the NULL terminator, set it.
  if (i < effectiveLength) {
    buffer[i] = '\0';
  }

  array_delete(tmp);
  return i;
}

int
String::WriteUtf8(char* buffer,
                  int length,
                  int* nchars_ref,
                  WriteHints hints) const
{
  // TODO handle -1 for length!
  size_t bytesWritten = JS_EncodeStringToBuffer(*this, buffer, length);

  // If we have enough space for the NULL terminator, set it.
  if (bytesWritten < size_t(length)) {
    buffer[bytesWritten++] = '\0';
  }

  // TODO check to make sure we don't overflow int
  if (nchars_ref) {
    // XXX This works when the whole string fits in length, but not if it's less
    *nchars_ref = this->Length();
  }
  return static_cast<int>(bytesWritten);
}

// static
Local<String>
String::Empty()
{
  return String::New("", 0);
}

String::AsciiValue::AsciiValue(Handle<v8::Value> val)
{
  // Set some defaults which will be used for empty values/strings
  mStr = NULL;
  mLength = 0;

  if (val.IsEmpty()) {
    return;
  }

  // TODO: Do we need something about HandleScope here?
  Local<String> str = val->ToString();
  if (!str.IsEmpty()) {
    int len = str->Length();
    // Need space for the NULL terminator.
    mStr = array_new<char>(len + 1);
    mLength = str->WriteAscii(mStr, 0, len + 1);
  }
}
String::AsciiValue::~AsciiValue()
{
  if (mStr)
    array_delete(mStr);
}

String::Utf8Value::Utf8Value(Handle<v8::Value> val)
{
  // Set some defaults which will be used for empty values/strings
  mStr = NULL;
  mLength = 0;

  if (val.IsEmpty()) {
    return;
  }

  // TODO: Do we need something about HandleScope here?
  Local<String> str = val->ToString();
  if (!str.IsEmpty()) {
    int len = str->Utf8Length();
    // Need space for the NULL terminator.
    mStr = array_new<char>(len + 1);
    mLength = str->WriteUtf8(mStr, len + 1) - 1;
  }
}
String::Utf8Value::~Utf8Value()
{
  if (mStr) {
    array_delete(mStr);
  }
}

} // namespace v8
