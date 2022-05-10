import 'dart:async';

class Events {
  static Event evError = Event(name: "error");
  static Event evInitialized = Event(name: "initialized");
  static Event evReinit = Event(name: "reinit");
  static Event evScan = Event(name: "scan");
  static Event evConnect = Event(name: "connect");
  static Event evConnected = Event(name: "connected");
  static Event evDiscover = Event(name: "discover");
  static Event evDiscovered = Event(name: "discovered");
  static Event evFoundTarget = Event(name: "finish");
  static Event evDisconnect = Event(name: "disconnect");
  static Event evDisconnected = Event(name: "disconnected");

  static StreamController<Event> eventHandler = StreamController<Event>();
}

class Event {
  String name;
  List<dynamic> parameters = [];

  Event({required this.name, parameters}) {
    if (parameters != null) {
      this.parameters = parameters;
    }
  }
}
