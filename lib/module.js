
exports.main = main;
exports.require = require;
exports.requireNative = requireNative;
exports.Module = Module;
exports.paths = [];
exports.extensions = [ ".js", ".node" ];

var natives = process.binding("natives")
  , nativeModuleCache = { module : { exports : exports } }

// load the main module, or return it.
function main (filename) {
  if (process.mainModule) return process.mainModule;
  var mod = nativeModuleCache["."] = process.mainModule = new Module(".")
  mod.filename = filename;
  // start the module when the event loop fires up.
  process.checkBreak();
  process.nextTick(function () { mod.go() });
}

function requireNative (id) {
  if (id in nativeModuleCache) {
    return nativeModuleCache[id].exports;
  }
  if (!(id in natives)) {
    throw new Error("Native module "+id+" not found");
  }
  var mod = nativeModuleCache[id] = new Module(id)
  mod.data = natives[id];
  return mod.go().exports;
}

// load up the module referenced by "id", from "parent"
function require (id, parent) {
  if (!parent) {
    if (this instanceof Module) parent = this;
    else if (process.mainModule) parent = process.mainModule;
  }
  if ((id in natives) || (id in nativeModuleCache)) {
    return requireNative(id);
  }
  if (!parent) {
    throw new Error("Cannot load non-native orphan module");
  }
  return parent.spawn(id).go().exports;
}

function Module (id, parent) {
  this.moduleCache = (parent) ? parent.moduleCache : {};

  var exports = {}
  Object.defineProperty(this, "exports",
    { set : function (e) {
        if (this.locked) {
          throw new Error(
            "Cannot set exports after being imported by another module.");
        }
        exports = e;
      }
    , get : function () { return exports }
    });

  this.locked = false;
  this.id = id;
  this.parent = parent;
}

Module.prototype.require = require;

Module.prototype.spawn = function (id) {
  // resolve and create the child module.
  var searchPaths = exports.paths
    , path = requireNative("path")

  if (!id) throw new Error("Invalid module id: "+id);

  id = id.replace(/^file:(\/\/)?/, '');
  if (id.charAt(0) === "/") {
    // absolute.
    searchPaths = [""];
  } else if (id.charAt(0) === '.') {
    searchPaths = [ path.dirname(this.filename) ];
  }
  if (searchPaths.length === 1) {
    // can only be one, so short-circuit here by checking the cache
    var shortCircuit = path.join(searchPaths[0], id)
    if (shortCircuit in this.moduleCache) {
      this.moduleCache[shortCircuit].locked = true;
      return this.moduleCache[shortCircuit];
    }
  }

  var exts = exports.extensions
    , found = null
    , searchPath
  for (var i = 0, l = searchPaths.length; i < l && !found; i ++) {
    searchPath = path.join(searchPaths[i], id);
    for (var j = 0, m = exts.length; j < m && !found; j ++) {
      var ext = exts[j]
        , file = searchPath+ext
        , index = path.join(searchPath, "index"+ext)
      found = exists(file) ? file
            : exists(index) ? index
            : null;
    }
  }
  if (!found) throw new Error("Module "+id+" not found");
  if (found in this.moduleCache) {
    this.moduleCache[found].locked = true;
    return this.moduleCache[found];
  }

  // mint the new child Module object
  id = searchPath;
  var child = new Module(id, this)
  this.moduleCache[id] = this.moduleCache[found] = child;
  child.filename = found;
  return child;
}

Module.prototype.go = function () {
  // already have filename or code.
  return this.compiled ? this
       : this.data ? this.compile()
       : this.load().compile();
}

Module.prototype.load = function () {
  // If it's already been loaded/compiled, just return.
  if (this.data || this.compiled) return this;

  var fs = requireNative("fs")
    , path = requireNative("path")
    , ext = path.extname(this.filename)

  // if it's a .node, then do the dlopen dance
  if (ext === ".node") {
    process.dlopen(this.filename, this.exports);
    this.compiled = function () {}
  } else {
    this.data = fs.readFileSync(this.filename);
    if (!this.data) {
      // empty file.  fake compilation.
      this.compiled = function () {}
    }
  }
  return this;
}

Module.prototype.compile = function () {
  if (this.compiled) return this;
  if (!this.data) return this.load().compile();

  if (!this.filename) this.filename = this.id;

  var self = this
  function require (id) { return self.require(id) }
  require.main = process.mainModule;
  require.paths = exports.paths;

  this.data = this.data.replace(/^\#\!.*/, '');
  this.compiled = process.compile(
    "(function (exports, require, module, __filename, __dirname) { "
      + this.data + "\n});"
    , this.filename);
  this.compiled
    ( this.exports
    , require
    , this
    , this.filename
    , dirname(this.filename)
    );
  return this;
}

// TODO: Move to fs.existsSync
function exists (path) {
  var fs = requireNative("fs")
  try {
    fs.statSync(path);
  } catch (e) {
    return false;
  }
  return true;
}
function dirname (path) {
  path = path || "";
  return path.substr(0, path.lastIndexOf("/")) || ".";
}
function bootstrap () {
  var path = requireNative("path")
  if (process.env.HOME) {
    exports.paths.unshift(path.join(process.env.HOME, ".node_libraries"));
  }
  if (process.env.NODE_PATH) {
    exports.paths = process.env.NODE_PATH.split(":").concat(exports.paths);
  }
}
bootstrap();
