#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sqlite3 *prepare_table(){
    sqlite3 *db;
    if (sqlite3_open("./db/data.db", &db) != SQLITE_OK){
        printf("Error creating/opening the database!\n");
        exit(1);
    }
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
            printf("Error: %s\n", sqlite3_errmsg(db));
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
        sqlite3_finalize(get_user);
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
    sqlite3_finalize(check_exists);
    free(check_stmt);
    free(stmt);
}

void create_file(sqlite3 *db, char *path, char *name, int uid){
    char *new_file = "INSERT INTO files (name, path, added_by) VALUES ('%s', '%s', %d)";
    char uid_buff[10];
    sprintf(uid_buff, "%d", uid);
    char *stmt = (char *)malloc(strlen(path)+strlen(name)+strlen(new_file)-5+strlen(uid_buff));
    sprintf(stmt, new_file, name, path, uid);
    char *check = "SELECT * FROM files WHERE name = '%s' AND path = '%s'";
    char *check_stmt = (char *)malloc(strlen(check)+strlen(name)+strlen(path)-3);
    sprintf(check_stmt, check, name, path);
    sqlite3_stmt *check_exists = run_func(db, check_stmt);
    if (check_exists == NULL){
        run_func(db, stmt);
        sqlite3_stmt *get_file = run_func(db, check_stmt);
        char fid[10];
        int file_id_raw = sqlite3_column_int(get_file, 0);
        sqlite3_finalize(get_file);
        sprintf(fid, "%d", file_id_raw);
        time_t seconds = time(NULL);
        char time_since[15];
        sprintf(time_since, "%ld", seconds);
        char *update_log = "INSERT INTO file_log (file_id, action, time) VALUES (%s, 0, %s)";
        char *raw_stmt = (char *)malloc(strlen(update_log)-3+strlen(time_since)+strlen(fid));
        sprintf(raw_stmt, update_log, fid, time_since);
        run_func(db, raw_stmt);
        free(raw_stmt);
    }
    sqlite3_finalize(check_exists);
    free(check_stmt);
    free(stmt);
}

void delete_file(sqlite3 *db, char *name, char *path){
    printf("Deleting file [%s%s]!\n", path, name);
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
    create_user(db, "testing", "password");
}
