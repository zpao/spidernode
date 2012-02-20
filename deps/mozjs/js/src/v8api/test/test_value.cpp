/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include "v8api_test_harness.h"

static inline Local<Value> CompileRun(const char* source) {
  return Script::Compile(String::New(source))->Run();
}

////////////////////////////////////////////////////////////////////////////////
//// Tests

/*
 * Most of Value's methods should already be (either implicitly or
 * explicitly) tested in test_api. The one's that aren't are covered here.
 */

void
test_IntegerConversion()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  // Convert string to Integer.
  Handle<Value> result = CompileRun("'123'");
  do_check_eq(result->IntegerValue(), 123);
  do_check_eq(result->ToInteger()->Value(), 123);

  // Convert integer number to Integer.
  result = CompileRun("123");
  do_check_eq(result->IntegerValue(), 123);
  do_check_eq(result->ToInteger()->Value(), 123);

  // Convert decimal number to Integer.
  result = CompileRun("123.45");
  do_check_eq(result->IntegerValue(), 123);
  do_check_eq(result->ToInteger()->Value(), 123);

  // Convert negative number to Integer.
  result = CompileRun("-123");
  do_check_eq(result->IntegerValue(), -123);
  do_check_eq(result->ToInteger()->Value(), -123);

/* TODO: we don't support 64 bit numbers yet

  // Convert huge number to Integer.
  result = CompileRun("0x7fffffffffffffff");
  do_check_eq(result->IntegerValue(), (int64_t)0x7fffffffffffffff);
  do_check_eq(result->ToInteger()->Value(), (int64_t)0x7fffffffffffffff);
*/

  // Convert NaN to Integer.
  result = CompileRun("NaN");
  do_check_eq(result->IntegerValue(), 0);
  do_check_eq(result->ToInteger()->Value(), 0);
}

void
test_BooleanConversion()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  // Convert an a truthy value, e.g. a string, to a boolean.
  Handle<Value> result = CompileRun("'123'");
  do_check_true(result->BooleanValue());
  do_check_true(result->ToBoolean()->Value());

  // Convert an a truthy value, e.g. a string, to a boolean.
  result = CompileRun("0");
  do_check_false(result->BooleanValue());
  do_check_false(result->ToBoolean()->Value());
}

void
test_IsFunction()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  // Check something that isn't a function, e.g. a string.
  Handle<Value> result = CompileRun("'123'");
  do_check_false(result->IsFunction());

  // Check function.
  result = CompileRun("function () {}");
  do_check_true(result->IsFunction());
}

void
test_IsArray()
{
  HandleScope handle_scope;
  Persistent<Context> context = Context::New();
  Context::Scope context_scope(context);

  // Check something that isn't a function, e.g. a string.
  Handle<Value> result = CompileRun("{}");
  do_check_false(result->IsArray());

  // Check function.
  result = CompileRun("[]");
  do_check_true(result->IsArray());
}

////////////////////////////////////////////////////////////////////////////////
//// Test Harness

Test gTests[] = {
  TEST(test_IntegerConversion),
  TEST(test_BooleanConversion),
  DISABLED_TEST(test_IsFunction, 102),
  TEST(test_IsArray),
};

const char* file = __FILE__;
#define TEST_NAME "Value Class"
#define TEST_FILE file
#include "v8api_test_harness_tail.h"
