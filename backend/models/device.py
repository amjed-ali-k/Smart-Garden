from typing import List
import uuid
from bson import ObjectId
from pydantic import BaseModel, Field


class PyObjectId(ObjectId):
    @classmethod
    def __get_validators__(cls):
        yield cls.validate

    @classmethod
    def validate(cls, v):
        if not ObjectId.is_valid(v):
            raise ValueError("Invalid objectid")
        return ObjectId(v)

    @classmethod
    def __modify_schema__(cls, field_schema):
        field_schema.update(type="string")


class PayloadSensorData(BaseModel):
    valve: List[bool] = Field(...)
    moisture: List[bool] = Field(...)
    client_name: str = Field(...)


class DBSensorDataIn(PayloadSensorData):
    created_at: float = Field(...)


class DBSensorData(PayloadSensorData):
    id: PyObjectId = Field(default_factory=PyObjectId, alias="_id")
    created_at: float = Field(...)


class DeviceConfig(BaseModel):
    watering_duration: int = Field(...)
    watering_intreval: int = Field(...)
    watering_enabled: bool = Field(...)
    watering_times: List[int] = Field(...)
    wifi_ssid: str = Field(...)
    wifi_password: str = Field(...)
    mqtt_host: str = Field(...)
    mqtt_port: int = Field(...)
    mqtt_username: str = Field(...)
    mqtt_password: str = Field(...)
    mqtt_client_name: str = Field(...)


class DBConfigsIn(DeviceConfig):
    updated_at: float = Field(...)


class DBConfigs(DBConfigsIn):
    id: PyObjectId = Field(default_factory=PyObjectId, alias="_id")


class HardWareStatus(BaseModel):
    mqtt_client_name: str = Field(...)
    valve: List[bool] = Field(...)
    moisture: List[bool] = Field(...)
    uptime: int = Field(...)


class DBHardwareStatus(HardWareStatus):
    id: PyObjectId = Field(default_factory=PyObjectId, alias="_id")


class FeedbackRes1(BaseModel):
    command: str = "status"
    uptime: int
    valve: List[bool]
    moisture: List[bool]


class FeedbackRes2(BaseModel):
    command: str = "moisture_sensor"
    moisture: bool
    sensor: int


class FeedbackRes3(BaseModel):
    command: str = "valve_status"
    valve: int
    status: bool


class FeedbackRes4(BaseModel):
    command: str = "uptime"
    uptime: int


class FeedbackRes5(DeviceConfig):
    command: str = "config"


# create union of all feedback responses
FeedbackRes = FeedbackRes1 | FeedbackRes2 | FeedbackRes3 | FeedbackRes4 | FeedbackRes5
