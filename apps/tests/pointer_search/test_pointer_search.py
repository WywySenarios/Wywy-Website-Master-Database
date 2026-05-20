"""Search-related tests for sql-receptionist pointer search."""

from string import Template
import unittest
import requests
from constants import SQL_RECEPTIONIST_URL, GENERIC_REQUEST_PARAMS
from config import CONFIG
from utils import to_lower_snake_case

FIRST_DB = to_lower_snake_case(CONFIG["data"][0]["dbname"])
FIRST_TABLE = to_lower_snake_case(CONFIG["data"][0]["tables"][0]["tableName"])

SEARCH_ENDPOINT = Template(SQL_RECEPTIONIST_URL + "/${db}/${table}/search")


def _search(db: str, table: str, q: str) -> requests.Response:
    url = SEARCH_ENDPOINT.substitute(db=db, table=table)
    params = {"q": q}
    return requests.get(url, **GENERIC_REQUEST_PARAMS, params=params)


class TestPointerSearch(unittest.TestCase):

    def test_search_missing_q(self) -> None:
        url = SEARCH_ENDPOINT.substitute(db=FIRST_DB, table=FIRST_TABLE)
        resp = requests.get(url, **GENERIC_REQUEST_PARAMS)
        self.assertEqual(resp.status_code, 400)
        self.assertIn("Query parameter 'q' is required", resp.text)

    def test_search_nonexistent_table(self) -> None:
        resp = _search(FIRST_DB, "nonexistent_table", "test")
        self.assertEqual(resp.status_code, 400)

    def test_search_nonexistent_database(self) -> None:
        resp = _search("nonexistent_db", FIRST_TABLE, "test")
        self.assertEqual(resp.status_code, 400)

    def test_search_empty_result(self) -> None:
        resp = _search(FIRST_DB, FIRST_TABLE, "xyznonexistent123")
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertIsInstance(data, list)

    def test_search_response_shape(self) -> None:
        resp = _search(FIRST_DB, FIRST_TABLE, "poi")
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertIsInstance(data, list)
        if data:
            item = data[0]
            self.assertIn("id", item)
            self.assertIn("label", item)

    def test_search_by_id(self) -> None:
        resp = _search(FIRST_DB, FIRST_TABLE, "1")
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertIsInstance(data, list)

    def test_search_origin_required(self) -> None:
        url = SEARCH_ENDPOINT.substitute(db=FIRST_DB, table=FIRST_TABLE)
        resp = requests.get(url, params={"q": "test"})
        self.assertEqual(resp.status_code, 400)
        self.assertIn("origin", resp.text.lower())

    def test_search_unauthenticated(self) -> None:
        url = SEARCH_ENDPOINT.substitute(db=FIRST_DB, table=FIRST_TABLE)
        resp = requests.get(
            url,
            params={"q": "test"},
            headers=dict(GENERIC_REQUEST_PARAMS["headers"]),
        )
        self.assertEqual(resp.status_code, 401)

    def test_search_empty_q(self) -> None:
        resp = _search(FIRST_DB, FIRST_TABLE, "")
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertIsInstance(data, list)
        if data:
            item = data[0]
            self.assertIn("id", item)
            self.assertIn("label", item)


if __name__ == "__main__":
    unittest.main()
