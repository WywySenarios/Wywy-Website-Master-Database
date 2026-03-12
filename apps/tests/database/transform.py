from ..config import CONFIG
from ..constants import CONN_CONFIG
from ..utils import to_lower_snake_case
from ..types import DataColumn, TableType
import psycopg
from typing import Callable, List, TypeAlias

TransformTargets: TypeAlias = dict[str, tuple[TableType, List[DataColumn] | None]]


def table_transform(
    transformation: Callable[
        [psycopg.Cursor, dict[str, tuple[TableType, List[DataColumn] | None]]],
        None,
    ],
) -> None:
    """Applies a transformation to every table. The cursor does not have autocommit.

    Args:
        transformation (Callable[[psycopg.Cursor, dict[str, tuple[TableType, List[DataColumn] | None]]], None]): The transformation to apply to every item inside the CONFIG. The transformation function takes in in the cursor and a dictionary with the database table name as the key and a tuple with length 2 containing the table type and the column schema if available.
    """

    for databaseSchema in CONFIG["data"]:
        with psycopg.connect(**CONN_CONFIG, dbname=databaseSchema["dbname"]) as conn:
            with conn.cursor() as cur:
                targets: dict[str, tuple[TableType, List[DataColumn] | None]] = {}
                for tableSchema in databaseSchema["tables"]:
                    table_name: str = to_lower_snake_case(tableSchema["tableName"])
                    targets[table_name] = ("data", tableSchema["schema"])

                    # main table

                    # tagging tables
                    if tableSchema.get("tagging", False) is True:
                        targets[f"{table_name}_tags"] = ("tags", None)
                        targets[f"{table_name}_tag_aliases"] = ("tag_aliases", None)
                        targets[f"{table_name}_tag_groups"] = ("tag_groups", None)
                        targets[f"{table_name}_tag_names"] = ("tag_names", None)

                    # purge descriptors
                    for descriptorSchema in tableSchema.get("descriptors", []):
                        targets[
                            f"{table_name}_{to_lower_snake_case(descriptorSchema['name'])}_descriptors"
                        ] = (
                            "descriptor",
                            descriptorSchema["schema"],
                        )  # @TODO check if this table type name is correct

                # call transformation
                transformation(
                    cur,
                    targets,
                )
