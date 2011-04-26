/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include "v8api_test_harness.h"

////////////////////////////////////////////////////////////////////////////////
//// Helpers

void
fill_string(char* buf,
            char filler,
            int length)
{
  for (int i = 0; i < length; i++) {
    buf[i] = filler;
  }
}

////////////////////////////////////////////////////////////////////////////////
//// Tests

void
test_Length()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a test";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  do_check_eq(str->Length(), TEST_LENGTH);

  // Now, make a string with an embedded NULL.
  TEST_STRING[8] = '\0';
  str = String::New(TEST_STRING, TEST_LENGTH);
  do_check_eq(str->Length(), TEST_LENGTH);

  // Finally, make sure that we end up calling strlen if no length is passed.
  str = String::New(TEST_STRING);

  int strlength = strlen(TEST_STRING);
  do_check_eq(str->Length(), strlength);
}

void
test_Utf8Length()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a UTF-8 test!  This is pi: π";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  do_check_eq(str->Utf8Length(), TEST_LENGTH);

  // Now, make a string with an embedded NULL.
  TEST_STRING[8] = '\0';
  str = String::New(TEST_STRING, TEST_LENGTH);
  do_check_eq(str->Utf8Length(), TEST_LENGTH);
}

void
test_Write()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a UTF-8 test!  This is pi: π";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  char* buf = new char[TEST_LENGTH * 2];

  for (int start = 0; start < TEST_LENGTH; start++) {
    // Fill the buffer with 'a' to ensure there are no NULLs to start with.
    fill_string(buf, 'a', TEST_LENGTH * 2);
    int copied = str->WriteAscii(buf, start, TEST_LENGTH * 2);
    do_check_eq(copied, TEST_LENGTH - start);
    do_check_eq(strlen(buf), TEST_LENGTH - start);
  }

  delete[] buf;
}

void
test_WriteAscii()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a UTF-8 test!  This is pi: π";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  char* buf = new char[TEST_LENGTH * 2];

  // Iterate over the entire string and use it as the start.
  int EXPECTED_LENGTH = TEST_LENGTH - 1; // We should drop the UTF-8 char.
  for (int start = 0; start < TEST_LENGTH; start++) {
    // Fill the buffer with 'a' to ensure there are no NULLs to start with.
    fill_string(buf, 'a', TEST_LENGTH * 2);
    int copied = str->WriteAscii(buf, start, TEST_LENGTH * 2);
    do_check_eq(copied, EXPECTED_LENGTH - start);
    do_check_eq(strlen(buf), EXPECTED_LENGTH - start);
  }

  delete[] buf;
}

void
test_WriteUtf8()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a UTF-8 test!  This is pi: π";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  char* buf = new char[TEST_LENGTH * 2];
  // Fill the buffer with 'a' to ensure there are no NULLs to start with.
  fill_string(buf, 'a', TEST_LENGTH * 2);
  int charsWritten;
  int copied = str->WriteUtf8(buf, TEST_LENGTH * 2, &charsWritten);
  do_check_eq(copied, TEST_LENGTH + 1);
  // π is 2 bytes, so strlen (TEST_LENGTH) returns 1 larger than charsWritten
  do_check_eq(charsWritten, TEST_LENGTH - 1);
  do_check_eq(strlen(buf), TEST_LENGTH);

  delete[] buf;
}

void
test_AsciiValue_operators()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "ascii string value";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  do_check_eq(str->Length(), TEST_LENGTH);
  String::AsciiValue asciiString(str);
  const char* one = *asciiString;
  char* two = *asciiString;
  do_check_true(0 == strcmp(TEST_STRING, one));
  do_check_true(0 == strcmp(TEST_STRING, two));
}

void
test_AsciiValue_length()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "toString";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  do_check_eq(str->Length(), TEST_LENGTH);
  String::AsciiValue k(str);
  do_check_eq(k.length(), TEST_LENGTH);
}

void
test_Utf8Value_length()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  char TEST_STRING[] = "this is a UTF-8 test!  This is pi: π";
  int TEST_LENGTH = strlen(TEST_STRING);
  Handle<String> str = String::New(TEST_STRING);
  do_check_eq(str->Length(), TEST_LENGTH - 1);
  String::Utf8Value k(str);
  do_check_eq(k.length(), TEST_LENGTH);
  do_check_true(0 == strcmp(TEST_STRING, *k));
}
////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_Length),
  TEST(test_Utf8Length),
  DISABLED_TEST(test_Write, 13),
  DISABLED_TEST(test_WriteAscii, 14),
  TEST(test_WriteUtf8),
  TEST(test_AsciiValue_operators),
  TEST(test_AsciiValue_length),
  TEST(test_Utf8Value_length),
};

const char* file = __FILE__;
#define TEST_NAME "String Classes"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
