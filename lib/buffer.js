// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var SlowBuffer = process.binding('buffer').SlowBuffer;


function toHex(n) {
  if (n < 16) return '0' + n.toString(16);
  return n.toString(16);
}


SlowBuffer.prototype.inspect = function() {
  var out = [],
      len = this.length;
  for (var i = 0; i < len; i++) {
    out[i] = toHex(this[i]);
  }
  return '<SlowBuffer ' + out.join(' ') + '>';
};


SlowBuffer.prototype.hexSlice = function(start, end) {
  var len = this.length;

  if (!start || start < 0) start = 0;
  if (!end || end < 0 || end > len) end = len;

  var out = '';
  for (var i = start; i < end; i ++) {
    out += toHex(this[i]);
  }
  return out;
};



SlowBuffer.prototype.toString = function(encoding, start, end) {
  encoding = String(encoding || 'utf8').toLowerCase();
  start = +start || 0;
  if (typeof end == 'undefined') end = this.length;

  // Fastpath empty strings
  if (+end == start) {
    return '';
  }

  switch (encoding) {
    case 'hex':
      return this.hexSlice(start, end);

    case 'utf8':
    case 'utf-8':
      return this.utf8Slice(start, end);

    case 'ascii':
      return this.asciiSlice(start, end);

    case 'binary':
      return this.binarySlice(start, end);

    case 'base64':
      return this.base64Slice(start, end);

    case 'ucs2':
    case 'ucs-2':
      return this.ucs2Slice(start, end);

    default:
      throw new Error('Unknown encoding');
  }
};


SlowBuffer.prototype.hexWrite = function(string, offset) {
  var len = string.length;
  offset = +offset || 0;

  // must be an even number of digits
  if (len % 2) {
    throw new Error('Invalid hex string');
  }
  for (var i = 0; i < len / 2; i ++) {
    var byte = parseInt(string.substr(i * 2, 2), 16);
    if (isNaN(byte)) throw new Error('Invalid hex string');
    this[offset + i] = byte;
  }
  return i;
}


SlowBuffer.prototype.write = function(string, offset, encoding) {
  // Support both (string, offset, encoding)
  // and the legacy (string, encoding, offset)
  if (!isFinite(offset)) {
    var swap = encoding;
    encoding = offset;
    offset = swap;
  }

  offset = +offset || 0;
  encoding = String(encoding || 'utf8').toLowerCase();

  switch (encoding) {
    case 'hex':
      return this.hexWrite(string, offset);

    case 'utf8':
    case 'utf-8':
      return this.utf8Write(string, offset);

    case 'ascii':
      return this.asciiWrite(string, offset);

    case 'binary':
      return this.binaryWrite(string, offset);

    case 'base64':
      return this.base64Write(string, offset);

    case 'ucs2':
    case 'ucs-2':
      return this.ucs2Write(start, end);

    default:
      throw new Error('Unknown encoding');
  }
};


// slice(start, end)
SlowBuffer.prototype.slice = function(start, end) {
  if (end > this.length) {
    throw new Error('oob');
  }
  if (start > end) {
    throw new Error('oob');
  }

  return new Buffer(this, end - start, +start);
};


// Buffer

function Buffer(subject, encoding, offset) {
  if (!(this instanceof Buffer)) {
    return new Buffer(subject, encoding, offset);
  }

  var type;

  // Are we slicing?
  if (typeof offset === 'number') {
    this.length = encoding;
    this.parent = subject;
    this.offset = offset;
  } else {
    // Find the length
    switch (type = typeof subject) {
      case 'number':
        this.length = subject;
        break;

      case 'string':
        this.length = Buffer.byteLength(subject, encoding);
        break;

      case 'object': // Assume object is an array
        this.length = subject.length;
        break;

      default:
        throw new Error('First argument needs to be a number, ' +
                        'array or string.');
    }

    if (this.length > Buffer.poolSize) {
      // Big buffer, just alloc one.
      this.parent = new SlowBuffer(this.length);
      this.offset = 0;

    } else {
      // Small buffer.
      if (!pool || pool.length - pool.used < this.length) allocPool();
      this.parent = pool;
      this.offset = pool.used;
      pool.used += this.length;
    }

    // Treat array-ish objects as a byte array.
    if (isArrayIsh(subject)) {
      for (var i = 0; i < this.length; i++) {
        this.parent[i + this.offset] = subject[i];
      }
    } else if (type == 'string') {
      // We are a string
      this.length = this.write(subject, 0, encoding);
    }
  }

  SlowBuffer.makeFastBuffer(this.parent, this, this.offset, this.length);
}

function isArrayIsh(subject) {
  return Array.isArray(subject) || Buffer.isBuffer(subject) ||
         subject && typeof subject === 'object' &&
         typeof subject.length === 'number';
}

exports.SlowBuffer = SlowBuffer;
exports.Buffer = Buffer;

Buffer.poolSize = 8 * 1024;
var pool;

function allocPool() {
  pool = new SlowBuffer(Buffer.poolSize);
  pool.used = 0;
}


// Static methods
Buffer.isBuffer = function isBuffer(b) {
  return b instanceof Buffer || b instanceof SlowBuffer;
};


// Inspect
Buffer.prototype.inspect = function inspect() {
  var out = [],
      len = this.length;
  for (var i = 0; i < len; i++) {
    out[i] = toHex(this.parent[i + this.offset]);
  }
  return '<Buffer ' + out.join(' ') + '>';
};


Buffer.prototype.get = function get(i) {
  if (i < 0 || i >= this.length) throw new Error('oob');
  return this.parent[this.offset + i];
};


Buffer.prototype.set = function set(i, v) {
  if (i < 0 || i >= this.length) throw new Error('oob');
  return this.parent[this.offset + i] = v;
};


// write(string, offset = 0, encoding = 'utf8')
Buffer.prototype.write = function(string, offset, encoding) {
  if (!isFinite(offset)) {
    var swap = encoding;
    encoding = offset;
    offset = swap;
  }

  offset = +offset || 0;
  encoding = String(encoding || 'utf8').toLowerCase();

  // Make sure we are not going to overflow
  var maxLength = this.length - offset;

  var ret;
  switch (encoding) {
    case 'hex':
      ret = this.parent.hexWrite(string, this.offset + offset, maxLength);
      break;

    case 'utf8':
    case 'utf-8':
      ret = this.parent.utf8Write(string, this.offset + offset, maxLength);
      break;

    case 'ascii':
      ret = this.parent.asciiWrite(string, this.offset + offset, maxLength);
      break;

    case 'binary':
      ret = this.parent.binaryWrite(string, this.offset + offset, maxLength);
      break;

    case 'base64':
      // Warning: maxLength not taken into account in base64Write
      ret = this.parent.base64Write(string, this.offset + offset, maxLength);
      break;

    case 'ucs2':
    case 'ucs-2':
      ret = this.parent.ucs2Write(string, this.offset + offset, maxLength);
      break;

    default:
      throw new Error('Unknown encoding');
  }

  Buffer._charsWritten = SlowBuffer._charsWritten;

  return ret;
};


// toString(encoding, start=0, end=buffer.length)
Buffer.prototype.toString = function(encoding, start, end) {
  encoding = String(encoding || 'utf8').toLowerCase();

  if (typeof start == 'undefined' || start < 0) {
    start = 0;
  } else if (start > this.length) {
    start = this.length;
  }

  if (typeof end == 'undefined' || end > this.length) {
    end = this.length;
  } else if (end < 0) {
    end = 0;
  }

  start = start + this.offset;
  end = end + this.offset;

  switch (encoding) {
    case 'hex':
      return this.parent.hexSlice(start, end);

    case 'utf8':
    case 'utf-8':
      return this.parent.utf8Slice(start, end);

    case 'ascii':
      return this.parent.asciiSlice(start, end);

    case 'binary':
      return this.parent.binarySlice(start, end);

    case 'base64':
      return this.parent.base64Slice(start, end);

    case 'ucs2':
    case 'ucs-2':
      return this.parent.ucs2Slice(start, end);

    default:
      throw new Error('Unknown encoding');
  }
};


// byteLength
Buffer.byteLength = SlowBuffer.byteLength;


// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
Buffer.prototype.copy = function(target, target_start, start, end) {
  var source = this;
  start || (start = 0);
  end || (end = this.length);
  target_start || (target_start = 0);

  if (end < start) throw new Error('sourceEnd < sourceStart');

  // Copy 0 bytes; we're done
  if (end === start) return 0;
  if (target.length == 0 || source.length == 0) return 0;

  if (target_start < 0 || target_start >= target.length) {
    throw new Error('targetStart out of bounds');
  }

  if (start < 0 || start >= source.length) {
    throw new Error('sourceStart out of bounds');
  }

  if (end < 0 || end > source.length) {
    throw new Error('sourceEnd out of bounds');
  }

  // Are we oob?
  if (end > this.length) {
    end = this.length;
  }

  if (target.length - target_start < end - start) {
    end = target.length - target_start + start;
  }

  return this.parent.copy(target.parent,
                          target_start + target.offset,
                          start + this.offset,
                          end + this.offset);
};


// slice(start, end)
Buffer.prototype.slice = function(start, end) {
  if (end === undefined) end = this.length;
  if (end > this.length) throw new Error('oob');
  if (start > end) throw new Error('oob');
  return new exports.Buffer(this.parent, end - start, +start + this.offset);
};


// Legacy methods for backwards compatibility.

Buffer.prototype.utf8Slice = function(start, end) {
  return this.toString('utf8', start, end);
};

Buffer.prototype.binarySlice = function(start, end) {
  return this.toString('binary', start, end);
};

Buffer.prototype.asciiSlice = function(start, end) {
  return this.toString('ascii', start, end);
};

Buffer.prototype.utf8Write = function(string, offset) {
  return this.write(string, offset, 'utf8');
};

Buffer.prototype.binaryWrite = function(string, offset) {
  return this.write(string, offset, 'binary');
};

Buffer.prototype.asciiWrite = function(string, offset) {
  return this.write(string, offset, 'ascii');
};

function createParentProxy(proxy, handler) {
  var proto = SlowBuffer.prototype;
  var pHandler = Object.create(handler);

  pHandler.get = function (receiver, name) {
    if (name in proto)
      return proto[name];
    return proxy.get(receiver, name);
  };
  return Proxy.create(pHandler);
}

// Shamelessly based on the example from
// https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Proxy
function proxifyArray(array) {
  // Other classes like to stash properties on us
  var propHolder = Object.create(Buffer.prototype);
  var handler = {
    // Fundamental traps
    getOwnPropertyDescriptor: function(name) {
      var desc = Object.getOwnPropertyDescriptor(array, name);
      // a trapping proxy's properties must always be configurable
      if (desc !== undefined) { desc.configurable = true; }
      return desc;
    },
    getPropertyDescriptor:  function(name) {
      var desc = Object.getPropertyDescriptor(array, name); // not in ES5
      // a trapping proxy's properties must always be configurable
      if (desc !== undefined) { desc.configurable = true; }
      return desc;
    },
    getOwnPropertyNames: function() {
      return Object.getOwnPropertyNames(array);
    },
    getPropertyNames: function() {
      return Object.getPropertyNames(array);                // not in ES5
    },
    defineProperty: function(name, desc) {
      Object.defineProperty(array, name, desc);
    },
    delete:       function(name) { return delete array[name]; },   
    fix:          function() {
      if (Object.isFrozen(array)) {
        return Object.getOwnPropertyNames(array).map(function(name) {
          return Object.getOwnPropertyDescriptor(array, name);
        });
      }
      // As long as obj is not frozen, the proxy won't allow itself to be fixed
      return undefined; // will cause a TypeError to be thrown
    },
   
    // derived traps
    has:          function(name) {
      return name in array || name in propHolder;
    },
    hasOwn:       function(name) { return Object.prototype.hasOwnProperty.call(array, name); },
    get:          function(receiver, name) {
      if (name == 'rawArray') {
        return array;
      }
      if (name == 'parent') {
        return createParentProxy(this, handler);
      }
      if (isNaN(+name) && name in propHolder) {
        return propHolder[name];
      } else {
        return array[name];
      }
    },
    set:          function(receiver, name, val) {
      if (isNaN(+name)) {
        propHolder[name] = val;
      } else {
        array[name] = val;
      }
      return true;
    }, // bad behavior when set fails in non-strict mode
    enumerate:    function() {
      var result = [];
      for (name in array) { result.push(name); };
      for (name in propHolder) { result.push(name); };
      return result;
    },
    keys: function() { return Object.keys(array) }
  };
  return Proxy.create(handler);
}

function createTypedArray(subject, encoding, offset) {
  var type;
  var length;
  var parent;

  // Are we slicing?
  if (typeof offset === 'number') {
    var actualOffset = offset - subject.offset;
    var preparent = subject.rawArray.subarray(actualOffset, actualOffset + encoding);
    parent = proxifyArray(preparent);
    parent.offset = offset;
    parent.length = encoding;
    return parent;
  } else {
    // Find the length
    switch (type = typeof subject) {
      case 'number':
        length = subject;
        break;

      case 'string':
        length = Buffer.byteLength(subject, encoding);
        break;

      case 'object': // Assume object is an array
        length = subject.length;
        break;

      default:
        throw new Error('First argument needs to be a number, ' +
                        'array or string.');
    }

    var buffer = new ArrayBuffer(length);
    var parent = proxifyArray(new Uint8Array(buffer));
    parent.offset = 0;

    // Treat array-ish objects as a byte array.
    if (isArrayIsh(subject)) {
      for (var i = 0; i < length; i++) {
        parent[i + parent.offset] = subject[i];
      }
    } else if (type == 'string') {
      // We are a string
      length = parent.write(subject, 0, encoding);
    }
    return parent;
  }
}

createTypedArray.isBuffer = function isTypedArray(b) {
  return b.rawArray instanceof Uint8Array;
};

Object.defineProperty(createTypedArray, '_charsWritten', {
  get: function () { return Buffer._charsWritten; }
});

createTypedArray.byteLength = Buffer.byteLength;

if ('ArrayBuffer' in this) {
  exports.Buffer = createTypedArray;
}
