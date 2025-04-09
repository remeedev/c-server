#include <sqlite3.h>

#ifndef db
#define db

sqlite3 *prepare_table();
sqlite3_stmt *run_func(sqlite3 *db, char *stmt);
void create_user(sqlite3 *db, char* username, char* pass);
void create_file(sqlite3 *db, char* name, char* path);
void setup(sqlite3 *db);

#endif
