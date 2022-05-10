import asyncio
import logging
import sys

from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice
from bleak.uuids import uuid16_dict, uuid128_dict

import subprocess

UART_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
UART_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
UART_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

memfaultCommands = ""


async def main():

    found_device_queue = asyncio.Queue()

    def end():
        # cancelling all tasks effectively ends the program
        for task in asyncio.all_tasks():
            task.cancel()

    def filter_for_device(device: BLEDevice, adv: AdvertisementData):
        if device.name == "Heaty" or device.name == "BIRD_TABLE":
            return True
        return False

    def on_detection(device: BLEDevice, adv: AdvertisementData):
        logger.debug(f"{device.address} RSSI: {device.rssi}, {adv}")
        if filter_for_device(device, adv):
            found_device_queue.put_nowait(device)

    def handle_rx(_: int, data: bytearray):
        global memfaultCommands
        memfaultCommands += data.decode()


    # workaround for Mac OS. Fixed by 12.3. See https://github.com/hbldh/bleak/issues/720#issuecomment-1009198158
    # Required to advertise NUS feature for the workaround.
    service_uuids = []
    for item in uuid16_dict:
        service_uuids.append("{0:04x}".format(item))
    service_uuids.extend(uuid128_dict.keys())
    # workaround end, but must use register_detection_callback, thus found_device_queue

    scanner = BleakScanner(service_uuids=service_uuids)
    scanner.register_detection_callback(on_detection)

    logger.debug("Scanning ...")
    await scanner.start()
    device = await asyncio.wait_for(found_device_queue.get(), timeout=60.0)
    await scanner.stop()

    def handle_disconnect(_: BleakClient):
        print("Device was disconnected, goodbye.")
        end()


    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:

        await client.start_notify(UART_TX_CHAR_UUID, handle_rx)

        await client.write_gatt_char(UART_RX_CHAR_UUID, b"log backend log_backend_uart halt\r\n")
        await asyncio.sleep(2)


        await client.write_gatt_char(UART_RX_CHAR_UUID, b"appCtrl memfault_retrieve_data\r\n")

        print("Retrieve Data")
        await asyncio.sleep(4)
        await client.write_gatt_char(UART_RX_CHAR_UUID, b"log go\r\n")
        for command in memfaultCommands.split("\r\n"):
            if command.startswith("memfault"):
                print(command)
                print(subprocess.Popen(command, shell=True, stdout=subprocess.PIPE).stdout.read().decode())
        await asyncio.sleep(3)


if __name__ == "__main__":
    try:
        asyncio.run(uart_terminal())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass
