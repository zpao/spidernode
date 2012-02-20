#ifndef v8_v8_debug_h__
#define v8_v8_debug_h__

#include "v8.h"

namespace v8 {

enum DebugEvent {
  Break = 1,
  Exception = 2,
  NewFunction = 3,
  BeforeCompile = 4,
  AfterCompile = 5,
  ScriptCollected = 6,
  BreakForCommand = 7
};

class Debug
{
public:
  class ClientData
  {
  public:
    virtual ~ClientData() { }
  };

  class Message
  {
  public:
    virtual bool IsEvent() const = 0;
    virtual bool IsResponse() const = 0;
    virtual DebugEvent GetEvent() const = 0;
    virtual bool WillStartRunning() const = 0;
    virtual Handle<Object> GetExecutionState() const = 0;
    virtual Handle<Object> GetEventData() const = 0;
    virtual Handle<String> GetJSON() const = 0;
    virtual Handle<Context> GetEventContext() const = 0;
    virtual ClientData* GetClientData() const = 0;
    virtual ~Message() { }
  };

  class EventDetails
  {
  public:
    virtual DebugEvent GetEvent() const = 0;
    virtual Handle<Object> GetExecutionState() const = 0;
    virtual Handle<Object> GetEventData() const = 0;
    virtual Handle<Context> GetEventContext() const = 0;
    virtual Handle<Value> GetCallbackData() const = 0;
    virtual ClientData* GetClientData() const = 0;
    virtual ~EventDetails() { }
  };

  typedef void (*EventCallback)(DebugEvent event, Handle<Object> exec_state, Handle<Object> event_data, Handle<Value> data);
  typedef void (*EventCallback2)(const EventDetails& event_details);
  typedef void (*MessageHandler)(const uint16_t* message, int length, ClientData* client_data);
  typedef void (*MessageHandler2)(const Message& message);
  typedef void (*HostDispatchHandler)();
  typedef void (*DebugMessageDispatchHandler)();

  static bool SetDebugEventListener(EventCallback that, Handle<Value> data = Handle<Value>()) {
    UNIMPLEMENTEDAPI(false);
  }
  static bool SetDebugEventListener2(EventCallback2 that, Handle<Value> data = Handle<Value>()) {
    UNIMPLEMENTEDAPI(false);
  }
  static bool SetDebugEventListener(Handle<Object> that, Handle<Value> data = Handle<Value>()) {
    UNIMPLEMENTEDAPI(false);
  }

  static void DebugBreak() {
    UNIMPLEMENTEDAPI();
  }
  static void CancelDebugBreak() {
    UNIMPLEMENTEDAPI();
  }
  static void DebugBreakForCommand(ClientData* data = NULL) {
    UNIMPLEMENTEDAPI();
  }

  static void SetMessageHandler(MessageHandler handler, bool message_handler_thread = false) {
    UNIMPLEMENTEDAPI();
  }
  static void SetMessageHandler2(MessageHandler2 handler) {
    UNIMPLEMENTEDAPI();
  }
  static void SendCommand(const uint16_t command, int length, ClientData* client_data = NULL) {
    UNIMPLEMENTEDAPI();
  }

  static void SetHostDispatchHandler(HostDispatchHandler handler, int period = 100) {
    UNIMPLEMENTEDAPI();
  }
  static void SetDebugMessageDispatchHandler(DebugMessageDispatchHandler handler, bool provide_locker = false) {
    UNIMPLEMENTEDAPI();
  }

  static Local<Value> Call(Handle<Function> fun, Handle<Value> data = Handle<Value>()) {
    UNIMPLEMENTEDAPI(Local<Value>());
  }
  static Local<Value> GetMirror(Handle<Value> obj) {
    UNIMPLEMENTEDAPI(Local<Value>());
  }

  static bool EnableAgent(const char* name, int port, bool wait_for_connection = false) {
    UNIMPLEMENTEDAPI(false);
  }

  static void ProcessDebugMessages() {
    UNIMPLEMENTEDAPI();
  }

  static Local<Context> GetDebugContext() {
    UNIMPLEMENTEDAPI(Local<Context>());
  }

};

} // namespace v8

#endif // v8_v8_debug_h__
