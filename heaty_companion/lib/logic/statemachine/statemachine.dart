import 'dart:async';

import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:heaty_companion/logic/statemachine/state.dart';
import 'package:heaty_companion/logic/statemachine/transition.dart';
import 'package:simple_logger/simple_logger.dart';

final logger = SimpleLogger();

class Statemachine {
  List<State> states;
  List<Transition> transitions;

  late State _currentState;
  StreamController<State> stateChangeNotifier = StreamController<State>();

  Statemachine({required this.states, required this.transitions, required State initState}) {
    _currentState = initState;
    if (_currentState.onEnter != null) {
      _currentState.onEnter!();
    }
  }

  void start(Stream<Event> stream) async {
    await for (final value in stream) {
      logger.info("Handle event ${value.name}");
      _handleEvent(value);
    }
  }

  State getState() {
    return _currentState;
  }

  void _handleEvent(Event ev) {
    for (var t in transitions) {
      if (ev == t.trigger && _currentState == t.from) {
        if (_currentState.onExit != null) {
          _currentState.onExit!();
        }

        _currentState = t.to;
        if (t.onTransition != null) {
          t.onTransition!();
        }

        stateChangeNotifier.add(_currentState);
        if (_currentState.onEnter != null) {
          _currentState.onEnter!();
        }
      }
    }
  }
}
