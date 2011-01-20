#include <node_http_parser.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <http_parser.h>

#include <strings.h>  /* strcasecmp() */
#include <string.h>  /* strdup() */
#include <stdlib.h>  /* free() */

// This is a binding to http_parser (http://github.com/ry/http-parser)
// The goal is to decouple sockets from parsing for more javascript-level
// agility. A Buffer is read from a socket and passed to parser.execute().
// The parser then issues callbacks with slices of the data
//     parser.onMessageBegin
//     parser.onPath
//     parser.onBody
//     ...
// No copying is performed when slicing the buffer, only small reference
// allocations.


namespace node {

using namespace v8;

static Persistent<String> delete_sym;
static Persistent<String> get_sym;
static Persistent<String> head_sym;
static Persistent<String> post_sym;
static Persistent<String> put_sym;
static Persistent<String> connect_sym;
static Persistent<String> options_sym;
static Persistent<String> trace_sym;
static Persistent<String> copy_sym;
static Persistent<String> lock_sym;
static Persistent<String> mkcol_sym;
static Persistent<String> move_sym;
static Persistent<String> propfind_sym;
static Persistent<String> proppatch_sym;
static Persistent<String> unlock_sym;
static Persistent<String> report_sym;
static Persistent<String> mkactivity_sym;
static Persistent<String> checkout_sym;
static Persistent<String> merge_sym;
static Persistent<String> msearch_sym;
static Persistent<String> notify_sym;
static Persistent<String> subscribe_sym;
static Persistent<String> unsubscribe_sym;
static Persistent<String> unknown_method_sym;

static Persistent<String> method_sym;
static Persistent<String> status_code_sym;
static Persistent<String> http_version_sym;
static Persistent<String> version_major_sym;
static Persistent<String> version_minor_sym;
static Persistent<String> should_keep_alive_sym;
static Persistent<String> upgrade_sym;


static inline Persistent<String>
method_to_str(unsigned short m) {
  switch (m) {
    case HTTP_DELETE:     return delete_sym;
    case HTTP_GET:        return get_sym;
    case HTTP_HEAD:       return head_sym;
    case HTTP_POST:       return post_sym;
    case HTTP_PUT:        return put_sym;
    case HTTP_CONNECT:    return connect_sym;
    case HTTP_OPTIONS:    return options_sym;
    case HTTP_TRACE:      return trace_sym;
    case HTTP_COPY:       return copy_sym;
    case HTTP_LOCK:       return lock_sym;
    case HTTP_MKCOL:      return mkcol_sym;
    case HTTP_MOVE:       return move_sym;
    case HTTP_PROPFIND:   return propfind_sym;
    case HTTP_PROPPATCH:  return proppatch_sym;
    case HTTP_UNLOCK:     return unlock_sym;
    case HTTP_REPORT:     return report_sym;
    case HTTP_MKACTIVITY: return mkactivity_sym;
    case HTTP_CHECKOUT:   return checkout_sym;
    case HTTP_MERGE:      return merge_sym;
    case HTTP_MSEARCH:    return msearch_sym;
    case HTTP_NOTIFY:     return notify_sym;
    case HTTP_SUBSCRIBE:  return subscribe_sym;
    case HTTP_UNSUBSCRIBE:return unsubscribe_sym;
    default:              return unknown_method_sym;
  }
}


#define ELEMENTS_TOTAL 1000
static struct http_parser_data elements[ELEMENTS_TOTAL];


class Parser : public ObjectWrap {
 public:
  Parser(enum http_parser_type type) : ObjectWrap() {
    Init(type);
  }


  ~Parser() {
  }


  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    String::Utf8Value type(args[0]->ToString());

    Parser *parser;

    if (0 == strcasecmp(*type, "request")) {
      parser = new Parser(HTTP_REQUEST);
    } else if (0 == strcasecmp(*type, "response")) {
      parser = new Parser(HTTP_RESPONSE);
    } else {
      return ThrowException(Exception::Error(
            String::New("Constructor argument be 'request' or 'response'")));
    }

    parser->Wrap(args.This());

    return args.This();
  }

  // var bytesParsed = parser->execute(buffer, off, len);
  static Handle<Value> Execute(const Arguments& args) {
    HandleScope scope;

    Parser *parser = ObjectWrap::Unwrap<Parser>(args.This());

    Local<Value> buffer_v = args[0];

    if (!Buffer::HasInstance(buffer_v)) {
      return ThrowException(Exception::TypeError(
            String::New("Argument should be a buffer")));
    }

    Local<Object> buffer_obj = buffer_v->ToObject();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_len = Buffer::Length(buffer_obj);

    size_t off = args[1]->Int32Value();
    if (off >= buffer_len) {
      return ThrowException(Exception::Error(
            String::New("Offset is out of bounds")));
    }

    size_t len = args[2]->Int32Value();
    if (off+len > buffer_len) {
      return ThrowException(Exception::Error(
            String::New("Length is extends beyond buffer")));
    }

    size_t nelements = http_parser_execute2(&parser->parser_,
                                            buffer_data + off,
                                            len,
                                            elements,
                                            ELEMENTS_TOTAL);

    // The result array has three elements for each element returned First
    // is a description string of the type. The next two depend on the type.
    Local<Array> result = Array::New(3 * nelements);

#define JS_OFFSET(el) Integer::New(el.payload.string.p - buffer_data)
#define JS_LEN(el) Integer::New(el.payload.string.len)

    for (int i = 0; i < nelements; i++) {
      switch (elements[i].type) {
        case HTTP_PARSER_ERROR:
          result->Set(3*i, String::New("ERROR"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, Integer::New(0));
          break;

        case HTTP_NEEDS_INPUT:
          result->Set(3*i, String::New("NEEDS_INPUT"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, Integer::New(0));
          break;

        case HTTP_NEEDS_DATA_ELEMENTS:
          result->Set(3*i, String::New("NEEDS_DATA_ELEMENTS"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, Integer::New(0));
          break;

        case HTTP_REQ_MESSAGE_START:
          result->Set(3*i, String::New("REQ_MESSAGE_START"));
          result->Set(3*i + 1, method_to_str(elements[i].payload.method));
          result->Set(3*i + 2, Null());
          break;

        case HTTP_RES_MESSAGE_START: /* payload.status */
          result->Set(3*i, String::New("RES_MESSAGE_START"));
          result->Set(3*i + 1, Integer::New(elements[i].payload.status_code));
          result->Set(3*i + 2, Null());
          break;

        case HTTP_VERSION: /* payload.version */
          result->Set(3*i, String::New("VERSION"));
          result->Set(3*i + 1, Integer::New(elements[i].payload.version.major));
          result->Set(3*i + 2, Integer::New(elements[i].payload.version.minor));
          break;

        case HTTP_PATH: /* payload.string */
          result->Set(3*i, String::New("PATH"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_QUERY_STRING: /* payload.string */
          result->Set(3*i, String::New("QUERY_STRING"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_FRAGMENT: /* payload.string */
          result->Set(3*i, String::New("FRAGMENT"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_URL: /* payload.string */
          result->Set(3*i, String::New("URL"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_HEADER_FIELD: /* payload.string */
          result->Set(3*i, String::New("HEADER_FIELD"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_HEADER_VALUE: /* payload.string */
          result->Set(3*i, String::New("HEADER_VALUE"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_HEADERS_END: /* payload.flags */
          // need info about upgrades
          result->Set(3*i, String::New("HEADERS_END"));
          result->Set(3*i + 1, Null());
          result->Set(3*i + 2, Null());
          break;

        case HTTP_BODY: /* payload.string */
          result->Set(3*i, String::New("BODY"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, JS_LEN(elements[i]));
          break;

        case HTTP_MESSAGE_END: /* payload.string */
          result->Set(3*i, String::New("MESSAGE_END"));
          result->Set(3*i + 1, JS_OFFSET(elements[i]));
          result->Set(3*i + 2, Integer::New(0));
          break;
      }
    }

#undef JS_OFFSET
#undef JS_LEN

    return scope.Close(result);
  }


  static Handle<Value> Reinitialize(const Arguments& args) {
    HandleScope scope;
    Parser *parser = ObjectWrap::Unwrap<Parser>(args.This());

    String::Utf8Value type(args[0]->ToString());

    if (0 == strcasecmp(*type, "request")) {
      parser->Init(HTTP_REQUEST);
    } else if (0 == strcasecmp(*type, "response")) {
      parser->Init(HTTP_RESPONSE);
    } else {
      return ThrowException(Exception::Error(
            String::New("Argument be 'request' or 'response'")));
    }
    return Undefined();
  }


 private:

  void Init (enum http_parser_type type) {
    http_parser_init(&parser_, type);
    parser_.data = this;
  }

  http_parser parser_;
};


void InitHttpParser(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Parser::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("HTTPParser"));

  NODE_SET_PROTOTYPE_METHOD(t, "execute", Parser::Execute);
  NODE_SET_PROTOTYPE_METHOD(t, "reinitialize", Parser::Reinitialize);

  target->Set(String::NewSymbol("HTTPParser"), t->GetFunction());

  delete_sym = NODE_PSYMBOL("DELETE");
  get_sym = NODE_PSYMBOL("GET");
  head_sym = NODE_PSYMBOL("HEAD");
  post_sym = NODE_PSYMBOL("POST");
  put_sym = NODE_PSYMBOL("PUT");
  connect_sym = NODE_PSYMBOL("CONNECT");
  options_sym = NODE_PSYMBOL("OPTIONS");
  trace_sym = NODE_PSYMBOL("TRACE");
  copy_sym = NODE_PSYMBOL("COPY");
  lock_sym = NODE_PSYMBOL("LOCK");
  mkcol_sym = NODE_PSYMBOL("MKCOL");
  move_sym = NODE_PSYMBOL("MOVE");
  propfind_sym = NODE_PSYMBOL("PROPFIND");
  proppatch_sym = NODE_PSYMBOL("PROPPATCH");
  unlock_sym = NODE_PSYMBOL("UNLOCK");
  report_sym = NODE_PSYMBOL("REPORT");
  mkactivity_sym = NODE_PSYMBOL("MKACTIVITY");
  checkout_sym = NODE_PSYMBOL("CHECKOUT");
  merge_sym = NODE_PSYMBOL("MERGE");
  msearch_sym = NODE_PSYMBOL("M-SEARCH");
  notify_sym = NODE_PSYMBOL("NOTIFY");
  subscribe_sym = NODE_PSYMBOL("SUBSCRIBE");
  unsubscribe_sym = NODE_PSYMBOL("UNSUBSCRIBE");;
  unknown_method_sym = NODE_PSYMBOL("UNKNOWN_METHOD");

  method_sym = NODE_PSYMBOL("method");
  status_code_sym = NODE_PSYMBOL("statusCode");
  http_version_sym = NODE_PSYMBOL("httpVersion");
  version_major_sym = NODE_PSYMBOL("versionMajor");
  version_minor_sym = NODE_PSYMBOL("versionMinor");
  should_keep_alive_sym = NODE_PSYMBOL("shouldKeepAlive");
  upgrade_sym = NODE_PSYMBOL("upgrade");
}

}  // namespace node

NODE_MODULE(node_http_parser, node::InitHttpParser);
