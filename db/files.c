#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

void add_files(){}

int main(int argc, char *argv[]){
    DIR *files = opendir("./files");
    if (files == NULL){
        printf("directory couldn't be opened!\n");
        exit(1);
    }
    struct dirent* file_info;
    while ((file_info = readdir(files)) != NULL){
        if (file_info->d_name[0] != '.'){
            printf("File found: %s\n", file_info->d_name);
        }
    }
    closedir(files);
    return 0;
}
