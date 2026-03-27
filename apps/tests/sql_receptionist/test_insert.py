"""INSERT related tests for sql-receptionist."""

import unittest
import requests
from os import environ
from Wywy_Website_Types import DataDatatype
from config import CONFIG
from constants import SQL_RECEPTIONIST_URL, SQL_RECEPTIONIST_AUTH_COOKIES, PSQLDATATYPES
from utils import to_lower_snake_case
from testing_helpers.purge import purge_database
from testing_helpers.populate import create_values, populate_database


class TestSelectEndpoints(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        purge_database()

    def test_insert(self):
        """Test the INSERT data (main table & descriptors) endpoint for every table."""

        # does the INSERT endpoint check the origin header?
        self.assertEqual(requests.get(f"{SQL_RECEPTIONIST_URL}").status_code, 400)

        # is the INSERT endpoint secured with authentication?
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}",
                headers={"Origin": environ["MAIN_URL"]},
                data="",
            ).status_code,
            403,
        )

        # does the INSERT endpoint require a database?
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}",
                headers={"Origin": environ["MAIN_URL"]},
                cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                data="",
            ).status_code,
            400,
        )

        # does the INSERT endpoint require a table?
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}/{to_lower_snake_case(CONFIG["data"][0]["dbname"])}",
                headers={"Origin": environ["MAIN_URL"]},
                cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                data="",
            ).status_code,
            400,
        )

        # does a target table type need to be specified?
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}/{to_lower_snake_case(CONFIG["data"][0]["dbname"])}/{to_lower_snake_case(CONFIG["data"][0]["tables"][0]["tableName"])}",
                headers={"Origin": environ["MAIN_URL"]},
                cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
            ).status_code,
            400,
        )

        # is INSERTing nothing or an empty JSON caught?
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}/{to_lower_snake_case(CONFIG["data"][0]["dbname"])}/{to_lower_snake_case(CONFIG["data"][0]["tables"][0]["tableName"])}/data",
                headers={"Origin": environ["MAIN_URL"]},
                cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                data="",
            ).status_code,
            400,
        )
        self.assertEqual(
            requests.post(
                f"{SQL_RECEPTIONIST_URL}/{to_lower_snake_case(CONFIG["data"][0]["dbname"])}/{to_lower_snake_case(CONFIG["data"][0]["tables"][0]["tableName"])}/data",
                headers={"Origin": environ["MAIN_URL"]},
                cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                json={},
            ).status_code,
            400,
        )

        populate_database()

        payload: dict[str, DataDatatype] = {}
        for database_info in CONFIG["data"]:
            database_name = to_lower_snake_case(database_info["dbname"])
            for table_info in database_info["tables"]:
                table_name = to_lower_snake_case(table_info["tableName"])
                payload = create_values(table_info)

                endpoint = f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/data"
                response = requests.post(
                    f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/data",
                    headers={"Origin": environ["MAIN_URL"]},
                    cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                    json=payload,
                )

                self.assertEqual(
                    response.status_code,
                    200,
                    f"Valid INSERT to {endpoint} not OK: {response.text}",
                )
                self.assertEqual(
                    int(response.text),
                    6,
                    "Valid INSERT does not insert into the expected ID.",
                )

                # @TODO descriptors

    # def test_insert_missing(self):
    #     # we will run duplicate tests because of aliases, but that doesn't matter
    #     datatypes_tested = set(PSQLDATATYPES.keys())
    #     targets_to_test = []
    #     for database_info in CONFIG["data"]:
    #         for table_info in database_info["tables"]:
    #             pass
