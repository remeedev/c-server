#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_LENGTH 16
#define SALT_LENGTH 16

char* salt_file_name = "./db/.salt.txt";

char *gen_random_seq(int len){
    srand(time(NULL));
    char *out = (char *)malloc(len);
    char *possible_letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int i = 0;
    while (i < FILE_LENGTH){
        out[i] = possible_letters[rand()%strlen(possible_letters)];
        if (i == FILE_LENGTH-1){
            out[i]='\0';
        }
        i++;
    }
    return out;
}

char *check_setup(){
    FILE *salt = fopen(salt_file_name, "r");
    if (salt == NULL){
        printf("No salt defined, creating...\n");
        salt = fopen(salt_file_name, "w");
        if (salt == NULL){
            printf("Couldn't create salt file!\n");
        }
        char *created_salt = gen_random_seq(SALT_LENGTH);
        fprintf(salt, "%s", created_salt);
        free(created_salt);
        fclose(salt);
    }
    salt = fopen(salt_file_name, "r");

    fseek(salt, 0L, SEEK_END);
    long length = ftell(salt);
    rewind(salt);
    
    char *content = (char *)malloc(length+1);
    fread(content, sizeof(char), length, salt);
    content[length] = '\0';
    fclose(salt);
    return content;
}

char *hash_pw(char *pw, char *salt){
    bool changed_salt = false;
    if (salt == NULL){
        salt = check_setup();
        changed_salt = true;
    }
    char *salted_pw = (char *)malloc(strlen(pw)+strlen(salt)+1);
    sprintf(salted_pw, "%s%s", pw, salt);
    char *file_name = (char *)malloc(FILE_LENGTH+4);
    char *file_seq = gen_random_seq(FILE_LENGTH);
    strcpy(file_name, file_seq);
    strcat(file_name, ".txt");
    free(file_seq);
    
    FILE *content = fopen(file_name, "w");
    if (content == NULL){
        printf("Error creating file!\n");
        free(file_name);
        free(salted_pw);
        if (changed_salt){
            free(salt);
        }
        return "";
    }
    fprintf(content, "%s", salted_pw);
    free(salted_pw);
    fclose(content);
    char *cmd_raw = "sha256sum %s";
    char *cmd = (char *)malloc(strlen(cmd_raw)+strlen(file_name)-1);
    sprintf(cmd, cmd_raw, file_name);
    // Running the command to hash
    FILE *file = popen(cmd, "r");
    free(cmd);
    char buffer[65];
    if (fscanf(file, "%64s", buffer) != 1){
        printf("Error hashing the password!\n");
    }
    pclose(file);
    if (remove(file_name) != 0){
        printf("Error deleting the file!\n");
    }
    free(file_name);
    if (changed_salt){
        free(salt);
    }
    char *out = (char *)malloc(65);
    strcpy(out, buffer);
    return out;
}

bool compare_pw(char *raw_pw, char *hash){
    char *set_salt = check_setup();
    char *compare_hash = hash_pw(raw_pw, set_salt);
    free(set_salt);
    bool out = strcmp(compare_hash, hash)==0;
    free(compare_hash);
    return out;
}
