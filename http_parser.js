var assert = process.assert;
var EventEmitter = require('events').EventEmitter;
var inherits = require('util').inherits;
var b = Buffer("GET / HTTP/1.1\r\n\r\n");

var InternalHTTPParser = process.binding("http_parser").HTTPParser;


function HTTPParser(type) {
  EventEmitter.call(this);
  this.internal = new InternalHTTPParser(type);
  this.waiting = [];
}
inherits(HTTPParser, EventEmitter);


HTTPParser.prototype.pause = function() {
  this.paused = true;
};


HTTPParser.prototype.resume = function() {
  this.paused = false;
};


HTTPParser.prototype._addHeader = function() {
  if (this.field && this.value) {
    if (this._headersComplete) {
      this.incoming.trailers.push([this.field, this.value]);
    } else {
      this.incoming.headers.push([this.field, this.value]);
    }
    this.field = null;
    this.value = null;
  }
};


HTTPParser.prototype._cycle = function() {
  var x; 
  while (!this.paused && (x = this.waiting.shift())) {
    var buf = x[0];
    var off = x[1];
    var len = x[2];

    var tokens = this.internal.execute(buf, off, len);
    console.log("%j", tokens);
    assert(tokens.length % 3 == 0);

    var i = 0;
    debugger;
    while (i < tokens.length) {
      var type = tokens[i++];
      var arg0 = tokens[i++];
      var arg1 = tokens[i++];

      switch (type) {
        case "ERROR":
          // Emit buffer and offset of error
          this.emit('error', buf, arg0);
          break;

        case "NEEDS_INPUT":
          this.emit('needsInput');
          break;

        case "NEEDS_DATA_ELEMENTS":
          this.waiting.unshift(buf, arg0, (off + len) - arg0);
          // go around again.
          break;

        case "REQ_MESSAGE_START":
          this.incoming = { method: arg0,
                            headers: [],
                            trailers: [] };
          break;

        case "RES_MESSAGE_START":
          this.incoming = { statusCode: arg0,
                            headers: [],
                            trailers: [] };
          break;

        case "VERSION":
          this.incoming.httpVersionMajor = arg0;
          this.incoming.httpVersionMinor = arg1;
          this.incoming.httpVersion = arg0 + '.' + arg1;
          break;

        case "PATH":
        case "FRAGMENT":
        case "QUERY_STRING":
          // not using these.
          break;

        case "URL":
          var slice = buf.toString('ascii', arg0, arg0 + arg1);
          if (this.incoming.url) {
            this.incoming.url += slice
          } else {
            this.incoming.url = slice;
          }
          break;

        case "HEADER_FIELD":
          var slice = buf.toString('ascii', arg0, arg0 + arg1).toLowerCase();

          // The token before this was 'HEADER_VALUE' and so we need to add
          // the value to the headers before we start with this field.
          if (this.value) this._addHeader();

          if (this.field) {
            this.field += slice;
          } else {
            this.field = slice;
          }
          break;

        case "HEADER_VALUE":
          var slice = buf.toString('ascii', arg0, arg0 + arg1);
          if (this.value) {
            this.value += slice;
          } else {
            this.value = slice;
          }
          break;

        case "HEADERS_END":
          // Add the last header
          if (this.value) this._addHeader();

          // XXX: upgrade, shouldKeepAlive ???

          this.incoming._headersComplete = true;
          this.emit("messageBegin", this.incoming);
          break;

        case "BODY":
          var body = buf.slice(arg0, arg0 + arg1);
          this.emit("body", body, this.incoming);
          break

        case "MESSAGE_END":
          // Add the last trailing header
          if (this.value) this._addHeader();
          this.emit("messageEnd", this.incoming);
          this.incoming = null;
          break;

        default:
          console.log(type, arg0, arg1);
          throw new Error("Unknown token");
      }
    }

  }
};


HTTPParser.prototype.execute = function(buffer, off, len) {
  this.waiting.push([buffer, off, len]);
  this._cycle();
};




var p = new HTTPParser('request');

p.on('messageBegin', function(m) {
  console.log('messageBegin ', m);
});

p.on('messageEnd', function(m) {
  console.log('messageEnd ', m);
});

var b = new Buffer("GET /blah HTTP/1.1\r\nhello: world\r\nsomething: else\r\n\r\n");
p.execute(b, 0, b.length);

