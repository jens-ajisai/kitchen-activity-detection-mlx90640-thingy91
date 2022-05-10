source .env/bin/activate
source /opt/nordic/ncs/v1.8.0/zephyr/zephyr-env.sh
export ZEPHYR_BASE=/opt/nordic/ncs/v1.8.0/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/opt/nordic/ncs/v1.8.0/toolchain
printenv
west config build.board thingy91_nrf9160_ns
west config build.dir-fmt ../build_heaty_nrf9160_ns
rm -rf ../build_heaty_nrf9160_ns
west build ../nrf9160_ns
