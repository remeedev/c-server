#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <string.h>
#include "db.h"

bool file_in_db(sqlite3 *_db, char *name, char *path){
    char *full_path = (char *)malloc(strlen(name)+strlen(path)+8);
    sprintf(full_path, "./files%s%s", path, name);
    char *find_cmd = "SELECT path, name FROM files WHERE name = '%s' AND path = '%s'";
    char *cmd = (char *)malloc(strlen(find_cmd)-3+strlen(full_path));
    sprintf(cmd, find_cmd, name, path);
    sqlite3_stmt *stmt = run_func(_db , cmd);
    if (stmt == NULL){
        sqlite3_finalize(stmt);
        free(cmd);
        free(full_path);
        return false;
    }
    bool found = false;
    int response = SQLITE_ROW;
    while (found == false && response == SQLITE_ROW){
        const unsigned char *file_path = sqlite3_column_text(stmt, 0);
        const unsigned char *file_name = sqlite3_column_text(stmt, 1);
        if (strcmp((char *)file_path, path) == 0 && strcmp((char *)file_name, name)==0){
            found = true;
        }
        response = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    free(full_path);
    free(cmd);
    return found;
}

bool file_in_fs(char *name, char* path){
    char *full_path = (char *)malloc(strlen(name)+strlen(path)+8);
    sprintf(full_path, "./files%s%s", path, name);
    FILE *file = fopen(full_path, "r");
    free(full_path);
    if (file == NULL){
        return false;
    }
    fclose(file);
    return true;
}

void update_files(sqlite3 *_db){
    DIR *files = opendir("./files");
    if (files == NULL){
        printf("directory couldn't be opened!\n");
        exit(1);
    }
    char *path = "/";
    struct dirent* file_info;
    while ((file_info = readdir(files)) != NULL){
        if (file_info->d_name[0] != '.'){
            if (file_in_db(_db, file_info->d_name, path) == false){
                char *full_path = (char *)malloc(strlen(file_info->d_name)+strlen(path)+1);
                sprintf(full_path, "%s%s", path, file_info->d_name);
                printf("[%s] : file not registered in db!\n", full_path);
                create_file(_db, path, file_info->d_name, 1);
                free(full_path);
            }
        }
    }
    sqlite3_stmt *files_logged = run_func(_db, "SELECT path, name FROM files");
    int res = SQLITE_ROW;
    while (files_logged != NULL && res==SQLITE_ROW){
        const unsigned char* file_path = sqlite3_column_text(files_logged, 0);
        const unsigned char* file_name = sqlite3_column_text(files_logged, 1);
        if (!file_in_fs((char *)file_name, (char *)file_path)){
            delete_file(_db, (char *)file_name, (char *)file_path);
        }
        res = sqlite3_step(files_logged);
    }
    sqlite3_finalize(files_logged);
    closedir(files);
}

int main(int argc, char *argv[]){
    sqlite3 *_db = prepare_table();
    setup(_db);
    update_files(_db);
    sqlite3_close(_db);
    return 0;
}
