import 'package:flutter_test/flutter_test.dart';
import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:heaty_companion/logic/statemachine/state.dart';
import 'package:heaty_companion/logic/statemachine/statemachine.dart';
import 'package:heaty_companion/logic/statemachine/transition.dart';
import 'package:simple_logger/simple_logger.dart';

void main() {
  final logger = SimpleLogger();

  test('Counter value should be incremented', () async {
    Event evScan = Event(name: "scan");
    Event evConnect = Event(name: "connect");
    Event evConnected = Event(name: "connected");
    Event evDiscover = Event(name: "discover");
    Event evFinishDiscovery = Event(name: "discovered");
    Event evFinishScan = Event(name: "finish");
    Event evDisconnect = Event(name: "disconnect");

    State idle = State(
        name: "Idle",
        onEnter: () {
          logger.info("onEnter Idle");
        },
        onExit: () {
          logger.info("onExit Idle");
        });
    State scanning = State(
        name: "Scanning",
        onEnter: () async {
          logger.info("start scanning");
          await Future.delayed(const Duration(seconds: 3), () {
            logger.info("add event evFinishScan");
            Events.eventHandler.add(evFinishScan);
          });
        });
    State connecting = State(
        name: "Connecting",
        onEnter: () {
          logger.info("onEnter Connecting");
        },
        onExit: () {
          logger.info("onExit Connecting");
        });
    State connected = State(
        name: "Connected",
        onEnter: () {
          logger.info("onEnter Connected");
        },
        onExit: () {
          logger.info("onExit Connected");
        });
    State searching = State(
        name: "Searching",
        onEnter: () {
          logger.info("onEnter searching");
        },
        onExit: () {
          logger.info("onExit searching");
        });

    List<Transition> transitions = [
      Transition(trigger: evScan, from: idle, to: scanning),
      Transition(trigger: evFinishScan, from: scanning, to: idle),
      Transition(trigger: evConnect, from: idle, to: connecting),
      Transition(trigger: evConnect, from: scanning, to: connecting),
      Transition(trigger: evConnected, from: connecting, to: connected),
      Transition(trigger: evDiscover, from: connected, to: searching),
      Transition(trigger: evFinishDiscovery, from: searching, to: connected),
      Transition(trigger: evDisconnect, from: connected, to: idle),
      Transition(trigger: evDisconnect, from: searching, to: idle),
      Transition(trigger: evDisconnect, from: connecting, to: idle),
    ];

    final statemachine = Statemachine(
        states: [idle, scanning, connecting, connected, searching], transitions: transitions, initState: idle);

    statemachine.start(Events.eventHandler.stream);
    Events.eventHandler.add(evScan);

    await Future.delayed(const Duration(seconds: 5), () {});
  });
}
