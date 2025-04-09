#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

sqlite3_stmt *run_func(sqlite3 *db, char *stmt){
    sqlite3_stmt *statement;
    if (sqlite3_prepare(db, stmt, -1, &statement, NULL) != SQLITE_OK){
        printf("Error preparing statement: %s\n", sqlite3_errmsg(db));
    }
    int res = sqlite3_step(statement);
    if (res==SQLITE_ROW){
        return statement;
    }
    if (res != SQLITE_DONE){
        printf("Wasn't able to perform action [%s]\n", stmt);
        if (res == SQLITE_ERROR){
            printf("Error!\n");
        }
        if (res == SQLITE_BUSY){
            printf("Busy!\n");
        }
        if (res == SQLITE_MISUSE){
            printf("Misuse!\n");
        }
        exit(1);
    }
    sqlite3_finalize(statement);
    return NULL;
}

void create_user(sqlite3 *db, char *username, char* pass){
    // don't judge, I am going to add the hash function to the password, just not yet
    char *new_user = "INSERT INTO users (name, pw) VALUES ('%s', '%s')";
    char *stmt = (char *)malloc(strlen(username)+strlen(pass)+strlen(new_user)-3);
    sprintf(stmt, new_user, username, pass);
    char *check = "SELECT * FROM users WHERE name = '%s'";
    char *check_stmt = (char *)malloc(strlen(check)+strlen(username)-1);
    sprintf(check_stmt, check, username);
    sqlite3_stmt *check_exists = run_func(db, check_stmt);
    if (check_exists == NULL){
        run_func(db, stmt);
        sqlite3_stmt *get_user = run_func(db, check_stmt);
        char uid[5];
        int user_id_raw = sqlite3_column_int(get_user, 0);
        sprintf(uid, "%d", user_id_raw);
        time_t seconds = time(NULL);
        char time_since[15];
        sprintf(time_since, "%ld", seconds);
        char *update_log = "INSERT INTO user_log (user_id, action, time) VALUES (%s, 0, %s)";
        char *raw_stmt = (char *)malloc(strlen(update_log)-3+strlen(time_since)+strlen(uid));
        sprintf(raw_stmt, update_log, uid, time_since);
        run_func(db, raw_stmt);
        free(raw_stmt);
    }
    free(check_stmt);
    free(stmt);
}

void setup(sqlite3 *db){
    char *create_users= "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, pw TEXT, sessionKey TEXT)";
    char *create_files = "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path TEXT, name TEXT, added_by INTEGER, last_edit INTEGER)";
    char *create_log_file = "CREATE TABLE IF NOT EXISTS file_log (id INTEGER PRIMARY KEY, file_id INTEGER, action INTEGER, time INTEGER)";
    char *create_log_user = "CREATE TABLE IF NOT EXISTS user_log (id INTEGER PRIMARY KEY, user_id INTEGER, action INTEGER, time INTEGER)";
    run_func(db, create_users);
    run_func(db, create_files);
    run_func(db, create_log_file);
    run_func(db, create_log_user);
    //sqlite3_stmt *pc = run_func(db, "SELECT * FROM users WHERE ");
    create_user(db, "testing", "password");
}

int main(int argc, char *argv[]){
    sqlite3 *db = prepare_table();
    setup(db);
    sqlite3_close(db);
    return 0;
}
