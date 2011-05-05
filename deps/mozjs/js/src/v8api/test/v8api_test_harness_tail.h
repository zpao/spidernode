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

#ifndef TEST_NAME
#error "Must #define TEST_NAME before including v8api_test_harness_tail.h"
#endif

#ifndef TEST_FILE
#error "Must #define TEST_FILE before include v8api_test_harness_tail.h"
#endif

#define TEST_INFO_STR "TEST-INFO | (%s) | "

int
main(int aArgc,
     char** aArgv)
{
  do_check_true(V8::Initialize());
  (void)printf("Running %s tests...\n", TEST_NAME);

  for (size_t i = 0; i < (sizeof(gTests) / sizeof(gTests[0])); i++) {
    Test &test = gTests[i];
    if (!test.disabled) {
      (void)printf(TEST_INFO_STR "Running %s.\n", TEST_FILE, test.name);
      TryCatch exceptionHandler;
      test.func();
      do_check_false(exceptionHandler.HasCaught());
    }
    else if (test.issue >= 0) {
      do_check_neq(test.issue, 0);
      (void)printf(TEST_INFO_STR "TODO %s is DISABLED.  Tracked in %s%u\n",
                   TEST_FILE, test.name,
                   "https://github.com/zpao/v8monkey/issues/", test.issue);
    }
  }
  do_check_true(V8::Dispose());

  // Check that we have passed all of our tests, and output accordingly.
  if (gPassedTests == gTotalTests) {
    passed(TEST_FILE);
  }

  (void)printf(TEST_INFO_STR  "%u of %u tests passed\n", TEST_FILE,
               unsigned(gPassedTests), unsigned(gTotalTests));

  return gPassedTests == gTotalTests ? 0 : -1;
}
