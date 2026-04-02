import unittest
import requests
from os import environ
from constants import AUTH_COOKIES


def test_endpoint_security(test_object: unittest.TestCase, endpoint: str) -> None:
    # does the endpoint check the origin header?
    test_object.assertEqual(requests.get(endpoint).status_code, 400)

    # is the endpoint secured with authentication?
    response = requests.post(
        endpoint,
        headers={"Origin": environ["MAIN_URL"]},
        data="",
    )
    test_object.assertEqual(
        response.status_code,
        401,
        f"Unauthenticated request did not respond with status 401: {response.status_code}: {response.text}",
    )

    # check if modifying authentication details invalidates the authentication
    for key in AUTH_COOKIES:
        cookies = {**AUTH_COOKIES}
        del cookies[key]

        response = requests.post(
            endpoint,
            cookies=cookies,
            headers={"Origin": environ["MAIN_URL"]},
            data="",
        )
        test_object.assertEqual(
            response.status_code,
            403,
            f"Invalid authentication (missing authentication attribute) did not respond with status 403: {response.status_code}: {response.text}",
        )

    for key in AUTH_COOKIES:
        cookies = {**AUTH_COOKIES}
        cookies[key] += " <- is now invalid"

        response = requests.post(
            endpoint,
            cookies=cookies,
            headers={"Origin": environ["MAIN_URL"]},
            data="",
        )
        test_object.assertEqual(
            response.status_code,
            403,
            f"Invalid authentication (incorrect attribute) did not respond with status 403: {response.status_code}: {response.text}",
        )
