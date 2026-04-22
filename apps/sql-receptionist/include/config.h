#include <stdbool.h>

enum table_type {
  MAIN_TABLE,
  TAGS_TABLE,
  TAG_NAMES_TABLE,
  TAG_ALIASES_TABLE,
  TAG_GROUPS_TABLE,
  DESCRIPTORS_TABLE,
  TAGGING_TABLE, // generic referring to any tagging table
};

enum column_type {
  DATA,
  COMMENTS,
  LATLONG_ACCURACY,
  ALTITUDE,
  ALTITUDE_ACCURACY,
};

struct data_column {
  const char *name;      // @todo validation
  const char *datatype;  // @todo validation
  bool comments;         // optional, ?useless here?
  const char *entrytype; // useless here
};

struct descriptor {
  const char *name;
  struct data_column *schema;
  unsigned int schema_count;
};

struct table {
  char *table_name; // @todo validation
  bool read;        // @todo validation
  bool write;       // @todo validation
  bool tagging;
  const char *entrytype; // @todo validation
  struct data_column *schema;
  unsigned int schema_count;
  struct descriptor *descriptors;
  unsigned int descriptors_count;
};

struct db {
  char *db_name; // @todo validation
  struct table *tables;
  unsigned int tables_count;
};

struct config {
  struct db *dbs;
  unsigned int dbs_count;
};

extern void load_config(struct config **cfg);
