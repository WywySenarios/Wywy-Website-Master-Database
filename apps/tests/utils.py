from typing import List
import re
from psycopg.cursor import Cursor


def to_snake_case(target: str) -> str:
    """Attempts to convert from regular words/sentences to snake_case. This will not affect strings already in underscore notation. (Does not work with camelCase)
    @param target
    @return Returns underscore notation string. e.g. "hi I am Wywy" -> "hi_I_am_Wywy"
    """
    stringFrags: List[str] = re.split(r"[\.\ \-]", target)

    output: str = ""

    for i in stringFrags:
        output += i + "_"

    return output[:-1]  # remove trailing underscore with "[:-1]"


def to_lower_snake_case(target: str) -> str:
    """Attempts to convert from regular words/sentences to lower_snake_case. This will not affect strings already in underscore notation. (Does not work with camelCase)
    @param target
    @return Returns lower_snake_case string. e.g. "hi I am Wywy" -> "hi_i_am_wywy"
    """
    stringFrags: List[str] = re.split(r"[\.\ \-]", target)

    output: str = ""

    for i in stringFrags:
        output += i.lower() + "_"

    return output[:-1]  # remove trailing underscore with "[:-1]"


def select_result_is_true(cur: Cursor, is_none_safe: bool = False) -> bool:
    """Checks if the result of the SELECT query is true.

    Args:
        cur (Cursor): The cursor to extract a row from.
        is_none_safe (bool, optional): Whether or not None is a valid value. Defaults to False.

    Raises:
        RuntimeError: When None is

    Returns:
        bool: Returns False if the result is None or False. Returns True if the result is True.
    """
    row = cur.fetchone()
    if row is None:
        if is_none_safe:
            return False
        else:
            raise RuntimeError("None returned as a result.")
    return row[0]
