(function (process) {

process.global.process = process;
process.global.global = process.global;
global.GLOBAL = global;

/** deprecation errors ************************************************/

function removed (reason) {
  return function () {
    throw new Error(reason)
  }
}

GLOBAL.__module = removed("'__module' has been renamed to 'module'");
GLOBAL.include = removed("include(module) has been removed. Use require(module)");
GLOBAL.puts = removed("puts() has moved. Use require('sys') to bring it back.");
GLOBAL.print = removed("print() has moved. Use require('sys') to bring it back.");
GLOBAL.p = removed("p() has moved. Use require('sys') to bring it back.");
process.debug = removed("process.debug() has moved. Use require('sys') to bring it back.");
process.error = removed("process.error() has moved. Use require('sys') to bring it back.");
process.watchFile = removed("process.watchFile() has moved to fs.watchFile()");
process.unwatchFile = removed("process.unwatchFile() has moved to fs.unwatchFile()");

GLOBAL.node = {};

node.createProcess = removed("node.createProcess() has been changed to process.createChildProcess() update your code");
process.createChildProcess = removed("childProcess API has changed. See doc/api.txt.");
node.exec = removed("process.exec() has moved. Use require('sys') to bring it back.");
node.inherits = removed("node.inherits() has moved. Use require('sys') to access it.");
process.inherits = removed("process.inherits() has moved to sys.inherits.");

node.http = {};
node.http.createServer = removed("node.http.createServer() has moved. Use require('http') to access it.");
node.http.createClient = removed("node.http.createClient() has moved. Use require('http') to access it.");

node.tcp = {};
node.tcp.createServer = removed("node.tcp.createServer() has moved. Use require('tcp') to access it.");
node.tcp.createConnection = removed("node.tcp.createConnection() has moved. Use require('tcp') to access it.");

node.dns = {};
node.dns.createConnection = removed("node.dns.createConnection() has moved. Use require('dns') to access it.");

process.assert = function (x, msg) {
  if (!(x)) throw new Error(msg || "assertion error");
};


// From jQuery.extend in the jQuery JavaScript Library v1.3.2
// Copyright (c) 2009 John Resig
// Dual licensed under the MIT and GPL licenses.
// http://docs.jquery.com/License
// Modified for node.js (formely for copying properties correctly)
var mixinMessage;
process.mixin = function() {
  if (!mixinMessage) {
    mixinMessage = 'deprecation warning: process.mixin will be removed from node-core future releases.\n'
    process.binding('stdio').writeError(mixinMessage);
  }
  // copy reference to target object
  var target = arguments[0] || {}, i = 1, length = arguments.length, deep = false, source;

  // Handle a deep copy situation
  if ( typeof target === "boolean" ) {
    deep = target;
    target = arguments[1] || {};
    // skip the boolean and the target
    i = 2;
  }

  // Handle case when target is a string or something (possible in deep copy)
  if ( typeof target !== "object" && !(typeof target === 'function') )
    target = {};

  // mixin process itself if only one argument is passed
  if ( length == i ) {
    target = GLOBAL;
    --i;
  }

  for ( ; i < length; i++ ) {
    // Only deal with non-null/undefined values
    if ( (source = arguments[i]) != null ) {
      // Extend the base object
      Object.getOwnPropertyNames(source).forEach(function(k){
        var d = Object.getOwnPropertyDescriptor(source, k) || {value: source[k]};
        if (d.get) {
          target.__defineGetter__(k, d.get);
          if (d.set) {
            target.__defineSetter__(k, d.set);
          }
        }
        else {
          // Prevent never-ending loop
          if (target === d.value) {
            return;
          }

          if (deep && d.value && typeof d.value === "object") {
            target[k] = process.mixin(deep,
              // Never move original objects, clone them
              source[k] || (d.value.length != null ? [] : {})
            , d.value);
          }
          else {
            target[k] = d.value;
          }
        }
      });
    }
  }
  // Return the modified object
  return target;
};

var cwd = process.cwd();

// bootstrap the module loading system.
var module = {};
process.compile("(function (exports) {"
               + process.binding("natives").module
               + "\n});", "module")(module);

// nextTick()

var nextTickQueue = [];
var nextTickWatcher = new process.IdleWatcher();
// Only debugger has maximum priority. Below that is the nextTickWatcher.
nextTickWatcher.setPriority(process.EVMAXPRI-1);

nextTickWatcher.callback = function () {
  var l = nextTickQueue.length;
  while (l--) {
    var cb = nextTickQueue.shift();
    cb();
  }
  if (nextTickQueue.length == 0) nextTickWatcher.stop();
};

process.nextTick = function (callback) {
  nextTickQueue.push(callback);
  nextTickWatcher.start();
};


// Signal Handlers

function isSignal (event) {
  return event.slice(0, 3) === 'SIG' && process.hasOwnProperty(event);
};
// load events to turn process into an event emitter.
module.require("events");
process.addListener("newListener", function (event) {
  if (isSignal(event) && process.listeners(event).length === 0) {
    var b = process.binding('signal_watcher');
    var w = new b.SignalWatcher(process[event]);
    w.addListener("signal", function () {
      process.emit(event);
    });
  }
});


// Timers
function addTimerListener (callback) {
  var timer = this;
  // Special case the no param case to avoid the extra object creation.
  if (arguments.length > 2) {
    var args = Array.prototype.slice.call(arguments, 2);
    timer.callback = function () { callback.apply(timer, args); };
  } else {
    timer.callback = callback;
  }
}

global.setTimeout = function (callback, after) {
  var timer = new process.Timer();
  addTimerListener.apply(timer, arguments);
  timer.start(after, 0);
  return timer;
};

global.setInterval = function (callback, repeat) {
  var timer = new process.Timer();
  addTimerListener.apply(timer, arguments);
  timer.start(repeat, repeat);
  return timer;
};

global.clearTimeout = function (timer) {
  if (timer instanceof process.Timer) {
    timer.stop();
  }
};

global.clearInterval = global.clearTimeout;

var debugLevel = parseInt(process.env["NODE_DEBUG"]);
function debug (x) {
  if (debugLevel > 0) {
    process.binding('stdio').writeError(x + "\n");
  }
}

var stdout;
process.__defineGetter__('stdout', function () {
  if (stdout) return stdout;
  var net = module.requireNative('net');
  stdout = new net.Stream(process.binding('stdio').stdoutFD);
  return stdout;
});

var stdin;
process.openStdin = function () {
  if (stdin) return stdin;
  var net = module.requireNative('net');
  var fd = process.binding('stdio').openStdin();
  stdin = new net.Stream(fd);
  stdin.resume();
  stdin.readable = true;
  return stdin;
};

process.exit = function (code) {
  process.emit("exit");
  process.reallyExit(code);
};

path = module.require("path");

// Make process.argv[0] and process.argv[1] into full paths.
if (process.argv[0].indexOf('/') > 0) {
  process.argv[0] = path.join(cwd, process.argv[0]);
}

if (process.argv[1].charAt(0) != "/") {
  process.argv[1] = path.join(cwd, process.argv[1]);
}

// Load the main module--the command line argument.
module.main(process.argv[1]);

// All our arguments are loaded. We've evaluated all of the scripts. We
// might even have created TCP servers. Now we enter the main eventloop. If
// there are no watchers on the loop (except for the ones that were
// ev_unref'd) then this function exits. As long as there are active
// watchers, it blocks.
process.loop();

process.emit("exit");

});
