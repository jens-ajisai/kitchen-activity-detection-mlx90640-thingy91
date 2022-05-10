import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart' hide State;
import 'package:flutter_blue/flutter_blue.dart';
import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:heaty_companion/logic/statemachine/state.dart';
import 'package:heaty_companion/logic/statemachine/statemachine.dart';
import 'package:heaty_companion/logic/statemachine/transition.dart';
import 'package:simple_logger/simple_logger.dart';

final logger = SimpleLogger();

// Next todo is the tx handling and disconnecting

class HeatMap {
  String incommingData = "";
  List<List<int>> heatmapData = [
    [0, 0, 0],
    [0, 1, 1],
    [1, 0, 2],
    [1, 1, 4]
  ];
  int transferredBytes = 0;

  reset() {
    incommingData = "";
    transferredBytes = 0;
  }
}

class CameraImage {
  String incommingData = "";
  Uint8List imageData = Uint8List(0);
  int transferredBytes = 0;
  String name = "";

  reset() {
    name = "";
    incommingData = "";
    transferredBytes = 0;
  }
}

class BleStatemachine with ChangeNotifier {
  late Statemachine statemachine;
  String state = "";
  FlutterBlue flutterBlue = FlutterBlue.instance;
  Set<BluetoothDevice> connectedBluetoothDevices = {};
  Set<BluetoothDevice> availableBluetoothDevices = {};
  StreamSubscription? scanSubscription;
  StreamSubscription? stateSubscription;
  StreamSubscription? heatMapTxSubscription;
  StreamSubscription? cameraImageTxSubscription;
  BluetoothDevice? targetDevice;
  List<BluetoothService> allServices = [];
  List<BluetoothService> targetServices = [];
  List<BluetoothCharacteristic> targetCharacteristics = [];

  HeatMap dataHeatMap = HeatMap();
  CameraImage dataCameraImage = CameraImage();

  final String heatyServiceUuid = "aaaf0900-c332-42a8-93bd-25e905756cb8";
  final String heatMapTxUuid = "aaaf0901-c332-42a8-93bd-25e905756cb8";
  final String heatMapIntervalUuid = "aaaf0902-c332-42a8-93bd-25e905756cb8";

  final String cameraServiceUuid = "caaf0900-c332-42a8-93bd-25e905756cb8";
  final String cameraTxUuid = "caaf090b-c332-42a8-93bd-25e905756cb8";
  final String cameraIntervalUuid = "caaf090a-c332-42a8-93bd-25e905756cb8";

  cancelSubscription(StreamSubscription? subscription) async {
    if (subscription != null) {
      await subscription.cancel();
      subscription = null;
    }
  }

  BluetoothCharacteristic getCharacteristic(
      List<BluetoothService> services, String serviceID, String characteristicID) {
    BluetoothService service = services.where((element) => element.uuid.toString() == serviceID).first;
    BluetoothCharacteristic char = service.characteristics.where((cc) => cc.uuid.toString() == characteristicID).first;
    logger.info("got characteristic" + char.toString());
    targetServices.add(service);
    targetCharacteristics.add(char);
    return char;
  }

  Future<void> processHeatmapData() async {
    try {
      logger.info(dataHeatMap.incommingData);
      final packedData = base64.decode(dataHeatMap.incommingData);
      logger.info('Unpack data');
      final rawHeatmapData = packedData.buffer.asFloat32List(0, packedData.length ~/ 4);

      List<List<int>> heatmapDataTmp = [];
      for (var h in Iterable<int>.generate(24).toList()) {
        for (var w in Iterable<int>.generate(32).toList()) {
          heatmapDataTmp.add([h, w, rawHeatmapData[h * 32 + w].toInt()]);
        }
      }
      dataHeatMap.heatmapData = heatmapDataTmp;
    } on Exception catch (_, e) {
      logger.severe(e);
      dataHeatMap.reset();
    }
    notifyListeners();
    dataHeatMap.reset();
  }

  Future<void> processCameraImageData() async {
    try {
      logger.info(dataCameraImage.incommingData);

      final headerSearch = RegExp(r'({.*?})');
      final match = headerSearch.firstMatch(dataCameraImage.incommingData);
      final matchedText = match?.group(1);
      if (matchedText != null) {
        final header = jsonDecode(matchedText);
        logger.info("Found Header $matchedText");
        dataCameraImage.incommingData = dataCameraImage.incommingData.replaceFirst(matchedText, "");

        dataCameraImage.imageData = base64.decode(dataCameraImage.incommingData);
        dataCameraImage.name = header["name"];
        if (dataCameraImage.imageData.length == header["size"]) {
          logger.info('Received image ${header["name"]}');
        }
      }
    } on Exception catch (_, e) {
      logger.severe(e);
      dataCameraImage.reset();
    }
    notifyListeners();
    dataCameraImage.reset();
  }

  void handleHeatMapTx(List<int> value) {
    dataHeatMap.transferredBytes += value.length;
    if (value.isNotEmpty) {
      if (value.last == 0) {
        dataHeatMap.incommingData += (String.fromCharCodes(value, 0, value.length - 1));
        processHeatmapData();
      } else {
        dataHeatMap.incommingData += (String.fromCharCodes(value));
      }
    }
  }

  void handleCameraImageTx(List<int> value) {
    dataCameraImage.transferredBytes += value.length;
    if (value.isNotEmpty) {
      if (value.last == 0) {
        dataCameraImage.incommingData += (String.fromCharCodes(value, 0, value.length - 1));
        processCameraImageData();
      } else {
        dataCameraImage.incommingData += (String.fromCharCodes(value));
      }
    }
  }

  Future<void> resetAll() async {
    if (targetDevice != null) {
      targetDevice!.disconnect();
    }
    connectedBluetoothDevices.clear();
    allServices.clear();
    targetServices.clear();
    targetCharacteristics.clear();
    if (scanSubscription != null) {
      await cancelSubscription(scanSubscription);
    }
    if (stateSubscription != null) {
      await cancelSubscription(stateSubscription);
    }
    if (heatMapTxSubscription != null) {
      await cancelSubscription(heatMapTxSubscription);
    }
    if (cameraImageTxSubscription != null) {
      await cancelSubscription(cameraImageTxSubscription);
    }
    // todo need to set this to false?
    //await txCharacteristic.setNotifyValue(false);
    targetDevice = null;
  }

  double heatMapInterval = 10;

  setHeatMapInterval() {
    BluetoothCharacteristic c = getCharacteristic(allServices, heatyServiceUuid, heatMapIntervalUuid);
    c.write([heatMapInterval.toInt()]);
  }

  double cameraImageInterval = 10;

  setCameraImageInterval() {
    BluetoothCharacteristic c = getCharacteristic(allServices, cameraServiceUuid, cameraIntervalUuid);
    c.write([heatMapInterval.toInt()]);
  }

  //-------------------------------------
  // Statemachine
  //-------------------------------------

  // TODO make these event parameters
  String targetName = "Heaty";

  BleStatemachine() {
    flutterBlue.setLogLevel(LogLevel.warning);

    State init = State(
        name: "Init",
        onEnter: () async {
          if (await flutterBlue.isOn) {
            Events.eventHandler.add(Events.evInitialized);
          } else {
            Events.eventHandler.add(Events.evError);
            Future.delayed(Duration(seconds: 2), () {
              Events.eventHandler.add(Events.evReinit);
            });
          }
          notifyListeners();
        },
        onExit: () {
          logger.info("onExit Init");
        });

    State idle = State(
        name: "Idle",
        onEnter: () async {
          logger.info("onEnter Idle");
          for (BluetoothDevice device in await flutterBlue.connectedDevices) {
            logger.info("Connected device: ${device.toString()}");
            connectedBluetoothDevices.add(device);
            // notifyListeners();
          }
          notifyListeners();
        },
        onExit: () {
          logger.info("onExit Idle");
        });
    State scanning = State(
        name: "Scanning",
        onEnter: () async {
          logger.info("onEnter Scanning");
          availableBluetoothDevices.clear();
          targetDevice = null;

          if (scanSubscription != null) {
            await cancelSubscription(scanSubscription);
          }
          if (!await flutterBlue.isScanning.first) {
            flutterBlue.startScan(timeout: const Duration(seconds: 120));
          }

          scanSubscription = flutterBlue.scanResults.listen((List<ScanResult> results) {
            for (ScanResult result in results) {
              logger.info("found device ${result.device.name}");
              if (result.device.name == targetName) {
                logger.info("found TARGET device ${result.device}");
                availableBluetoothDevices.add(result.device);
                // notifyListeners();
                flutterBlue.stopScan();
                targetDevice = result.device;
                Events.eventHandler.add(Events.evFoundTarget);
              }
            }
          });
          notifyListeners();
        });
    State connecting = State(
        name: "Connecting",
        onEnter: () async {
          logger.info("onEnter Connecting - to $targetDevice");
          if (targetDevice != null) {
            targetDevice!.connect();
            if (stateSubscription != null) {
              await cancelSubscription(stateSubscription);
            }
            stateSubscription = targetDevice!.state.listen((event) async {
              if (event == BluetoothDeviceState.connected) {
                logger.info("connected to target device");
                Events.eventHandler.add(Events.evConnected);

                connectedBluetoothDevices.add(targetDevice!);
                availableBluetoothDevices.remove(targetDevice!);
                //notify();
              }
              if (event == BluetoothDeviceState.disconnected) {
                Events.eventHandler.add(Events.evDisconnect); // TODO better disconnected?
              }
            });
            //notifyListeners();
          }
          notifyListeners();
        },
        onExit: () {
          logger.info("onExit Connecting");
        });
    State connected = State(
        name: "Connected",
        onEnter: () {
          logger.info("onEnter Connected");
          if (allServices.isEmpty) {
            Events.eventHandler.add(Events.evDiscover);
          } else {}
          notifyListeners();
        },
        onExit: () {
          logger.info("onExit Connected");
        });
    State discovering = State(
        name: "Discovering",
        onEnter: () async {
          logger.info("onEnter Discovering");
          targetServices.clear();
          targetCharacteristics.clear();
          allServices.clear();
          dataHeatMap.reset();
          dataCameraImage.reset();

          if (heatMapTxSubscription != null) {
            await cancelSubscription(heatMapTxSubscription);
          }
          if (cameraImageTxSubscription != null) {
            await cancelSubscription(cameraImageTxSubscription);
          }

          allServices = await targetDevice!.discoverServices();
          BluetoothCharacteristic heatMapTxCharacteristic =
              getCharacteristic(allServices, heatyServiceUuid, heatMapTxUuid);
          await heatMapTxCharacteristic.setNotifyValue(true);
          heatMapTxSubscription = heatMapTxCharacteristic.value.listen((value) async {
            handleHeatMapTx(value); // *** automatically subscribe for data!
          });
          BluetoothCharacteristic cameraImageTxCharacteristic =
              getCharacteristic(allServices, cameraServiceUuid, cameraTxUuid);
          await cameraImageTxCharacteristic.setNotifyValue(true);
          cameraImageTxSubscription = cameraImageTxCharacteristic.value.listen((value) async {
            handleCameraImageTx(value); // *** automatically subscribe for data!
          });
          Events.eventHandler.add(Events.evDiscovered);
          notifyListeners();
        },
        onExit: () {
          logger.info("onExit Discovering");
        });

    List<Transition> transitions = [
      Transition(trigger: Events.evReinit, from: init, to: init, onTransition: resetAll), // ok
      Transition(trigger: Events.evInitialized, from: init, to: idle), // ok
      Transition(trigger: Events.evScan, from: idle, to: scanning), // ok
      Transition(trigger: Events.evFoundTarget, from: scanning, to: idle), // ok
      Transition(trigger: Events.evConnect, from: idle, to: connecting), // ok
      Transition(trigger: Events.evConnect, from: scanning, to: connecting), //ok
      Transition(trigger: Events.evConnected, from: connecting, to: connected), //ok
      Transition(trigger: Events.evDiscover, from: connected, to: discovering), // ok
      Transition(trigger: Events.evDiscovered, from: discovering, to: connected),
      Transition(trigger: Events.evDisconnected, from: connected, to: idle, onTransition: resetAll),
      Transition(trigger: Events.evDisconnect, from: discovering, to: idle, onTransition: resetAll),
      Transition(trigger: Events.evDisconnect, from: connecting, to: idle, onTransition: resetAll),
      Transition(trigger: Events.evDisconnect, from: connected, to: idle, onTransition: resetAll),
    ];

    statemachine = Statemachine(
        states: [idle, scanning, connecting, connected, discovering], transitions: transitions, initState: init);

    statemachine.start(Events.eventHandler.stream);
    listenStateUpdate();
  }

  void listenStateUpdate() async {
    await for (final value in statemachine.stateChangeNotifier.stream) {
      logger.info("listenStateUpdate: ${value.name}");
      state = value.name;
      notifyListeners();
    }
  }

  void notify() {
    notifyListeners();
  }
}
