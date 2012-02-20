/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

// ints
static inline void check_eq_helper(const char* aFile, int aLine,
                                   int aExpected,
                                   int aActual)
{
  if (aExpected == aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}
static inline void check_ne_helper(const char* aFile, int aLine,
                                   int aExpected,
                                   int aActual)
{
  if (aExpected != aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// int64_ts
static inline void check_eq_helper(const char* aFile, int aLine,
                                   int64_t aExpected,
                                   int64_t aActual)
{
  if (aExpected == aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}
static inline void check_ne_helper(const char* aFile, int aLine,
                                   int64_t aExpected,
                                   int64_t aActual)
{
  if (aExpected != aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// doubles
static inline void check_eq_helper(const char* aFile, int aLine,
                                   double aExpected,
                                   double aActual)
{
  if (aExpected == aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}
static inline void check_ne_helper(const char* aFile, int aLine,
                                   double aExpected,
                                   double aActual)
{
  if (aExpected != aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// bools
static inline void check_eq_helper(const char* aFile, int aLine,
                                   bool aExpected,
                                   bool aActual)
{
  if (aExpected == aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}
static inline void check_ne_helper(const char* aFile, int aLine,
                                   bool aExpected,
                                   bool aActual)
{
  if (aExpected != aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// char*s
static inline void check_eq_helper(const char* aFile, int aLine,
                                   const char* aExpected,
                                   const char* aActual)
{
  if ((aExpected == NULL && aActual == NULL) ||
      (aExpected != NULL && aActual != NULL &&
       strcmp(aExpected, aActual) == 0)) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}
static inline void check_ne_helper(const char* aFile, int aLine,
                                   const char* aExpected,
                                   const char* aActual)
{
  if (!((aExpected == NULL && aActual == NULL) ||
       (aExpected != NULL && aActual != NULL &&
        strcmp(aExpected, aActual) == 0))) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// Handle<Value>s
void check_eq_helper(const char* aFile, int aLine,
                     v8::Handle<v8::Value> aExpected,
                     v8::Handle<v8::Value> aActual)
{
  bool areEqual;
  if (aExpected.IsEmpty()) {
    areEqual = aActual.IsEmpty();
  }
  else {
    areEqual = !aActual.IsEmpty() && aExpected->Equals(aActual);
  }
  if (areEqual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected '" << aExpected << "', got '";
    temp << aActual << "' at line " << aLine;
    fail(temp.str().c_str());
  }
}

void check_ne_helper(const char* aFile, int aLine,
                     v8::Handle<v8::Value> aExpected,
                     v8::Handle<v8::Value> aActual)
{
  bool areEqual;
  if (aExpected.IsEmpty()) {
    areEqual = aActual.IsEmpty();
  }
  else {
    areEqual = !aActual.IsEmpty() && aExpected->Equals(aActual);
  }
  if (!areEqual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}

// Some tests use raw pointers...
void check_ne_helper(const char* aFile, int aLine,
                     intptr_t aExpected,
                     v8::Value* aActual)
{
  int64_t actual = reinterpret_cast<intptr_t>(aActual);
  int64_t expected = aExpected;
  check_ne_helper(aFile, aLine, expected, actual);
}

// mixed
static inline void check_ne_helper(const char* aFile, int aLine,
                                   int aExpected,
                                   bool aActual)
{
  if (aExpected == 0 && aActual ||
      aExpected != 0 && !aActual) {
    gPassedTests++;
  }
  else {
    std::ostringstream temp;
    temp << aFile << " | Expected and Actual are equal:  '" << aExpected << "' ";
    temp << " at line " << aLine;
    fail(temp.str().c_str());
  }
}


#define CHECK do_check_true
#define CHECK_EQ(expected, actual) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    check_eq_helper(__FILE__, __LINE__, expected, actual); \
  JS_END_MACRO
#define CHECK_NE(expected, actual) \
  JS_BEGIN_MACRO \
    gTotalTests++; \
    check_ne_helper(__FILE__, __LINE__, expected, actual); \
  JS_END_MACRO
