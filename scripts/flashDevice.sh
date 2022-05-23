SERIAL_DEV=$(ls /dev/tty.usbmodem*)
echo $SERIAL_DEV

~/tools/nrfutil-mac.1 pkg generate --hw-version 52 --sd-req=0x00 --application ../dataCollector/build/zephyr/zephyr.hex --application-version 1 build.zip
~/tools/nrfutil-mac.1 dfu usb-serial -pkg build.zip -p $SERIAL_DEV
