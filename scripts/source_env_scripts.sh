source .env/bin/activate
source /opt/nordic/ncs/v1.8.0/zephyr/zephyr-env.sh
export MQTT_USER_NAME=__REPLACE_ME__MQTT_USER_NAME
export MQTT_CREDENTIALS=__REPLACE_ME__MQTT_PASSWORD
export MQTT_FEED_NAME_HEATMAP=__REPLACE_ME__MQTT_TOPIC_HEATMAP
printenv
