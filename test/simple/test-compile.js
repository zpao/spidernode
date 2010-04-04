require("../common");

debug('compile a string');
var result = process.compile('"passed";');
assert.equal('passed', result);

debug('compile a thrown error');
assert.throws(function() {
  process.compile('throw new Error("test");');
});

hello = 5;
process.compile('hello = 2');
assert.equal(2, hello);


debug("pass values");
code = "foo = 1;"
     + "bar = 2;"
     + "if (typeof baz !== 'undefined') throw new Error('test fail');";
foo = 2;
obj = { foo : 0, baz : 3 };
var baz = process.compile(code);
assert.equal(0, obj.foo);
assert.equal(2, bar);
assert.equal(1, foo);

debug("call a function");
f = function () { foo = 100 };
process.compile("f()");
assert.equal(100, foo);

