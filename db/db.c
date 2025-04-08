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
    char *create_users= "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, mail TEXT, pw TEXT)";
    char *create_files = "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path TEXT, name TEXT, added_by INTEGER, last_edit INTEGER)";
    sqlite3_stmt *statement; 
    sqlite3_prepare(db, create_users, -1, &statement, NULL);
    if (sqlite3_step(statement)!=SQLITE_DONE){
        printf("Wasn't able to create user table!\n");
        exit(1);
    }
    sqlite3_finalize(statement);
    sqlite3_prepare(db, create_files, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE){
        printf("Wasn't able to create file table!\n");
        exit(1);
    }
    sqlite3_finalize(statement);
}

int main(int argc, char *argv[]){
    sqlite3 *db = prepare_table();
    setup(db);
    sqlite3_close(db);
    return 0;
}
