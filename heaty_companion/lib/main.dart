import 'package:heaty_companion/logic/BleStatemachine.dart';
import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:heaty_companion/views/bluetooth_off_screen.dart';
import 'package:heaty_companion/views/bluetooth_on_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';
import 'package:provider/provider.dart';

const String heatyName = "Heaty";

void main() {
  runApp(
    ChangeNotifierProvider<BleStatemachine>(
      create: (_) => BleStatemachine(),
      child: const HeatyCompanionApp(),
    ),
  );
}

class HeatyCompanionApp extends StatefulWidget {
  const HeatyCompanionApp({Key? key}) : super(key: key);

  @override
  _HeatyCompanionAppState createState() => _HeatyCompanionAppState();
}

class _HeatyCompanionAppState extends State<HeatyCompanionApp> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Heaty Companion',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: StreamBuilder<BluetoothState>(
          stream: FlutterBlue.instance.state,
          initialData: BluetoothState.on,
          builder: (c, snapshot) {
            final state = snapshot.data;
            if (state == BluetoothState.on) {
              Events.eventHandler.add(Events.evScan);
              return FindDevicesScreen();
            }
            return BluetoothOffScreen(state: state);
          }),
    );
  }
}
