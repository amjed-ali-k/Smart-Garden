import os
from pymongo import MongoClient


mongoClient = MongoClient(os.environ.get("DB_URL") or "localhost", 27017)
db = mongoClient["smart-garden"]
