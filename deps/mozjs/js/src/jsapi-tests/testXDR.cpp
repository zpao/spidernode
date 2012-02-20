/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=99:
 */

#include "tests.h"
#include "jsscript.h"
#include "jsxdrapi.h"

BEGIN_TEST(testXDR_bug506491)
{
    const char *s =
        "function makeClosure(s, name, value) {\n"
        "    eval(s);\n"
        "    return let (n = name, v = value) function () { return String(v); };\n"
        "}\n"
        "var f = makeClosure('0;', 'status', 'ok');\n";

    // compile
    JSScript *script = JS_CompileScript(cx, global, s, strlen(s), __FILE__, __LINE__);
    CHECK(script);

    // freeze
    JSXDRState *w = JS_XDRNewMem(cx, JSXDR_ENCODE);
    CHECK(w);
    CHECK(JS_XDRScript(w, &script));
    uint32_t nbytes;
    void *p = JS_XDRMemGetData(w, &nbytes);
    CHECK(p);
    void *frozen = JS_malloc(cx, nbytes);
    CHECK(frozen);
    js_memcpy(frozen, p, nbytes);
    JS_XDRDestroy(w);

    // thaw
    script = NULL;
    JSXDRState *r = JS_XDRNewMem(cx, JSXDR_DECODE);
    JS_XDRMemSetData(r, frozen, nbytes);
    CHECK(JS_XDRScript(r, &script));
    JS_XDRDestroy(r);  // this frees `frozen`

    // execute
    jsvalRoot v2(cx);
    CHECK(JS_ExecuteScript(cx, global, script, v2.addr()));

    // try to break the Block object that is the parent of f
    JS_GC(cx);

    // confirm
    EVAL("f() === 'ok';\n", v2.addr());
    jsvalRoot trueval(cx, JSVAL_TRUE);
    CHECK_SAME(v2, trueval);
    return true;
}
END_TEST(testXDR_bug506491)

BEGIN_TEST(testXDR_bug516827)
{
    // compile an empty script
    JSScript *script = JS_CompileScript(cx, global, "", 0, __FILE__, __LINE__);
    CHECK(script);

    // freeze
    JSXDRState *w = JS_XDRNewMem(cx, JSXDR_ENCODE);
    CHECK(w);
    CHECK(JS_XDRScript(w, &script));
    uint32_t nbytes;
    void *p = JS_XDRMemGetData(w, &nbytes);
    CHECK(p);
    void *frozen = JS_malloc(cx, nbytes);
    CHECK(frozen);
    js_memcpy(frozen, p, nbytes);
    JS_XDRDestroy(w);

    // thaw
    script = NULL;
    JSXDRState *r = JS_XDRNewMem(cx, JSXDR_DECODE);
    JS_XDRMemSetData(r, frozen, nbytes);
    CHECK(JS_XDRScript(r, &script));
    JS_XDRDestroy(r);  // this frees `frozen`

    // execute with null result meaning no result wanted
    CHECK(JS_ExecuteScript(cx, global, script, NULL));
    return true;
}
END_TEST(testXDR_bug516827)
