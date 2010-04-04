require("../common");

debug('evalhere a string');
var script = process.evalnocx('"passed";');
var result = process.evalhere(script);
assert.equal('passed', result);

debug('compile a thrown error');
script = process.evalnocx('throw new Error("test");');
assert.throws(function() {
  process.evalhere(script);
});

hello = 5;
script = process.evalnocx('hello = 2');
process.evalhere(script);
assert.equal(2, hello);


debug("pass values");
code = "foo = 1;"
     + "bar = 2;"
     + "if (typeof baz !== 'undefined') throw new Error('test fail');";
foo = 2;
obj = { foo : 0, baz : 3 };
script = process.evalnocx(code);
process.evalhere(script);
assert.equal(0, obj.foo);
assert.equal(2, bar);
assert.equal(1, foo);

debug("call a function");
f = function () { foo = 100 };
script = process.evalnocx("f()");
process.evalhere(script);
assert.equal(100, foo);

