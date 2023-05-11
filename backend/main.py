import datetime
from typing import List
from fastapi import FastAPI, HTTPException
import os

from pydantic import BaseModel

from db.db import db, mongoClient
from mqtt import mqtt_start, mqtt_stop, publish
from models.device import (
    DBConfigs,
    DBConfigsIn,
    DBHardwareStatus,
    DBSensorData,
    DBSensorDataIn,
    DeviceConfig,
    HardWareStatus,
    PayloadSensorData,
)


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


@app.get("/sensor_history/{client_name}/{from_date}/{to_date}")
async def get_sensor_history(
    client_name: str, from_date: datetime.datetime, to_date: datetime.datetime
) -> List[DBSensorDataIn]:
    data = db.sensor_history.find(
        {
            "client_name": client_name,
            "created_at": {"$gte": from_date.timestamp(), "$lte": to_date.timestamp()},
        }
    )
    return [DBSensorDataIn(**i) for i in data]


@app.get("/sensor/{client_name}/status")
async def get_sensor_status(client_name: str) -> HardWareStatus:
    data = db.hardware_status.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise HTTPException(status_code=404, detail="Device not found")
    return HardWareStatus(**data)


class PostValveDetails(BaseModel):
    valve: int
    status: bool


@app.post("/sensor/{client_name}/valve")
def change_valve_status(client_name: str, details: PostValveDetails):
    data = db.hardware_status.find_one({"mqtt_client_name": client_name})

    if data is None:
        raise HTTPException(status_code=404, detail="Device not found")

    publish(
        f"/{client_name}/commands",
        {
            "command": "open_valve" if details.status else "close_valve",
            "valve": details.valve,
        },
    )

    return {"success": True}


@app.get("/config/{client_name}")
async def get_config(client_name: str) -> DBConfigsIn:
    data = db.configs.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise HTTPException(status_code=404, detail="Device not found")
    return data


@app.post("/config/{client_name}")
async def set_config(client_name: str, config: DeviceConfig):
    data = db.configs.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise HTTPException(status_code=404, detail="Device not found")
    db.configs.update_one(
        {"mqtt_client_name": client_name},
        {
            "$set": DBConfigsIn(
                **config.dict(),
                updated_at=datetime.datetime.now().timestamp(),
            ).dict()
        },
    )
    publish(f"/{client_name}/commands", {"command": "set_config", **config.dict()})

    return {"success": True}
