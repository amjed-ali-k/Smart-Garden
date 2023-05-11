from typing import List
from fastapi import FastAPI
import os

from db.db import db, mongoClient
from mqtt import mqtt_start, mqtt_stop
from models.device import DBSensorData, PayloadSensorData


app = FastAPI()


app = FastAPI()


@app.on_event("startup")
def connect_mqtt():
    mqtt_start()
    mongoClient.start_session()


@app.on_event("shutdown")
def disconnect_mqtt():
    mqtt_stop()
    mongoClient.close()


@app.get("/")
async def read_root():
    return {"Hello": "World"}


@app.get("/mqtt-logs/{client_name}")
async def get_mqtt_logs(client_name: str) -> List[PayloadSensorData]:
    res = list(db.sensor_history.find({"client_name": client_name}, limit=100))
    response: List[PayloadSensorData] = []
    for r in res:
        response.append(PayloadSensorData(**r))
    return response
