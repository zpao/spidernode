/**********************************************************************/

// Module

function Module (id, parent) {
  this.id = id;
  this.exports = {};
  this.parent = parent;

  this.moduleCache = parent ? parent.moduleCache : {};
  this.moduleCache[this.id] = this;

  this.filename = null;
  this.loaded = false;
  this.exited = false;
  this.children = [];
};

var rootModule = {moduleCache:{}};

function createInternalModule (id, constructor) {
  var m = new Module(id, rootModule);
  m._setup(constructor);
  m.loaded = true;
  return m;
};

Module.prototype._setup = function (constructor) {
  if (typeof constructor === "function") {
    constructor.call(this, this.exports);
  } else {
    this.exports = constructor;
  }
}

function sandboxIn(module) {
  var s = Object.create(module);
  s.moduleCache = Object.create(module.moduleCache);
  return s;
}


// This contains the source code for the files in lib/
// Like, natives.fs is the contents of lib/fs.js
var natives = process.binding('natives');

function loadNative (id) {
  var m = new Module(id, rootModule);
  m._setup(m._compile(natives[id], id));
  m.loaded = true;
  return m;
}

exports.requireNative = requireNative;
function requireNative (id) {
  if (rootModule.moduleCache[id]) return rootModule.moduleCache[id].exports;
  if (!natives[id]) throw new Error('No such native module ' + id);
  return loadNative(id).exports;
}

// Event

var eventsFn = process.compile("(function (exports) {" + natives.events + "\n})",
                               "events");
var eventsModule = createInternalModule('events', eventsFn);
var events = eventsModule.exports;


// Modules

var debugLevel = parseInt(process.env["NODE_DEBUG"], 16);
function debug (x) {
  if (debugLevel & 1) {
    process.binding('stdio').writeError(x + "\n");
  }
}

var pathModule = createInternalModule
  ( 'path'
  , process.compile
    ( "(function (exports) {" + natives.path + "})"
    , "path"
    )
  );

var path = pathModule.exports;

function existsSync (path) {
  try {
    process.binding('fs').stat(path);
    return true;
  } catch (e) {
    return false;
  }
}


var extensionCache = {
  ".js": null, //identity, no compiler needed
  ".node": function() { throw new Error(".node cannot be compiled into js."); },
};

var modulePaths = [path.join(process.installPrefix, "lib", "node")];

if (process.env["HOME"]) {
  modulePaths.unshift(path.join(process.env["HOME"], ".node_libraries"));
}

if (process.env["NODE_PATH"]) {
  [].unshift.apply(modulePaths, process.env["NODE_PATH"].split(":"));
}


/* Sync unless callback given */
function findModulePath (id, dirs, callback) {
  process.assert(dirs instanceof Array);

  if (/^https?:\/\//.exec(id)) {
    if (callback) {
      callback(id);
    } else {
      throw new Error("Sync http require not allowed.");
    }
    return;
  }

  if (/\.(js|node)$/.exec(id)) {
    throw new Error("No longer accepting filename extension in module names");
  }

  if (dirs.length == 0) {
    if (callback) {
      callback();
    } else {
      return; // sync returns null
    }
  }

  var dir = dirs[0];
  var rest = dirs.slice(1, dirs.length);

  if (id.charAt(0) == '/') {
    dir = '';
    rest = [];
  }

  var locations = [];
  var extensions = Object.keys(extensionCache);
  for (var i = 0, l = extensions.length; i < l; i++) {
    var ext = extensions[i];
    locations.push(
      path.join(dir, id + ext),
      path.join(dir, id, 'index' + ext)
    );
  }

  function searchLocations () {
    var location = locations.shift();
    if (!location) {
      return findModulePath(id, rest, callback);
    }

    // if async
    if (callback) {
      path.exists(location, function (found) {
        if (found) {
          callback(location);
        } else {
          return searchLocations();
        }
      });

    // if sync
    } else {
      if (existsSync(location)) {
        return location;
      } else {
        return searchLocations();
      }
    }
  }
  return searchLocations();
}


// sync - no i/o performed
function resolveModulePath(request, parent) {
  var id, paths;
  if (request.charAt(0) == "." && (request.charAt(1) == "/" || request.charAt(1) == ".")) {
    // Relative request
    debug("RELATIVE: requested:" + request + " set ID to: "+id+" from "+parent.id);

    var exts = [];
    var extensions = Object.keys(extensionCache);
    for (var i = 0, l = extensions.length; i < l; i++) {
      var ext = extensions[i];
      exts.push(ext.slice(1));
    }

    var parentIdPath = path.dirname(parent.id +
      (path.basename(parent.filename).match(new RegExp('^index\\.(' + exts.join('|') + ')$')) ? "/" : ""));
    id = path.join(parentIdPath, request);
    paths = [path.dirname(parent.filename)];
  } else {
    id = request;
    // debug("ABSOLUTE: id="+id);
    paths = modulePaths;
  }

  return [id, paths];
}


function loadModule (request, parent, callback) {
  var resolvedModule = resolveModulePath(request, parent),
      id = resolvedModule[0],
      paths = resolvedModule[1];

  debug("loadModule REQUEST  " + (request) + " parent: " + parent.id);

  var cachedModule = parent.moduleCache[id];

  if (!cachedModule) {
    // Try to compile from native modules
    if (natives[id]) {
      debug('load native module ' + id);
      cachedModule = loadNative(id);
    }
  }

  if (cachedModule) {
    debug("found  " + JSON.stringify(id) + " in cache");
    if (callback) {
      callback(null, cachedModule.exports);
    } else {
      return cachedModule.exports;
    }

  } else {
    // Not in cache
    debug("looking for " + JSON.stringify(id) + " in " + JSON.stringify(paths));

    if (!callback) {
      // sync
      var filename = findModulePath(request, paths);
      if (!filename) {
        throw new Error("Cannot find module '" + request + "'");
      } else {
        var module = new Module(id, parent);
        module.loadSync(filename);
        return module.exports;
      }

    } else {
      // async
      findModulePath(request, paths, function (filename) {
        if (!filename) {
          var err = new Error("Cannot find module '" + request + "'");
          callback(err);
        } else {
          var module = new Module(id, parent);
          module.load(filename, callback);
        }
      });
    }
  }
};


// This function allows the user to register file extensions to custom
// Javascript 'compilers'.  It accepts 2 arguments, where ext is a file
// extension as a string. E.g. '.coffee' for coffee-script files.  compiler
// is the second argument, which is a function that gets called when the
// specified file extension is found. The compiler is passed a single
// argument, which is, the file contents, which need to be compiled.
//
// The function needs to return the compiled source, or an non-string
// variable that will get attached directly to the module exports. Example:
//
//    require.registerExtension('.coffee', function(content) {
//      return doCompileMagic(content);
//    });
function registerExtension(ext, compiler) {
  if ('string' !== typeof ext && false === /\.\w+$/.test(ext)) {
    throw new Error('require.registerExtension: First argument not a valid extension string.');
  }

  if ('function' !== typeof compiler) {
    throw new Error('require.registerExtension: Second argument not a valid compiler function.');
  }

  extensionCache[ext] = compiler;
}


var loaders = {};


Module.prototype.goLoading = function (filename) {
  process.assert(!this.loaded);
  this.filename = filename;
  return loaders[path.extname(filename)] || this._loadDefault;
}


Module.prototype.loadSync = function (filename) {
  debug("loadSync " + JSON.stringify(filename) + " for module " + JSON.stringify(this.id));
  this.goLoading(filename).sync.call(this);
};


Module.prototype.load = function (filename, callback) {
  debug("load " + JSON.stringify(filename) + " for module " + JSON.stringify(this.id));
  this.goLoading(filename).async.call(this, callback);
};


loaders[".node"] = {
  sync: function () {
    this.loaded = true;
    process.dlopen(this.filename, this.exports);
  },

  async: function (callback) {
    var self = this;
    // XXX Not yet supporting loading from HTTP. would need to download the
    // file, store it to tmp then run dlopen on it.
    self.loaded = true;
    process.dlopen(self.filename, self.exports); // FIXME synchronus
    if (callback) callback(null, self.exports);
  },
};


function cat (id, callback) {
  if (id.match(/^http:\/\//)) {
    loadModule('http', process.mainModule, function (err, http) {
      if (err) {
        if (callback) callback(err);
      } else {
        http.cat(id, callback);
      }
    });
  } else {
    requireNative('fs').readFile(id, 'utf8', callback);
  }
}


// returns constructor function or exports object
Module.prototype._compile = function (content, filename) {
  // remove shebang
  content = content.replace(/^\#\!.*/, '');

  // Compile content if needed
  var ext = path.extname(filename);
  if (extensionCache[ext]) {
    content = extensionCache[ext](content);
  }

  if ('string' === typeof content) {
    // create wrapper function
    var wrapper = "(function (exports, require, module, __filename, __dirname) { "
                + content
                + "\n});";

    var compiledWrapper = process.compile(wrapper, filename);
    var dirName = path.dirname(filename);
    if (filename === process.argv[1]) {
      process.checkBreak();
    }
    return function (exports) {
      compiledWrapper.apply(
        exports, [exports, makeRequireFunction(this), this, filename, dirName]
      );
    };
  } else {
    return content;
  }
};

function makeRequireFunction(module) {
  function requireAsync (url, cb) {
    loadModule(url, module, cb);
  }

  function require (path) {
    return loadModule(path, module);
  }

  function requireSandboxedAsync (url, cb) {
    loadModule(url, sandboxIn(module), cb);
  }

  function requireSandboxed (path) {
    return loadModule(path, sandboxIn(module));
  }

  require.paths = modulePaths;
  require.async = requireAsync;
  require.sandboxed = requireSandboxed;
  require.sandboxed.async = requireSandboxedAsync;
  require.main = process.mainModule;
  require.registerExtension = registerExtension;

  return require;
}

Module.prototype._loadDefault = {
  sync: function () {
    var content = requireNative('fs').readFileSync(this.filename, 'utf8');
    this._setup(this._compile(content, this.filename));
    this.loaded = true;
  },

  async: function (callback) {
    var self = this;
    cat(this.filename, function (err, content) {
      debug('cat done');
      if (err) {
        if (callback) callback(err);
      } else {
        try {
          self._setup(self._compile(content, self.filename));
        } catch(e) {
          if (callback) callback(e);
          return;
        }
        self._waitChildrenLoad(function () {
          self.loaded = true;
          if (self.onload) self.onload();
          if (callback) callback(null, self.exports);
        });
      }
    });
  },
};


Module.prototype._waitChildrenLoad = function (callback) {
  var nloaded = 0;
  var children = this.children;
  for (var i = 0; i < children.length; i++) {
    var child = children[i];
    if (child.loaded) {
      nloaded++;
    } else {
      child.onload = function () {
        child.onload = null;
        nloaded++;
        if (children.length == nloaded && callback) callback();
      };
    }
  }
  if (children.length == nloaded && callback) callback();
};



// bootstrap main module.
exports.runMain = function () {

  // Load the main module--the command line argument.
  process.mainModule = new Module(".");
  process.mainModule.loadSync(process.argv[1]);
}
