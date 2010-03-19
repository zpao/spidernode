
exports.main = main;
exports.require = require;
exports.requireNative = requireNative;
exports.Module = Module;
exports.paths = [];
exports.registerExtension = registerExtension;
exports.extensionCache = {};
exports.registerScheme = registerScheme;
exports.schemeCache = {};

// This contains the source code for the files in lib/
// Like, natives.fs is the contents of lib/fs.js
var natives = process.binding('natives'),
  nativeModuleCache = { module : exports };

function requireNative (id, cb) {
  if (id in nativeModuleCache) {
    return nativeModuleCache[id];
  }
  if (!(id in natives)) {
    var er = new Error(
      "Native module "+id+" not found");
    if (cb) cb(er);
    else throw er;
    return er;
  }
  var mod = new Module(id, null, id);
  var data = nativeModuleCache[id] = mod.compile(natives[id]);
  if (cb) cb(null, data);
  return data;
}

function require (id, parent, cb) {
  if (id in natives) return requireNative(id, cb);
  if (id in nativeModuleCache) {
    return nativeModuleCache[id];
  }
  if (parent && parent.uri) {
    var base = parent.uri;
  } else {
    var base = "";
  }
  if (!cb) return requireSync(base, id, parent);
  else requireAsync(base, id, parent, cb);
}

function requireSync (base, id, parent) {
  var uri = resolve(base, id, parent);
  if (uri instanceof exports.Module) {
    var child = uri;
    child.locked = true;
    return child.exports;
  }
  if (!uri) throw new Error(
    "Module "+id+" could not be found");

  if (parent && (uri in parent.moduleCache)) {
    var child = parent.moduleCache[uri];
    child.locked = true;
    return child.exports;
  }
  var child = new exports.Module(uri, parent);
  return loadModuleSync(child);
}

function requireAsync (base, id, parent, cb) {
  resolve(base, id, parent, function (er, uri) {
    if (er) return cb(er);
    if (uri instanceof exports.Module) {
      var child = uri;
      child.locked = true;
      return cb(null, child.exports);
    }

    if (!uri) cb(new Error(
      "Module "+id+" could not be found"));

    if (parent && (uri in parent.moduleCache)) {
      var child = parent.moduleCache[uri];
      child.locked = true;
      return cb(null, child.exports);
    }
    var child = new exports.Module(uri, parent);
    loadModule(child, cb);
  });
}

function loadModuleSync (module) {
  var data = module.load();
  if ("string" === typeof data) {
    data = module.compile(data);
  } else if (data !== undefined) {
    module.exports = data;
  }
  if ("string" === typeof data && module.ext !== ".js") {
    // if it's not already JS, it can compile to a JS string.
    module.compileJS(data);
  } else if (data !== undefined) {
    // this is a safe usage.  However, since it's just been
    // put in the cache, and has been the subject of a require(),
    // the lock may already be set.
    var locked = module.locked;
    module.locked = false;
    module.exports = data;
    module.locked = locked;
  }
  return module.exports;
}
function loadModule (module, cb) {
  function firstCompile (er, data) {
    if (er) return cb(er);
    if ("string" === typeof data && module.ext !== ".js") {
      // if it's not already JS, it can compile to a JS string.
      return module.compileJS(data, cb);
    } else if (data !== undefined) {
      // this is a safe usage.  However, since it's just been
      // put in the cache, and has been the subject of a require(),
      // the lock may already be set.
      var locked = module.locked;
      module.locked = false;
      module.exports = data;
      module.locked = locked;
    }
    cb(null, module.exports);
  }
  module.load(function (er, data) {
    if (er) return cb(er);
    if ("string" === typeof data) {
      return module.compile(data, firstCompile);
    } else if (data !== undefined) {
      module.exports = data;
    }
    cb(null, module.exports);
  });
}

// search the paths for id, and then return the resolved filename.
function resolve (root, id, parent, cb) {
  var searchPaths = exports.paths,
    filenames = [],
    f = 0,
    path = requireNative("path"),
    fs = requireNative("fs");

  if (!id) throw new Error("Invalid module id: "+id);
  
  // check the scheme here.
  // if it's something registered, then treat it as absolute.
  var scheme = schemename(id);
  if (scheme === "file:") {
    id = id.substr(0, scheme.length);
    if (id.substr(0, 2) === "//") id = id.substr(2);
  } else if (scheme) {
    var s = exports.schemeCache[scheme];
    s = s && s.resolve;
    if (s) {
      if (cb) {
        if (parent) parent.womb[id] = true;
        s(id, parent, function (er, resolvedUri) {
          if (er) return cb(er);
          if (parent) {
            parent.womb[resolvedUri] = parent.womb[id];
            delete parent.womb[id];
          }
          cb(null, resolvedUri);          
        });
      } else {
        return s(id, parent);
      }
    }
  }

  // the default file-system style lookup
  if (id.charAt(0) === "/") {
    // absolute.
    searchPaths = [""];
  } else if (id.charAt(0) === '.') {
    searchPaths = [ dirname(root) ];
  }
  for (var i = 0, l = searchPaths.length; i < l; i ++) {
    var searchPath = searchPaths[i].replace(/\/+$/, '');
    // short circuit here by checking the cache.
    searchPath = path.join(searchPath, id);
    if (parent && (searchPath in parent.moduleCache)) {
      if (cb) {
        cb(null, parent.moduleCache[searchPath]);
      }
      return parent.moduleCache[searchPath];
    }
    var exts = Object.keys(exports.extensionCache);
    for (var j = 0, m = exts.length; j < m; j ++) {
      var ext = exts[j];
      filenames[f++] = searchPath+ext;
      filenames[f++] = path.join(searchPath, "index"+ext);
    }
  }

  // now filenames is the list of files to check for.
  if (cb) { // async
    if (parent) {
      parent.womb[id] = true;
    }
    (function L (i) {
      if (i === filenames.length) {
        if (parent) {
          // miscarry
          delete parent.womb[id];
        }
        return cb(new Error("Module "+id+" not found"));
      }
      exists(filenames[i], function (e) {
        if (!e) return L(i+1);
        if (parent) {
          // trimester advancement
          parent.womb[filenames[i]] = parent.womb[id];
          delete parent.womb[id];
        }
        cb(null, filenames[i]);
      });
    })(0);
    return;
  }
  // sync
  for (var i = 0, l = filenames.length; i < l; i ++) {
    if (existsSync(filenames[i])) {
      return filenames[i];
    }
  }
  throw new Error("Module "+id+" not found");
}

function main (uri) {
  if (process.mainModule) return process.mainModule;
  process.mainModule = new Module(uri, null, ".");
  nativeModuleCache["."] = process.mainModule;
  loadModule(process.mainModule, function (er) {
    if (er) throw er;
  });
  return process.mainModule;
}

function Module (uri, parent, id) {
  this.uri = this.filename = uri;
  this.id = id || this.filename.replace(/(\/index)?\.[^\.]+$/, '');
  this.ext = extname(uri || "") || ".js";
  this.scheme = schemename(uri);
  
  var moduleExports = {};
  Object.defineProperty(this, "exports",
    { set : function (newExports) {
        if (this.locked) throw new Error(
          this.uri+"\n"+
          "Cannot set exports after being required by another module");
        moduleExports = newExports;
      }
    , get : function () { return moduleExports }
    });


  if (parent) {
    this.parent = parent;
    if ("function" === typeof parent.womb[uri]) {
      this.onload = parent.womb[uri];
    }
    // birthing process
    delete parent.womb[uri];
    parent.children.push(this);
    this.moduleCache = parent.moduleCache;
  } else {
    this.moduleCache = {};
  }
  
  this.moduleCache[ id ] = this.moduleCache[ uri ] = this;

  this.loaded = false;
  this.locked = false;
  this.exited = false;
  this.children = [];
  this.womb = {};

  // bind require's context
  var self = this;
  this.require = function (id) { return require(id, self) };
  Object.defineProperty(this.require, "main",
    { get : function () { return process.mainModule }});
  this.require.async = function (id, cb) { return require(id, self, cb || noop) };
  Object.defineProperty(this.require, "paths",
    { set : function (newPaths) { exports.paths = newPaths }
    , get : function () { return exports.paths }
    });
  this.require.registerExtension = registerExtension;
  this.require.registerScheme = registerScheme;
}
Module.prototype.load = function (cb) {
  var loader;
  // first check for scheme, since that has a greater effect on how to get the data.
  var s = this.scheme;
  s = s && exports.schemeCache[s];
  if (s) {
    loader = s && s.load;
  }
  if (!loader) {
    // use the loader registered for this extension
    loader = exports.extensionCache[ this.ext ];
    loader = loader && loader.load;
  }
  if (!loader && (this.ext !== ".js")) {
    loader = exports.extensionCache[ ".js" ];
    loader = loader && loader.load;
  }
  if (!loader) {
    var er = new Error("No loader found for "+this.uri);
    if (cb) cb(er);
    else throw er;
    return er;
  }
  this.load = loader;
  return this.load(cb);
}
// mod.compile(code, cb)
// if ext is provided, then pretend it's that kind of module.
function getCompiler (ext) {
  ext = ext || ".js";
  var compiler = exports.extensionCache[ ext ];
  compiler = compiler && compiler.compile;
  if (!compiler && (ext !== ".js")) {
    return getCompiler(".js");
  }
  return compiler;
}
Module.prototype.compile = function (code, cb) {
  this.compile = getCompiler(this.ext);
  return this.compile(code, cb);
}
Module.prototype.compileJS = function (code, cb) {
  // if its already been compiled as a JS module,
  // then don't compile it again.  Set to noop here
  // so that the logic is simpler later on.
  this.compileJS = getCompiler(".js");
  if (this.compile === this.compileJS) {
    this.compileJS = noop;
  }
  return this.compileJS(code, cb);
}
Module.prototype.waitForChildren = function (cb) {
  var children = this.children,
    expecting = Object.keys(this.womb),
    litter = expecting.length + children.length,
    loaded = 0;
  
  this.waiting = function () {
    this.waiting = null;
    this.loaded = true;
    if (this.onload) this.onload();
    cb(null, this.exports);
  }
  
  function onload () {
    this.onload = null;
    loaded ++;
    if (loaded === litter && this.parent.waiting) this.parent.waiting();
  }

  for (var i = 0; i < children.length; i++) {
    var child = children[i];
    if (child.loaded) {
      loaded ++;
    } else {
      child.onload = onload;
    }
  }
  for (var i = 0; i < expecting.length; i ++) {
    // this will get attached once it's resolved, and then called when it's loaded.
    this.womb[ expecting[i] ] = onload;
  }
  if (loaded === litter && this.waiting) this.waiting();
}

// specify a function that tells how to fetch things starting with the scheme.
function registerScheme (scheme, phase, loader) {
  var sc = exports.schemeCache;
  sc[scheme] = sc[scheme] || {};
  sc[scheme][phase] = loader;
}

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
//    require("module").registerExtension('.coffee', function(content) {
//      return doCompileMagic(content);
//    });
// Optional second argument "phase" specifies where in the loading process
// this function should be called.  Default is "compile".
// Supported phase values: ["load", "compile"]
// Each phase can return either a string (which will be passed on to the next phase,
// or eventually treated like a javascript string by default), or some other kind
// of non-string thing, which will stop the loading process and set that thing
// as the module.exports.
function registerExtension(ext, compiler) {
  var phase = "compile";
  if (arguments.length > 2) {
    phase = compiler;
    compiler = arguments[2];
  }

  if ('string' !== typeof ext || false === /^\.\w+$/.test(ext)) {
    throw new Error('require.registerExtension: First argument not a valid extension string.');
  }

  if ('function' !== typeof compiler) {
    throw new Error('require.registerExtension: Second argument not a valid compiler function.');
  }

  exports.extensionCache[ext] = exports.extensionCache[ext] || {};
  exports.extensionCache[ext][phase] = compiler;
}

function existsSync (path) {
  try {
    process.binding('fs').stat(path);
  } catch (e) {
    return false;
  }
  return true;
}
function noop () {}
function exists (path, cb) {
  process.binding('fs').stat(path, function (er) { cb(!er) });
}
dirname = function (path) {
  path = path || "";
  return path.substr(0, path.lastIndexOf("/")) || ".";
};
function extname (path) {
  var index = path.lastIndexOf('.');
  return index < 0 ? '' : path.substring(index);
}
function schemename (path) {
  var s = /^[a-z][a-z0-9]*:/(path);
  return s && s[0] || "";
}

// bootstrap the module module
// this is where the default node module-loading logic is.
function bootstrap () {
  // register the default handlers for .js, .node, and http modules.
  registerExtension(".js", "load", function (cb) {
    var fs = exports.requireNative("fs"),
      filename = this.uri;
    if (cb) {
      fs.readFile(filename, cb);
    } else {
      return fs.readFileSync(filename);
    }
  });
  registerExtension(".js", "compile", function (content, cb) {
    content = content.replace(/^\#\!.*/, '');
    var wrapper = "(function (exports, require, module, __filename, __dirname) { "
                + content
                + "\n});";
    if (this.filename === process.argv[1]) {
      process.checkBreak();
    }
    try {
      process.compile(wrapper, this.filename)(
        this.exports,
        this.require,
        this,
        this.filename,
        dirname(this.filename)
      );
    } catch (ex) {
      if (cb) return cb(ex);
      else throw ex;
    }
    if (cb) {
      this.waitForChildren(cb);
    } else {
      this.loaded = true;
      return this.exports;
    }
  });

  // the module is already compiled, so compiling it to a JS function is unnecessary.
  exports.registerExtension(".node", "load", function (filename, cb) {
    // XXX Not yet supporting loading from HTTP. would need to download the
    // file, store it to tmp then run dlopen on it.
    process.dlopen(filename, this.exports); // FIXME synchronus
    if (cb) cb(null, this.exports);
    return this.exports;
  });

  // TODO: This should download the file to a temp location, and then
  // somehow signal that the downloaded file should be loaded.
  exports.registerScheme("http:", "resolve", function (url, parent, cb) {
    if (!cb) throw new Error(
      "Sync require() not allowed for remote modules");
    if (path.extname(url) === ".node") {
      // TODO: download the file to a temp folder, and then
      // cb() with the temp file location.  Keep a cache of
      // url->tempfile to avoid unnecessary fetching.
      throw new Error(
        "Remote-loading compiled modules is not (yet) supported.");
    }
    if (cb) cb(null, url)
    return url;
  });
  exports.registerScheme("http:", "load", function (cb) {
    require("http").cat(this.uri, cb);
  });
  
  var path = requireNative("path");

  if (process.env["HOME"]) {
    exports.paths.unshift(path.join(process.env["HOME"], ".node_libraries"));
  }

  if (process.env["NODE_PATH"]) {
    exports.paths = process.env["NODE_PATH"].split(":").concat(exports.paths);
  }

}

bootstrap();
