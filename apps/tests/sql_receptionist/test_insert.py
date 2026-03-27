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

    def test_insert_generic(self):
        """Test if basic INSERT endpoint logic security is enabled."""
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

    def test_insert_tag_names(self):
        """Test the INSERT tag_names endpoint for every table."""
        empty_body_json_checked: bool = False

        populate_database()

        for database_info in CONFIG["data"]:
            database_name = to_lower_snake_case(database_info["dbname"])
            for table_info in database_info["tables"]:
                table_name = to_lower_snake_case(table_info["tableName"])
                endpoint = (
                    f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tag_names"
                )
                response = requests.post(
                    endpoint,
                    headers={"Origin": environ["MAIN_URL"]},
                    cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                    json={"tag_name": "'very unique tag name'"},
                )
                if table_info.get("tagging", False):
                    self.assertEqual(
                        response.status_code,
                        200,
                        f"Valid INSERT to {endpoint} is not OK: {response.status_code}: {response.text}",
                    )

                    # also check for empty body or JSON case if we haven't already done so
                    if not empty_body_json_checked:
                        # is INSERTing nothing or an empty JSON caught?
                        endpoint = f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tag_names"
                        response = requests.post(
                            endpoint,
                            headers={"Origin": environ["MAIN_URL"]},
                            cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                            data="",
                        )
                        self.assertEqual(
                            response.status_code,
                            400,
                            f"Invalid INSERT to {endpoint} does not respond with status 400: {response.status_code}: {response.text}",
                        )
                        endpoint = f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tag_names"
                        response = requests.post(
                            endpoint,
                            headers={"Origin": environ["MAIN_URL"]},
                            cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                            json={},
                        )
                        self.assertEqual(
                            response.status_code,
                            400,
                            f"Invalid INSERT to {endpoint} does not respond with status 400: {response.status_code}: {response.text}",
                        )
                        empty_body_json_checked = True

                else:
                    # check that tables with tagging disabled cannot INSERT tags
                    self.assertEqual(
                        response.status_code,
                        400,
                        f"Invalid insert to {endpoint} did not respond with status 400: {response.status_code}: {response.text}",
                    )

    def test_insert_tags(self):
        """Test the INSERT tags endpoint for every table."""
        empty_body_json_checked: bool = False

        populate_database()

        for database_info in CONFIG["data"]:
            database_name = to_lower_snake_case(database_info["dbname"])
            for table_info in database_info["tables"]:
                table_name = to_lower_snake_case(table_info["tableName"])
                endpoint = f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tags"
                response = requests.post(
                    endpoint,
                    headers={"Origin": environ["MAIN_URL"]},
                    cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                    json={"entry_id": 1, "tag_id": 1},
                )
                if table_info.get("tagging", False):
                    self.assertEqual(
                        response.status_code,
                        200,
                        f"Valid INSERT to {endpoint} is not OK: {response.status_code}: {response.text}",
                    )

                    # also check for empty body or JSON case if we haven't already done so
                    if not empty_body_json_checked:
                        # is INSERTing nothing or an empty JSON caught?
                        endpoint = (
                            f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tags"
                        )
                        response = requests.post(
                            endpoint,
                            headers={"Origin": environ["MAIN_URL"]},
                            cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                            data="",
                        )
                        self.assertEqual(
                            response.status_code,
                            400,
                            f"Invalid INSERT to {endpoint} does not respond with status 400: {response.status_code}: {response.text}",
                        )
                        endpoint = (
                            f"{SQL_RECEPTIONIST_URL}/{database_name}/{table_name}/tags"
                        )
                        response = requests.post(
                            endpoint,
                            headers={"Origin": environ["MAIN_URL"]},
                            cookies=SQL_RECEPTIONIST_AUTH_COOKIES,
                            json={},
                        )
                        self.assertEqual(
                            response.status_code,
                            400,
                            f"Invalid INSERT to {endpoint} does not respond with status 400: {response.status_code}: {response.text}",
                        )
                        empty_body_json_checked = True

                else:
                    # check that tables with tagging disabled cannot INSERT tags
                    self.assertEqual(
                        response.status_code,
                        400,
                        f"Invalid insert to {endpoint} did not respond with status 400: {response.status_code}: {response.text}",
                    )

    def test_insert(self):
        """Test the INSERT data (main table & descriptors) endpoint for every table."""
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
