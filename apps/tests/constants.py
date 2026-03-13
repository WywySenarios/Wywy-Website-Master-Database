from os import environ
from typing import Literal
from .types import Datatype, PostgresDatatype

# Constants
RESERVED_DATABASE_NAMES = ["info"]
RESERVED_TABLE_NAMES: list[str] = []
RESERVED_TABLE_SUFFIXES = [
    "tags",
    "tag_aliases",
    "tag_names",
    "tag_groups",
    "descriptors",
]
RESERVED_COLUMN_NAMES = ["id", "user", "users", "primary_tag"]
RESERVED_COLUMN_SUFFIXES = ["comments"]
PSQLDATATYPES: dict[Datatype, PostgresDatatype] = {
    "int": "integer",
    "integer": "integer",
    "float": "real",
    "number": "real",
    # "double": "double precision",
    "str": "text",
    "string": "text",
    "text": "text",
    "bool": "boolean",
    "boolean": "boolean",
    "date": "date",
    "time": "time",
    "timestamp": "timestamp",
    # "interval": "interval",
    "enum": "enum",
    "geodetic point": "ST_Point",
}
CONSTRAINT_NAMES = {
    "pkey": "pkey",
    "not_null": "not_null",
    "min": "min",
    "max": "max",
    "values": "values",
    "fkey": "fkey",  # requires additional params afterward
    "unique": "unique",
    # "default": "default",
}

CONN_CONFIG: dict[Literal["host", "port", "user", "password", "sslmode"], str] = {
    "host": environ["DATABASE_HOST"],
    "port": environ["DATABASE_PORT"],
    "user": environ["DATABASE_USERNAME"],
    "password": environ["DATABASE_PASSWORD"],
    "sslmode": "prefer",
}
