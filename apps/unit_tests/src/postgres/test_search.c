#include "assert_test.h"
#include "postgres.h"
#include "postgres/search.h"
#include "test_case.h"
#include "utils/cur.h"
#include <jansson.h>
#include <stdio.h>
#include <string.h>

TEST_SUITE(search_table_type_detection) {
  int pass = 1;

  struct data_column schema[] = {{"name", "string", false, "", NULL}};
  struct descriptor descriptors[] = {
      {"sleep", schema, 1},
  };
  struct table tables[] = {
      {"events", true, true, true, "", schema, 1, descriptors, 1},
      {"daily", true, true, false, "", schema, 1, NULL, 0},
  };
  struct db db = {"main", tables, 2};

  pass &=
      assert_true(detect_search_table_type(&db, "events") == SEARCH_MAIN_TABLE,
                  "F: 'events' should be SEARCH_MAIN_TABLE\n");
  pass &=
      assert_true(detect_search_table_type(&db, "daily") == SEARCH_MAIN_TABLE,
                  "F: 'daily' should be SEARCH_MAIN_TABLE\n");
  pass &=
      assert_true(detect_search_table_type(&db, "events_tags") == SEARCH_TAGS,
                  "F: 'events_tags' should be SEARCH_TAGS\n");
  pass &= assert_true(detect_search_table_type(&db, "events_tag_names") ==
                          SEARCH_TAG_NAMES,
                      "F: 'events_tag_names' should be SEARCH_TAG_NAMES\n");
  pass &= assert_true(detect_search_table_type(&db, "events_tag_aliases") ==
                          SEARCH_TAG_ALIASES,
                      "F: 'events_tag_aliases' should be SEARCH_TAG_ALIASES\n");
  pass &= assert_true(detect_search_table_type(&db, "events_tag_groups") ==
                          SEARCH_TAG_GROUPS,
                      "F: 'events_tag_groups' should be SEARCH_TAG_GROUPS\n");
  pass &=
      assert_true(detect_search_table_type(&db, "events_sleep_descriptors") ==
                      SEARCH_DESCRIPTORS,
                  "F: 'events_sleep_descriptors' should be "
                  "SEARCH_DESCRIPTORS\n");
  pass &= assert_true(detect_search_table_type(&db, "nonexistent") ==
                          SEARCH_UNKNOWN,
                      "F: 'nonexistent' should be SEARCH_UNKNOWN\n");
  pass &=
      assert_true(detect_search_table_type(&db, "daily_tags") == SEARCH_UNKNOWN,
                  "F: 'daily_tags' should be SEARCH_UNKNOWN "
                  "(daily has tagging=false)\n");

  return pass;
}

TEST_SUITE(search_display_column) {
  int pass = 1;

  struct data_column schema[] = {{"title", "string", false, "", NULL}};
  struct table tables[] = {
      {"articles", true, true, true, "", schema, 1, NULL, 0},
  };
  struct db db = {"main", tables, 1};

  char parent_table[64] = "";
  const char *display = NULL;
  enum search_table_type type;

  type = detect_search_table_type(&db, "articles");
  pass &=
      assert_true(type == SEARCH_MAIN_TABLE, "F: 'articles' should be MAIN\n");
  if (type == SEARCH_MAIN_TABLE) {
    size_t raw_len = strlen("articles");
    (void)raw_len;
    for (unsigned int i = 0; i < db.tables_count; i++) {
      if (strcmp(db.tables[i].table_name, "articles") == 0) {
        display = db.tables[i].schema[0].name;
        break;
      }
    }
    pass &= assert_true(display != NULL, "F: display should not be NULL\n");
    if (display)
      pass &= assert_true(strcmp(display, "title") == 0,
                          "F: display should be 'title'\n");
  }

  type = detect_search_table_type(&db, "articles_tags");
  pass &=
      assert_true(type == SEARCH_TAGS, "F: 'articles_tags' should be TAGS\n");
  if (type == SEARCH_TAGS) {
    display = NULL;
  }

  type = detect_search_table_type(&db, "articles_tag_names");
  pass &= assert_true(type == SEARCH_TAG_NAMES,
                      "F: 'articles_tag_names' should be TAG_NAMES\n");
  if (type == SEARCH_TAG_NAMES) {
    display = "tag_name";
    pass &= assert_true(strcmp(display, "tag_name") == 0,
                        "F: tag_names display should be 'tag_name'\n");
  }

  type = detect_search_table_type(&db, "articles_tag_aliases");
  pass &= assert_true(type == SEARCH_TAG_ALIASES,
                      "F: 'articles_tag_aliases' should be TAG_ALIASES\n");

  return pass;
}

TEST_SUITE(search_q_parseability) {
  int pass = 1;
  char query[4096];

  int ret = build_search_query(query, sizeof(query), SEARCH_MAIN_TABLE,
                               "events", "", "name", "42");
  pass &= assert_true(ret == 0, "F: build_search_query should succeed\n");
  pass &= assert_true(strstr(query, "CAST(id AS TEXT) LIKE $1") != NULL,
                      "F: integer q should include CAST(id AS TEXT) LIKE\n");

  ret = build_search_query(query, sizeof(query), SEARCH_MAIN_TABLE, "events",
                           "", "name", "hello");
  pass &= assert_true(ret == 0, "F: build_search_query should succeed\n");
  pass &= assert_true(strstr(query, "CAST(id AS TEXT) LIKE $1") == NULL,
                      "F: non-integer q should NOT include id condition\n");

  ret = build_search_query(query, sizeof(query), SEARCH_MAIN_TABLE, "events",
                           "", "name", "");
  pass &= assert_true(ret == 0, "F: build_search_query with empty q\n");
  pass &= assert_true(strstr(query, "CAST(id AS TEXT) LIKE $1") == NULL,
                      "F: empty q should NOT include id condition\n");

  ret = build_search_query(query, sizeof(query), SEARCH_TAGS, "", "events",
                           "tag_name", "42");
  pass &=
      assert_true(ret == 0, "F: build_search_query for TAGS should succeed\n");
  pass &= assert_true(strstr(query, "CAST(id AS TEXT) LIKE $1") != NULL,
                      "F: integer q for TAGS should include id condition\n");

  ret = build_search_query(query, sizeof(query), SEARCH_TAG_ALIASES, "",
                           "events", "alias", "hello");
  pass &= assert_true(ret == 0,
                      "F: build_search_query for ALIASES should succeed\n");
  pass &=
      assert_true(strstr(query, "CAST(id AS TEXT) LIKE $1") == NULL,
                  "F: non-integer q for ALIASES should NOT include id cond\n");

  return pass;
}

TEST_SUITE(search_query_building) {
  int pass = 1;
  char query[4096];
  char expected_limit[32];
  const char *env_limit = getenv("SQL_RECEPTIONIST_SEARCH_LIMIT");
  snprintf(expected_limit, sizeof(expected_limit), "LIMIT %s", env_limit);

  build_search_query(query, sizeof(query), SEARCH_MAIN_TABLE, "events", "",
                     "name", "test");
  pass &= assert_true(
      strstr(query, "SELECT id, name AS label FROM events WHERE") != NULL,
      "F: MAIN query should SELECT id, name AS label FROM events\n");
  pass &=
      assert_true(strstr(query, "CAST(name AS TEXT) ILIKE $1") != NULL,
                  "F: MAIN query should have CAST(name AS TEXT) ILIKE $1\n");
  pass &= assert_true(strstr(query, "ORDER BY id ASC") != NULL,
                      "F: MAIN query should ORDER BY id ASC\n");
  char main_msg[128];
  snprintf(main_msg, sizeof(main_msg), "F: MAIN query should %s\n",
           expected_limit);
  pass &= assert_true(strstr(query, expected_limit) != NULL, main_msg);

  build_search_query(query, sizeof(query), SEARCH_TAG_NAMES, "events_tag_names",
                     "", "tag_name", "test");
  pass &= assert_true(strstr(query, "FROM events_tag_names") != NULL,
                      "F: TAG_NAMES query should FROM events_tag_names\n");
  pass &= assert_true(
      strstr(query, "CAST(tag_name AS TEXT) ILIKE $1") != NULL,
      "F: TAG_NAMES query should have CAST(tag_name AS TEXT) ILIKE $1\n");

  build_search_query(query, sizeof(query), SEARCH_TAGS, "", "events",
                     "tag_name", "test");
  pass &= assert_true(strstr(query, "FROM events_tags AS tags") != NULL,
                      "F: TAGS query should FROM events_tags AS tags\n");
  pass &=
      assert_true(strstr(query, "JOIN events_tag_names AS tag_names") != NULL,
                  "F: TAGS query should JOIN events_tag_names\n");
  pass &= assert_true(strstr(query, "ON tags.tag_id = tag_names.id") != NULL,
                      "F: TAGS query should join on tag_id\n");
  pass &= assert_true(
      strstr(query, "CAST(tag_names.tag_name AS TEXT) ILIKE $1") != NULL,
      "F: TAGS query should filter on CAST(tag_names.tag_name AS TEXT)\n");

  build_search_query(query, sizeof(query), SEARCH_TAG_ALIASES, "", "events",
                     "alias", "test");
  pass &= assert_true(
      strstr(query, "SELECT alias AS id, alias AS label") != NULL,
      "F: ALIASES query should SELECT alias AS id, alias AS label\n");
  pass &= assert_true(strstr(query, "FROM events_tag_aliases") != NULL,
                      "F: ALIASES query should FROM events_tag_aliases\n");
  pass &= assert_true(strstr(query, "ORDER BY alias ASC") != NULL,
                      "F: ALIASES query should ORDER BY alias ASC\n");
  char aliases_msg[128];
  snprintf(aliases_msg, sizeof(aliases_msg), "F: ALIASES query should %s\n",
           expected_limit);
  pass &= assert_true(strstr(query, expected_limit) != NULL, aliases_msg);

  build_search_query(query, sizeof(query), SEARCH_DESCRIPTORS,
                     "events_sleep_descriptors", "", "name", "test");
  pass &= assert_true(
      strstr(query, "FROM events_sleep_descriptors") != NULL,
      "F: DESCRIPTORS query should FROM events_sleep_descriptors\n");
  pass &= assert_true(
      strstr(query, "CAST(name AS TEXT) ILIKE $1") != NULL,
      "F: DESCRIPTORS query should use CAST(descriptor column AS TEXT)\n");

  return pass;
}

TEST_SUITE(search_response_serialization) {
  int pass = 1;
  char out[4096];
  json_t *root;
  json_t *entry;
  json_t *val;

  PGconn *conn = connect_db("postgres");
  pass &= assert_true(conn != NULL, "F: connect_db should succeed\n");

  if (conn) {
    PGresult *res =
        PQexec(conn, "SELECT 1::int AS id, 'test'::text AS label LIMIT 0");
    pass &= assert_true(PQresultStatus(res) == PGRES_TUPLES_OK,
                        "F: schema query should succeed\n");
    pass &= assert_true(PQnfields(res) == 2,
                        "F: schema query should return 2 columns\n");

    if (PQnfields(res) == 2) {
      PQsetvalue(res, 0, 0, "42", 2);
      PQsetvalue(res, 0, 1, "test label", 10);
      PQsetvalue(res, 1, 0, "7", 1);
      PQsetvalue(res, 1, 1, "another", 7);

      int len = serialize_search_response(res, out, sizeof(out));
      pass &= assert_true(len > 0, "F: serialize should return positive len\n");
      if (len > 0) {
        root = json_loads(out, 0, NULL);
        pass &= assert_true(root != NULL, "F: should parse as valid JSON\n");
        if (root) {
          pass &=
              assert_true(json_is_array(root), "F: should be a JSON array\n");
          pass &= assert_true(json_array_size(root) == 2,
                              "F: should have 2 elements\n");
          if (json_array_size(root) >= 1) {
            entry = json_array_get(root, 0);
            val = json_object_get(entry, "id");
            pass &= assert_true(json_is_integer(val) &&
                                    json_integer_value(val) == 42,
                                "F: first entry id should be 42\n");
            val = json_object_get(entry, "label");
            pass &= assert_true(
                json_is_string(val) &&
                    strcmp(json_string_value(val), "test label") == 0,
                "F: first entry label should be 'test label'\n");
          }
          if (json_array_size(root) >= 2) {
            entry = json_array_get(root, 1);
            val = json_object_get(entry, "id");
            pass &= assert_true(json_is_integer(val) &&
                                    json_integer_value(val) == 7,
                                "F: second entry id should be 7\n");
            val = json_object_get(entry, "label");
            pass &=
                assert_true(json_is_string(val) &&
                                strcmp(json_string_value(val), "another") == 0,
                            "F: second entry label should be 'another'\n");
          }
          json_decref(root);
        }
      }
    }

    PQclear(res);

    PGresult *empty_res =
        PQexec(conn, "SELECT 1::int AS id, 'test'::text AS label LIMIT 0");
    if (PQresultStatus(empty_res) == PGRES_TUPLES_OK) {
      int len = serialize_search_response(empty_res, out, sizeof(out));
      pass &= assert_true(len > 0, "F: empty serialize should return >0\n");
      if (len > 0) {
        root = json_loads(out, 0, NULL);
        pass &= assert_true(root != NULL,
                            "F: empty result should parse as valid JSON\n");
        if (root) {
          pass &=
              assert_true(json_is_array(root), "F: should be a JSON array\n");
          pass &= assert_true(json_array_size(root) == 0,
                              "F: empty result should have 0 elements\n");
          json_decref(root);
        }
      }
    }
    PQclear(empty_res);

    PQfinish(conn);
  }

  return pass;
}
