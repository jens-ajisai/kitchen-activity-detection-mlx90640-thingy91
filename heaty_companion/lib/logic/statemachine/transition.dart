import 'dart:ui';

import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:heaty_companion/logic/statemachine/state.dart';

class Transition {
  Event trigger;
  State from;
  State to;
  VoidCallback? onTransition;

  Transition({required this.trigger, required this.from, required this.to, this.onTransition});
}
