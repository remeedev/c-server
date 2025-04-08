#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

sqlite3 *prepare_table(){
    FILE *db_file = fopen("data.db", "r");
    if (db_file == NULL){
        printf("Database file is non-existent, creating file!\n");
        db_file = fopen("data.db", "w");
    }
    fclose(db_file);
    sqlite3 *db;
    sqlite3_open("data.db", &db);
    if (db == NULL){
        printf("Memory allocation error for db!\n");
        exit(1);
    }
    return db;
}

void setup(sqlite3 *db){
    char *statement_raw = "CREATE TABLE IF NOT EXISTS testing (id INTEGER PRIMARY KEY, name TEXT, num INTEGER)";
    sqlite3_stmt *statement; 
    sqlite3_prepare(db, statement_raw, -1, &statement, NULL);
    if (sqlite3_step(statement)==SQLITE_DONE){
        printf("database setup properly!\n");
    }
    sqlite3_finalize(statement);
    char *bogus_add = "INSERT INTO testing (name, num) VALUES ('testing', 1)";
    sqlite3_stmt *bogus_stmt;
    sqlite3_prepare(db, bogus_add, -1, &bogus_stmt, NULL);
    if (sqlite3_step(bogus_stmt) == SQLITE_DONE){
        printf("Added first bogus statement!\n");
    }
    sqlite3_reset(bogus_stmt);
    if (sqlite3_step(bogus_stmt) == SQLITE_DONE){
        printf("Added second bogus statement!\n");
    }
    sqlite3_finalize(bogus_stmt);
}

void query(sqlite3 *db){
    char *bogus_query = "SELECT * FROM testing";
    sqlite3_stmt *statement;
    sqlite3_prepare(db, bogus_query, -1, &statement, NULL);
    while (sqlite3_step(statement) == SQLITE_ROW){
        int columns = sqlite3_column_count(statement);
        int pos = 0;
        while (pos < columns){
            int type = sqlite3_column_type(statement, pos);
            if (type == SQLITE_INTEGER){
                int value = sqlite3_column_int(statement, pos);
                printf("[%d]", value);
            }
            if (type == SQLITE_FLOAT){
                float value = sqlite3_column_int(statement, pos);
                printf("[%f]", value);
            }
            if (type == SQLITE_TEXT){
                const unsigned char *value = sqlite3_column_text(statement, pos);
                printf("[%s]", value);
            }
            if (type == SQLITE_BLOB){
                printf("[%d] is a blob!\n", pos);
            }
            if (type == SQLITE_NULL){
                printf("[%d] is a null value!\n", pos);
            }
            pos++;
        }
        printf("\n");
    }
    sqlite3_finalize(statement);
}

int main(int argc, char *argv[]){
    sqlite3 *db = prepare_table();
    setup(db);
    query(db);
    sqlite3_close(db);
    return 0;
}
