from typing import Union
from fastapi import FastAPI
from fastapi_mqtt import FastMQTT, MQTTConfig


app = FastAPI()

mqtt_config = MQTTConfig(
    host="broker.hivemq.com",
    port=1883,
    keepalive=60,
    username=None,
    password=None,
)

mqtt = FastMQTT(config=mqtt_config)
mqtt.init_app(app)


@mqtt.on_connect()
def connect(client, flags, rc, properties):
    mqtt.subscribe("/SmartGarden-82FA/feedback")
    print("Connected")


@mqtt.subscribe("/SmartGarden-82FA/feedback")
async def subscribe_feedback(client, topic, payload, qos, properties):
    print("Received message")
    print(payload)
    print(topic)
    print(qos)
    print(properties)


@app.get("/")
async def read_root():
    return {"Hello": "World"}


@app.get("/items/{item_id}")
async def read_item(item_id: int, q: Union[str, None] = None):
    return {"item_id": item_id, "q": q}
