import 'package:flutter/material.dart';
import 'package:graphic/graphic.dart';
import 'package:heaty_companion/logic/BleStatemachine.dart';
import 'package:heaty_companion/logic/statemachine/event.dart';
import 'package:provider/provider.dart';
import 'package:simple_logger/simple_logger.dart';

class FindDevicesScreen extends StatelessWidget {
  FindDevicesScreen({Key? key}) : super(key: key);
  final logger = SimpleLogger();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Heaty Companion'),
      ),
      body: SingleChildScrollView(
        child: Container(
          margin: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 20.0),
          child: Column(
            children: <Widget>[
              // status and next action button
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  MaterialButton(
                      color: Colors.lightBlueAccent,
                      child: Text("Connect"),
                      onPressed: () {
                        Events.eventHandler.add(Events.evConnect);
                      }),
                  MaterialButton(
                      color: Colors.lightBlueAccent,
                      child: Text("Set Time"),
                      onPressed: () {
                        Provider.of<BleStatemachine>(context, listen: false).setTime();
                      }),
                  MaterialButton(
                      color: Colors.lightBlueAccent,
                      child: Text("Disconnect"),
                      onPressed: () {
                        Events.eventHandler.add(Events.evDisconnect);
                      }),
                  MaterialButton(
                      color: Colors.lightBlueAccent,
                      child: Text("Scan"),
                      onPressed: () {
                        Events.eventHandler.add(Events.evScan);
                      }),
                  Text(Provider.of<BleStatemachine>(context, listen: true).state),
                  (Provider.of<BleStatemachine>(context, listen: true).heatMapTxSubscription != null)
                      ? Text("Listening")
                      : Text("not Listening"),
                ],
              ),
              (Provider.of<BleStatemachine>(context, listen: true).targetDevice != null)
                  ? Text(Provider.of<BleStatemachine>(context, listen: true).targetDevice!.name +
                      " " +
                      Provider.of<BleStatemachine>(context, listen: true).targetDevice!.id.id)
                  : Container(),

              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  if (Provider.of<BleStatemachine>(context, listen: true).dataCameraImage.incommingData.isNotEmpty)
                    Text(
                        'Received cameraImage bytes${Provider.of<BleStatemachine>(context, listen: true).dataCameraImage.transferredBytes}'),
                ],
              ),
              (Provider.of<BleStatemachine>(context, listen: true).dataCameraImage.imageData.isNotEmpty)
                  ? Image(
                      image: MemoryImage(Provider.of<BleStatemachine>(context, listen: true).dataCameraImage.imageData),
                    )
                  : const Text("No Image"),

              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  if (Provider.of<BleStatemachine>(context, listen: true).dataHeatMap.incommingData.isNotEmpty)
                    Text(
                        'Received heatMap bytes${Provider.of<BleStatemachine>(context, listen: true).dataHeatMap.transferredBytes}'),
                ],
              ),
              (Provider.of<BleStatemachine>(context, listen: true).dataHeatMap.heatmapData.isNotEmpty)
                  ? Container(
                      margin: const EdgeInsets.only(top: 10),
                      width: 350,
                      height: 300,
                      child: Chart(
                        data: Provider.of<BleStatemachine>(context, listen: false).dataHeatMap.heatmapData,
                        variables: {
                          'row': Variable(
                            accessor: (List datum) => datum[0].toString(),
                          ),
                          'column': Variable(
                            accessor: (List datum) => datum[1].toString(),
                          ),
                          'sales': Variable(
                            accessor: (List datum) => datum[2] as num,
                            // TODO try  scale: LinearScale(min: 6, max: 9),
                          ),
                        },
                        elements: [
                          PolygonElement(
                            color: ColorAttr(variable: 'sales', values: [Colors.blue, Colors.orange, Colors.red]),
                          ),
                        ],
                        axes: [
                          Defaults.horizontalAxis,
                          Defaults.verticalAxis,
                        ],
                        selections: {'tap': PointSelection()},
                        tooltip: TooltipGuide(),
                      ),
                    )
                  : const Text("No Image"),
              Text("Settings", style: Theme.of(context).textTheme.headline2),
              ListTile(
                  title: const Text("HeatMap Interval"),
                  subtitle: Slider(
                    min: 10,
                    max: 300,
                    divisions: 100,
                    label:
                        Provider.of<BleStatemachine>(context, listen: false).heatMapInterval.toInt().toString() + "s",
                    onChanged: (value) {
                      Provider.of<BleStatemachine>(context, listen: false).heatMapInterval = value;
                      Provider.of<BleStatemachine>(context, listen: false).notify();
                    },
                    value: Provider.of<BleStatemachine>(context, listen: true).heatMapInterval,
                  ),
                  trailing: TextButton(
                      child: const Text("Apply"),
                      onPressed: () {
                        Provider.of<BleStatemachine>(context, listen: false).setHeatMapInterval();
                      })),
              ListTile(
                  title: const Text("Camera Interval"),
                  subtitle: Slider(
                    min: 10,
                    max: 300,
                    divisions: 100,
                    label: Provider.of<BleStatemachine>(context, listen: false).cameraImageInterval.toInt().toString() +
                        "s",
                    onChanged: (value) {
                      Provider.of<BleStatemachine>(context, listen: false).cameraImageInterval = value;
                      Provider.of<BleStatemachine>(context, listen: false).notify();
                    },
                    value: Provider.of<BleStatemachine>(context, listen: true).cameraImageInterval,
                  ),
                  trailing: TextButton(
                      child: const Text("Apply"),
                      onPressed: () {
                        Provider.of<BleStatemachine>(context, listen: false).setCameraImageInterval();
                      })),
            ],
          ),
        ),
      ),
    );
  }
}
