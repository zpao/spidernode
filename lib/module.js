
// this is the file that loads all the other files.

// TODO:
// async require(), with child waiting
// registerSchema

exports.require = require;
exports.requireNative = requireNative;
exports.Module = Module;
exports.paths = [];
exports.registerExtension = registerExtension;
exports.extensionCache = {};
exports.registerProtocol = registerProtocol;
exports.protocolCache = {};

// This contains the source code for the files in lib/
// Like, natives.fs is the contents of lib/fs.js
var natives = process.binding('natives'),
  nativeModuleCache = { module : exports };

// this is actually always sync, since the modules are preloaded in the binding.
// the cb is just for API parallelness.
function requireNative (id, cb) {
  var mod = new Module(id);
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
  for (var i in mod) {
    process.stdio.writeError(i+": "+mod[i]+"\n");
  }
  var data = nativeModuleCache[id] = mod.compile(natives[id]);
  if (cb) cb(null, data);
  return data;
}


// first pass, just sync, just js, just local
function require (id, parent) {

  // Prefer native modules always.
  if (id in natives) return requireNative(id);

  if (parent && parent.uri) {
    var base = parent.uri;
  } else {
    var base = "";
  }
  var uri = resolve(base, id);
  if (!uri) throw new Error(
    "Module "+id+" could not be found");
  if (parent && uri in parent.moduleCache) {
    var child = parent.moduleCache[uri];
    child.locked = true;
    return child.exports;
  }
  var child = new exports.Module(uri, parent);
  var data = child.load();
  if ("string" === typeof data) {
    data = child.compile(data);
  }
  if ("string" === typeof data && child.ext !== ".js") {
    // if it's not already JS, it can compile to a JS string.
    child.compile(data, ".js");
  }
  if (parent) {
    parent.moduleCache[uri] = child;
  }
  child.moduleCache[uri] = child;
  return child.exports;
}

// search the paths for id, and then return the resolved filename.
function resolve (root, id, cb) {
  var searchPaths = exports.paths,
    filenames = [],
    f = 0,
    path = requireNative("path"),
    fs = requireNative("fs");

  process.stdio.writeError("resolve: "+root+" "+id+"\n");
  if (!id) throw new Error("Invalid module id: "+id);
  
  if (id.substr(0, 7) === "file://") id = id.substr(7);
  else if (id.substr(0, 5) === "file:") id = id.substr(5);

  if (id.charAt(0) === "/") {
    // absolute.
    searchPaths = [""];
  } else if (id.charAt(0) === '.') {
    searchPaths = [ dirname(root) ];
  }

  for (var i = 0, l = searchPaths.length; i < l; i ++) {
    var searchPath = searchPaths[i].replace(/\/+$/, '');
    var exts = Object.keys(exports.extensionCache);
    for (var j = 0, m = exts.length; j < m; j ++) {
      var ext = exts[j];
      filenames[f++] = path.join(searchPath, id+ext);
      filenames[f++] = path.join(searchPath, id+"/index"+ext);
    }
  }
  process.stdio.writeError("search for "+id+" in these places: \n"+filenames.join("\n")+"\n");

  // now filenames is the list of files to check for.
  if (cb) { // async
    (function L (i) {
      if (i === filenames.length) {
        return cb(new Error("Module "+id+" not found"));
      }
      exists(filenames[i], function (e) {
        if (!e) return L(i+1);
        cb(null, filenames[i]);
      });
    })(0);
    return;
  }
  // sync
  for (var i = 0, l = filenames.length; i < l; i ++) {
    if (existsSync(filenames[i])) {
      process.stdio.writeError("found: "+filenames[i]+"\n");
      return filenames[i];
    }
  }
  throw new Error("Module "+id+" not found");
}

function Module (uri, parent) {
  this.uri = this.filename = uri;

  // legacy, delete
  this.id = this.filename.replace(/\.[^\.]+$/, '');

  process.stdio.writeError("module - "+this.filename+"\n");
  this.ext = extname(uri || "") || ".js";
  this.protocol = protocolname(uri);
  
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
    this.parent.children.push(this);
    this.moduleCache = parent.moduleCache;
  } else {
    this.moduleCache = {};
  }
  
  this.moduleCache[ uri ] = this;

  this.loaded = false;
  this.exited = false;
  this.children = [];

  // bind require's context
  var self = this;
  this.require = function (id) { return require(id, self) };
  this.require.async = function (id, cb) { return require(id, self, cb) };
  this.require.paths = exports.paths;
  this.require.registerExtension = registerExtension;
  this.require.registerProtocol = registerProtocol;
}
Module.prototype.load = function (cb) {
  // use the loader registered for this extension
  var loader = exports.extensionCache[ this.ext ];
  loader = loader && loader.load;
  if (!loader) {
    var er = new Error(
      "No loader registered for "+this.ext+" modules");
    if (cb) cb(er);
    else throw er;
    return er;
  }
  this.load = loader;
  return this.load(cb);
}
Module.prototype.compile = function (code, cb) {
  var compiler = exports.extensionCache[ this.ext ];
  compiler = compiler && compiler.compile;
  if (!compiler) {
    var er = new Error(
      "No compiler registered for "+this.ext+" modules");
    if (cb) cb(er);
    else throw er;
    return er;
  }
  this.compile = compiler;
  return this.compile(code, cb);
}

function registerProtocol (protocol, fetcher) {
  throw new Error("NYI");
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
  process.stdio.writeError("registerExtension "+ext+" "+phase+"\n"+compiler+"\n");


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
    process.stdio.writeError(path+" not found: "+e.stack+"\n");
    return false;
  }
  return true;
}
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
function protocolname (path) {
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
      process.stdio.writeError("readFileSync("+filename+")\n");
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
    process.stdio.writeError("compiling: "+wrapper);
    process.compile(wrapper, this.filename)(
      this.exports,
      this.require,
      this,
      this.filename,
      dirname(this.filename)
    );
    
    return this.exports;
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
  // exports.registerProtocol("http:", function (uri, cb) {
  //   if (!cb) throw new Error(
  //     "Sync require() not allowed for remote modules");
  //   if (path.extname(url) === ".node") {
  //     throw new Error(
  //       "Remote-loading compiled modules is not (yet) supported.");
  //   }
  //   require("http").cat(url, cb);
  // });
  
  var path = requireNative("path");

  if (process.env["HOME"]) {
    exports.paths.unshift(path.join(process.env["HOME"], ".node_libraries"));
  }

  if (process.env["NODE_PATH"]) {
    exports.paths = process.env["NODE_PATH"].split(":").concat(exports.paths);
  }

}

bootstrap();
