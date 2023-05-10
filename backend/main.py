from typing import Union
from fastapi import FastAPI
from fastapi_mqtt import FastMQTT, MQTTConfig
from deta import Deta  # Import Deta

db = deta.Base("simple_db")


app = FastAPI()

mqtt_config = MQTTConfig()

mqtt = FastMQTT(config=mqtt_config)
mqtt.init_app(app)


@mqtt.on_connect()
def connect(client, flags, rc, properties):
    mqtt.subscribe("/SmartGarden-82FA/feedback")


@mqtt.on_message()
async def message(client, topic, payload, qos, properties):
    print("Received message")
    print(payload)
    print(topic)
    print(qos)
    print(properties)
    await db.put({"topic": topic, "payload": payload.decode("utf-8")})


@app.get("/")
async def read_root():
    return {"Hello": "World"}


@app.get("/items/{item_id}")
async def read_item(item_id: int, q: Union[str, None] = None):
    return {"item_id": item_id, "q": q}
