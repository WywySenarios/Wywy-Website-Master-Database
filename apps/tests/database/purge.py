import psycopg
from psycopg import sql
from .transform import table_transform, TransformTargets


def purge_transformation(cur: psycopg.Cursor, targets: TransformTargets):
    cur.execute(
        sql.SQL(
            """
            TRUNCATE TABLE {table_names} CASCADE RESTART IDENTITY;
            """
        ).format(table_names=sql.SQL(", ").join(targets.keys()))
    )


def purge_database():
    """Purges the Postgres database."""

    table_transform(purge_transformation)
