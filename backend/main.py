import datetime, pytz
from typing import List
from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
from fastapi.middleware.cors import CORSMiddleware

from pydantic import BaseModel
from db.db import db, mongoClient
from mqtt import mqtt_start, mqtt_stop, publish
from models.device import (
    DBConfigsIn,
    DBSensorDataIn,
    DeviceConfig,
    HardwareStatusIn,
    PublicDeviceConfig,
)


app = FastAPI()

origins = [
    "https://web-smartgarden-ui.s2.tebs.co.in",
    "http://localhost:5173",
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
tz = pytz.timezone("Asia/Kolkata")


@app.on_event("startup")
def connect_mqtt():
    mqtt_start()
    mongoClient.start_session()


@app.on_event("shutdown")
def disconnect_mqtt():
    mqtt_stop()
    mongoClient.close()


# favicon
@app.get("/favicon.ico", include_in_schema=False)
async def favicon():
    return FileResponse("favicon.ico")


@app.get("/")
async def read_root():
    # Show nice welcome message
    return {"message": "Welcome to your smart garden"}


device_not_found = HTTPException(status_code=404, detail="Device not found")


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
async def get_sensor_status(client_name: str) -> HardwareStatusIn:
    data = db.hardware_status.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise device_not_found
    res = HardwareStatusIn(**data)
    diff = datetime.datetime.now(tz) - datetime.datetime.fromtimestamp(
        res.updated_at, tz
    )
    print("Requested status - Time diffrence is : ", diff.seconds)
    if diff.seconds > 30:
        publish(f"/{client_name}/commands", {"command": "get_status"})
    # publish command
    return res


class PostValveDetails(BaseModel):
    valve: int
    status: bool


@app.post("/sensor/{client_name}/valve")
def change_valve_status(client_name: str, details: PostValveDetails):
    data = db.hardware_status.update_one(
        {"mqtt_client_name": client_name},
        {"$set": {f"valve.{details.valve}": details.status}},
    )

    if data is None:
        raise device_not_found

    publish(
        f"/{client_name}/commands",
        {
            "command": "open_valve" if details.status else "close_valve",
            "valve": details.valve,
        },
    )

    return {"success": True}


@app.get("/config/{client_name}")
async def get_config(client_name: str) -> PublicDeviceConfig:
    data = db.configs.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise device_not_found
    return PublicDeviceConfig(**data)


@app.post("/config/{client_name}")
async def set_config(client_name: str, config: DeviceConfig):
    data = db.configs.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise device_not_found
    db.configs.update_one(
        {"mqtt_client_name": client_name},
        {
            "$set": DBConfigsIn(
                **config.dict(),
                updated_at=datetime.datetime.now(tz).timestamp(),
            ).dict()
        },
    )
    publish(f"/{client_name}/commands", {"command": "set_config", **config.dict()})

    return {"success": True}


@app.post("/sensor/{client_name}/reboot")
async def reboot(client_name: str):
    data = db.configs.find_one({"mqtt_client_name": client_name})
    if data is None:
        raise device_not_found
    publish(f"/{client_name}/commands", {"command": "restart"})
    return {"success": True}


@app.get("/device/{client_name}/logs")
async def get_logs(client_name: str):
    data = db.device_logs.find(
        {"mqtt_client_name": client_name}, limit=100, sort=[("created_at", -1)]
    )

    return [{**i, "_id": str(i["_id"])} for i in data]
