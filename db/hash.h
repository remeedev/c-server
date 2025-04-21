#include <stdbool.h>

#ifndef hash_mod
#define hash_mod

char *gen_random_seq(int len);
char *check_setup();
char *hash_pw(char *pw, char *salt);
bool compare_pw(char *raw_pw, char *hash);

#endif
