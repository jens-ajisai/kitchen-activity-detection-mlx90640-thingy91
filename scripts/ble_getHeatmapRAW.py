import asyncio
import logging
import platform
import sys
import os

from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice
from bleak.uuids import uuid16_dict, uuid128_dict


from PIL import Image
import base64

import json
import re

import struct
import io

from Adafruit_IO import Client

logger = logging.getLogger(__name__)


HEATMAP_SERVICE_UUID = "aaaf0900-c332-42a8-93bd-25e905756cb8"
HEATMAP_TX_UUID      = "aaaf0901-c332-42a8-93bd-25e905756cb8"
SENSOR_INTERVAL_UUID = "aaaf0902-c332-42a8-93bd-25e905756cb8"

incommingData = ""

aio = Client(os.environ.get('MQTT_USER_NAME'), os.environ.get('MQTT_CREDENTIALS'))
feedName = os.environ.get('MQTT_FEED_NAME_HEATMAP')


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

    async def print_services(svcs):
        svcs = await client.get_services()
        for service in svcs:
            logger.debug(f"[Service] {service}")
            for char in service.characteristics:
                if "read" in char.properties:
                    try:
                        value = bytes(await client.read_gatt_char(char.uuid))
                        logger.debug(
                            f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {value}"
                        )
                    except Exception as e:
                        logger.error(
                            f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {e}"
                        )
                else:
                    value = None
                    logger.debug(
                        f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {value}"
                    )
                for descriptor in char.descriptors:
                    try:
                        value = bytes(
                            await client.read_gatt_descriptor(descriptor.handle)
                        )
                        logger.debug(f"\t\t[Descriptor] {descriptor}) | Value: {value}")
                    except Exception as e:
                        logger.error(f"\t\t[Descriptor] {descriptor}) | Value: {e}")


    def process_image_data(img_data):
        logger.debug(f"img_data {img_data}")
        decoded_data = base64.decodebytes(img_data.encode('ascii'))
        U = struct.unpack("768f", decoded_data)
        newList = [(int) (x) for x in U]
        logger.debug(f"decoded_data {decoded_data}")
        logger.debug(f"newList {newList}")
        image = Image.frombytes('L', (32,24), bytes(newList), "raw")
        # image.thumbnail((32*8,24*8), Image.ANTIALIAS)
        # image = image.resize((32*8,24*8), Image.ANTIALIAS)
        image.show()

        in_mem_file = io.BytesIO()
        image.save(in_mem_file, format = "PNG")
        in_mem_file.seek(0)
        img_bytes = in_mem_file.read()
        base64_encoded_result_bytes = base64.b64encode(img_bytes)
        base64_encoded_result_str = base64_encoded_result_bytes.decode('ascii')

        aio.send(feedName, base64_encoded_result_str)


    def handle_rx(_: int, data: bytearray):
        global incommingData
        logger.debug(f"received: {data}")
        incommingData += data.decode()
        if(data[-1] == 0):
            incommingData = incommingData[:-1]
            process_image_data(incommingData)
            incommingData = ""


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
        await print_services(client)

        await client.start_notify(HEATMAP_TX_UUID, handle_rx)
        logger.info("Connected.")

        loop = asyncio.get_running_loop()

        while True:
            # This waits until you type a line and press ENTER.
            print("Enter an interval [s].")
            data = await loop.run_in_executor(None, sys.stdin.buffer.readline)

            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break

            try:
                data = data.replace(b"\n", b"")
                numberAsByte = int(data).to_bytes(1, byteorder='big')

                await client.write_gatt_char(SENSOR_INTERVAL_UUID, numberAsByte)
                logger.info(f"sent: {numberAsByte}")
            except:
                pass

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)-15s %(levelname)s: %(message)s",
    )
    logger.setLevel(logging.DEBUG)
    try:
        asyncio.run(main())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass
