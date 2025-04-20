#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_LENGTH 16
#define SALT_LENGTH 16

char* salt_file_name = ".salt.txt";

char *gen_random_seq(int len){
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

int main(int argc, char *argv[]){
    srand(time(NULL));
    char *salt_created;
    if (argc > 1){
        if (strcmp(argv[1], "-c") == 0){
            if (argc != 4){
                printf("Usage:\n./hash -c <hash> <pw>\n");
                return 1;
            }
            if (compare_pw(argv[3], argv[2])){
                printf("Password and hash match!\n");
            }else{
                printf("Password and hash don't match!\n");
            }
            return 0;
        }
        int i = 1;
        salt_created = check_setup();
        while (i < argc){
            char *hashed = hash_pw(argv[i], salt_created);
            printf("Hashed pw %d: [%s]\n", i, hashed);
            free(hashed);
            i++;
        }
    }
    free(salt_created);
    return 0;
}
