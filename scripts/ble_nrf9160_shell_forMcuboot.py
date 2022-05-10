import asyncio
import logging
import platform
import sys

from serial_asyncio import open_serial_connection

from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice
from bleak.uuids import uuid16_dict, uuid128_dict

logger = logging.getLogger(__name__)

SERIAL_NRF91_BRIDGE_SERVICE_UUID = "aaaf0a10-c332-42a8-93bd-25e905756cb8";
SERIAL_NRF91_BRIDGE_RX_CHAR_UUID = "aaaf0a11-c332-42a8-93bd-25e905756cb8";
SERIAL_NRF91_BRIDGE_TX_CHAR_UUID = "aaaf0a12-c332-42a8-93bd-25e905756cb8";

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
        writer.write(data)
        logger.debug(f"recv: {data.decode()}")

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

    reader, writer = await open_serial_connection(url='/dev/ttys004', baudrate=115200, rtscts=True,dsrdtr=True)

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(SERIAL_NRF91_BRIDGE_TX_CHAR_UUID, handle_rx)
        logger.info("Connected.")

        loop = asyncio.get_running_loop()

        await client.write_gatt_char(SERIAL_NRF91_BRIDGE_RX_CHAR_UUID, b"log backend log_backend_uart halt\r\n")
        await asyncio.sleep(2)

        while True:

            data = await reader.read(1000)

            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break

#            data = data.replace(b"\r", b"\r\n")

            await client.write_gatt_char(SERIAL_NRF91_BRIDGE_RX_CHAR_UUID, data)
            logger.info(f"sent: {data}")


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
    except serial.serialutil.SerialException:
        print("Please make sure to call 'socat -d -d pty,raw,echo=0 pty,raw,echo=0'")
        print("And the created devices are /dev/ttys003 and /dev/ttys004. Otherwise edit the script.")
