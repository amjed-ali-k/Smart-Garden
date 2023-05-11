import datetime
import json
import os
import random
from typing import Dict
import paho.mqtt.client as mqtt
from db.db import db
from models.device import (
    DBConfigsIn,
    DBHardwareStatus,
    DBSensorDataIn,
    FeedbackRes1,
    FeedbackRes2,
    FeedbackRes3,
    FeedbackRes4,
    FeedbackRes5,
    PayloadSensorData,
)

mqttClient = mqtt.Client(f"Cloud-{random.randint(0, 1000)}")


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print("Failed to connect, return code %d\n", rc)


def mqtt_start():
    # Set Connecting Client ID
    mqttClient.on_connect = on_connect
    mqttClient.connect(os.environ.get("MQTT_HOST") or "broker.hivemq.com", 1883)
    mqttClient.subscribe("/+/feedback")
    mqttClient.subscribe("/+/sensor-data")
    mqttClient.message_callback_add("/+/feedback", on_message_feedback)
    mqttClient.message_callback_add("/+/sensor-data", on_message_sensor_data)

    mqttClient.loop_start()


def mqtt_stop():
    mqttClient.loop_stop()
    mqttClient.disconnect()


command_type = {
    "status": FeedbackRes1,
    "moisture_sensor": FeedbackRes2,
    "valve_status": FeedbackRes3,
    "uptime": FeedbackRes4,
    "config": FeedbackRes5,
}


def on_message_feedback(topic, client: mqtt.Client, message: mqtt.MQTTMessage):
    client_name = message.topic.split("/")[1]
    decoded: Dict = json.loads(message.payload)
    if not decoded.get("command"):
        return
    command = decoded["command"]
    print(command)
    if command == "status":
        data = FeedbackRes1.parse_raw(message.payload)
        db.hardware_status.update_one(
            {"mqtt_client_name": client_name},
            {
                "$set": DBHardwareStatus(
                    **data.dict(), mqtt_client_name=client_name
                ).dict()
            },
        )

    elif command == "moisture_sensor":
        data = FeedbackRes2.parse_raw(message.payload)
        index = data.sensor
        value = data.moisture
        db.hardware_status.update_one(
            {"mqtt_client_name": client_name}, {"$set": {f"moisture.{index}": value}}
        )

    elif command == "valve_status":
        data = FeedbackRes3.parse_raw(message.payload)
        index = data.valve
        value = data.status
        db.hardware_status.update_one(
            {"mqtt_client_name": client_name}, {"$set": {f"valve.{index}": value}}
        )

    elif command == "uptime":
        data = FeedbackRes4.parse_raw(message.payload)
        db.hardware_status.update_one(
            {"mqtt_client_name": client_name}, {"$set": {"uptime": data.uptime}}
        )

    elif command == "config":
        data = FeedbackRes5.parse_raw(message.payload)
        if (
            db.configs.update_one(
                {"mqtt_client_name": data.mqtt_client_name},
                {
                    "$set": DBConfigsIn(
                        **data.dict(), updated_at=datetime.datetime.now().timestamp()
                    ).dict()
                },
            ).modified_count
            == 0
        ):
            db.configs.insert_one(
                DBConfigsIn(
                    **data.dict(), updated_at=datetime.datetime.now().timestamp()
                ).dict()
            )

    db.mqtt_logs.insert_one(decoded)


def on_message_sensor_data(topic, client: mqtt.Client, message: mqtt.MQTTMessage):
    db.sensor_history.insert_one(
        DBSensorDataIn(
            **PayloadSensorData.parse_raw(message.payload).dict(),
            created_at=datetime.datetime.now().timestamp(),
        ).dict()
    )


def publish(topic: str, payload: dict):
    mqttClient.publish(topic, payload=json.dumps(payload))
