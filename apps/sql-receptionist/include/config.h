#include <stdbool.h>

struct data_column {
  const char *name;                  // @todo validation
  const char *datatype;              // @todo validation
  bool comments;                     // optional, ?useless here?
  const char *entrytype;             // useless here
};

struct descriptor {
  const char *name;
  struct data_column *schema;
  unsigned int schema_count;
};

struct table {
  char *table_name;      // @todo validation
  bool read;             // @todo validation
  bool write;            // @todo validation
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
