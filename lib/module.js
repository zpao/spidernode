
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
  process.checkBreak();
  mod.filename = filename;
  return mod.compile();
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
  return mod.compile().exports;
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
  return parent.spawn(id).compile().exports;
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

  var exts = exports.extensions
    , found = null
    , searchPath
  for (var i = 0, l = searchPaths.length; i < l && !found; i ++) {
    searchPath = path.join(searchPaths[i], id);
    // safe to short-circuit here, since we'll be checking each file
    // if the particular ID isn't in the cache.
    if (searchPath in this.moduleCache) {
      var mod = this.moduleCache[searchPath];
      mod.locked = true;
      return mod;
    }
    for (var j = 0, m = exts.length; j < m && !found; j ++) {
      var ext = exts[j]
        , file = searchPath+ext
        , index = path.join(searchPath, "index"+ext)
      if (!ext) throw new Error("Invalid module file extension: "+ext);
      found = file in this.moduleCache ? file
            : index in this.moduleCache ? index
            : exists(file) ? file
            : exists(index) ? index
            : null;
    }
  }
  if (!found) throw new Error("Module "+id+" not found. NODE_PATH="+searchPaths.join(":"));
  if (found in this.moduleCache) {
    this.moduleCache[found].locked = true;
    return this.moduleCache[found];
  }
  return this.mintChild(searchPath, found);
}
Module.prototype.mintChild = function (id, filename) {
  // mint the new child Module object
  var child = new Module(id, this)
  this.moduleCache[id] = this.moduleCache[filename] = child;
  child.filename = filename;
  return child;
}

Module.prototype.load = function () {
  // If it's already been loaded/compiled, just return.
  if (this.data || this.compiled) return this;
  var ext = extname(this.filename)
  this.loaders[ (ext in this.loaders) ? ext : ".js" ].call(this);
  if (!this.data) this.compiled = true;
  return this;
}
Module.prototype.loaders =
  { ".node" : function () {
      process.dlopen(this.filename, this.exports);
    }
  , ".js" : function () {
      this.data = requireNative("fs").readFileSync(this.filename);
    }
  }
Module.prototype.mintRequire = function () {
  var self = this
  function require (id) { return self.require(id) }
  require.main = process.mainModule;
  require.paths = exports.paths;
  return require;
}
Module.prototype.compile = function () {
  if (this.compiled) return this;
  if (!this.data) return this.load().compile();

  if (!this.filename) this.filename = this.id;
  var ext = extname(this.filename)
  this.compiled = true;
  this.compilers[ (ext in this.loaders) ? ext : ".js" ].call
    ( this
    , this.mintRequire()
    );
  return this;
}
Module.prototype.compilers =
  { ".js" : function (require) {
      this.data = this.data.replace(/^\#\!.*/, '');
      process.compile
        ( "(function (exports, require, module, __filename, __dirname) { "
          + this.data + "\n});"
        , this.filename
        ).call
        ( this
        , this.exports
        , require
        , this
        , this.filename
        , dirname(this.filename)
        );
    }
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
function extname (path) {
  var index = path.lastIndexOf('.');
  return index < 0 ? '' : path.substring(index);
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