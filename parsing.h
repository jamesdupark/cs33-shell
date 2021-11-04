/*
 * Brown University - Department of Computer Science
 * CS033 - Introduction To Computer Systems - Fall 2020
 * Prof. Thomas W. Doeppner
 */

#include <stddef.h>

#ifndef PARSING
#define PARSING

/* function declaration */
int parse(char buffer[1024], char *tokens[512], char *argv[512], int redir[3]);
int set_tok(char **tok_ptr, int mode, int i, int *offset, char *tokens[512],
            int *redir);
int id_rd_tok(char *tok);
char *handle_redir(char *tok, char *tokens[512], int *offset, int *redir,
                   int i);

#endif
