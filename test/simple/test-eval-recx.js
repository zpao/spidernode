require("../common");

debug('evalrecx a string');
var script = process.evalnocx('"passed";');
var result1 = process.evalrecx(script);
var result2 = process.evalrecx(script);
assert.equal('passed', result1);
assert.equal('passed', result2);

debug('evalrecx a thrown error');
script = process.evalnocx('throw new Error("test");');
assert.throws(function() {
  process.evalrecx(script);
});

hello = 5;
script = process.evalnocx('hello = 2');
process.evalrecx(script);
assert.equal(5, hello);


debug("pass values in and out");
code = "foo = 1;"
     + "bar = 2;"
     + "if (baz !== 3) throw new Error('test fail');";
foo = 2;
obj = { foo : 0, baz : 3 };
script = process.evalnocx(code);
var baz = process.evalrecx(script, obj);
assert.equal(1, obj.foo);
assert.equal(2, obj.bar);
assert.equal(2, foo);

debug("call a function by reference");
script = process.evalnocx("f()");
function changeFoo () { foo = 100 }
process.evalrecx(script, { f : changeFoo });
assert.equal(foo, 100);

debug("modify an object by reference");
script = process.evalnocx("f.a = 2");
var f = { a : 1 };
process.evalrecx(script, { f : f });
assert.equal(f.a, 2);

debug("invalid script argument");
assert.throws(function() {
  process.evalrecx('"hello";');
});


