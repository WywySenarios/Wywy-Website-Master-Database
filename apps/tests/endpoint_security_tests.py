import unittest
import requests
from os import environ
from constants import AUTH_COOKIES


def test_endpoint_security(test_object: unittest.TestCase, endpoint: str) -> None:
    # does the endpoint check the origin header?
    test_object.assertEqual(requests.get(endpoint).status_code, 400)

    # is the endpoint secured with authentication?
    test_object.assertEqual(
        requests.post(
            endpoint,
            headers={"Origin": environ["MAIN_URL"]},
            data="",
        ).status_code,
        401,
    )

    # check if modifying authentication details invalidates the authentication
    for key in AUTH_COOKIES:
        cookies = {**AUTH_COOKIES}
        del cookies[key]

        test_object.assertEqual(
            requests.post(
                endpoint,
                cookies=cookies,
                headers={"Origin": environ["MAIN_URL"]},
                data="",
            ).status_code,
            403,
        )

    for key in AUTH_COOKIES:
        cookies = AUTH_COOKIES
        cookies[key] += " <- is now invalid"

        test_object.assertEqual(
            requests.post(
                endpoint,
                cookies=cookies,
                headers={"Origin": environ["MAIN_URL"]},
                data="",
            ).status_code,
            403,
        )
