#!/usr/bin/env python

# For now this file won't actually do any importing.
# This will read through V8's test-api.cc and output the corresponding test
# name for test_api.cpp. It will be formatted so that it can be pasted into
# test_api.cpp but without any function body. For example:
#
# void test_GlobalPrototype() { }
#
# When that test gets implemented, that line that was pasted in will need to
# be reformatted to match the style in the file. There is also the manual step
# of finding the right place to paste each chunk.

# Future ideas:
#  - Compare to tests in test_api.cpp
#    + determine if new tests should be added
#    + determine if existing tests are modified
#  - Modify test_api.cpp to do the manual steps
#    + put stubs in the right place
#    + add the tests to gTests
#    + add actual test, not just stubs
#    + test compilation of added tests, comment out if compilation fails


import re

# For now just assume we'll be reading from test-api.cc
src = "./test-api.cc"
srcfile = open(src, "r")

# Compile the regex since we'll be using it a lot.
test_re = re.compile("^(THREADED_)?TEST\((.+)\)")

out_stubs = ""
out_gTests = "Test gTests[] = {\n"

linenum = 0
for line in srcfile:
  linenum += 1
  match = test_re.match(line)
  if match is not None:
    out_stubs += "// from test-api.cc:" + str(linenum)
    out_stubs += "\nvoid\ntest_" + match.group(2) + "()\n{ }\n\n"
    out_gTests += "  UNIMPLEMENTED_TEST(test_" + match.group(2) + "),\n"

out_gTests += "};"

print "////////////////////////////////////////////////////////////////////////////////"
print "//// Tests\n"
print out_stubs
print "////////////////////////////////////////////////////////////////////////////////"
print "//// Test Harness\n"
print out_gTests

srcfile.close()
