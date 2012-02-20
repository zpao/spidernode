/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 et lcs=trail\:.,tab\:>~ :
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is v8api test code.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Shawn Wilsher <me@shawnwilsher.com> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdarg.h>
#include <sstream>
#include "jstypes.h"

#include "../v8-internal.h"
using namespace v8;
namespace i = ::i;

static size_t gTotalTests = 0;
static size_t gPassedTests = 0;

#define do_check_true(aCondition) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    if (aCondition) { \
      gPassedTests++; \
    } else { \
      fail("%s | Expected true, got false at line %d", __FILE__, __LINE__); \
    } \
  JS_END_MACRO

#define do_check_false(aCondition) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    if (!aCondition) { \
      gPassedTests++; \
    } else { \
      fail("%s | Expected false, got true at line %d", __FILE__, __LINE__); \
    } \
  JS_END_MACRO

#define do_check_success(aResult) \
  do_check_true(NS_SUCCEEDED(aResult))

#ifdef DEBUG
// DEBUG bulids use a struct, but OPT builds are just 64-bit unsigned numbers.
std::ostream&
operator<<(std::ostream& o,
           const jsval& val)
{
  o << val;
  // o << val.asBits; TODO: find replacement for asBits
  return o;
}
#endif

std::ostream&
operator<<(std::ostream& o,
           const Handle<Value>& val)
{
  if (val.IsEmpty()) {
    o << "<empty handle>";
  } else {
    Local<String> str = val->ToString();
    int len = str->Length();
    char* asciiStr = new char[len + 1];
    (void)str->WriteAscii(asciiStr);
    o << asciiStr;
    delete[] asciiStr;
  }
  return o;
}

#define do_check_eq(aActual, aExpected) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    if (aExpected == aActual) { \
      gPassedTests++; \
    } else { \
      std::ostringstream temp; \
      temp << __FILE__ << " | Expected '" << aExpected << "', got '"; \
      temp << aActual <<"' at line " << __LINE__; \
      fail(temp.str().c_str()); \
    } \
  JS_END_MACRO

#define do_check_neq(aActual, aExpected) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    if (aExpected != aActual) { \
      gPassedTests++; \
    } else { \
      std::ostringstream temp; \
      temp << __FILE__ << " | Expected '" << aExpected << "' to not be '"; \
      temp << aActual <<"' at line " << __LINE__; \
      fail(temp.str().c_str()); \
    } \
  JS_END_MACRO

bool
operator==(const Handle<Value>& a,
           const Handle<Value>& b)
{
  return a->StrictEquals(b);
}

/**
 * Prints the given failure message and arguments using printf, prepending
 * "TEST-UNEXPECTED-FAIL " for the benefit of the test harness and
 * appending "\n" to eliminate having to type it at each call site.
 */
void fail(const char* msg, ...)
{
  va_list ap;

  printf("TEST-UNEXPECTED-FAIL | ");

  va_start(ap, msg);
  vprintf(msg, ap);
  va_end(ap);

  putchar('\n');
}

/**
 * Prints the given string prepending "TEST-PASS | " for the benefit of
 * the test harness and with "\n" at the end, to be used at the end of a
 * successful test function.
 */
void passed(const char* test)
{
  printf("TEST-PASS | %s\n", test);
}

struct Test
{
  void (*func)(void);
  const char* const name;
  bool disabled;
  bool throws;
  int issue;
};
#define TEST(aName) \
  {aName, #aName, false, false, 0}
#define TEST_THROWS(aName) \
  {aName, #aName, false, true, 0}
#define DISABLED_TEST(aName, aIssueNumber) \
  {aName, #aName, true, false, aIssueNumber}
#define UNIMPLEMENTED_TEST(aName) \
  {aName, #aName, true, false, -1}
#define UNWANTED_TEST(aName) UNIMPLEMENTED_TEST(aName)
