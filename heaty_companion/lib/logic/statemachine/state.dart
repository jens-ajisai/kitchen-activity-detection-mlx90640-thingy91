import 'package:flutter/material.dart';

class State {
  String name;
  VoidCallback? onEnter;
  VoidCallback? onExit;

  State({required this.name, this.onEnter, this.onExit});
}
