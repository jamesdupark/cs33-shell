#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "parsing.h"
#include "lib_checks.c"
#include "jobs.h"


/*
 * change_def_handlers()
 * 
 * - Description: Sets the default behavior for SIGINT, SIGSTP, and SIGTTOU
 * according to the given handler
 * 
 * - Arguments: handler: sighandler that defines the new behavior for the three
 * signals (i.e. SIG_DFL, SIG_IGN)
 * 
 * - Usage: change_def_handlers(SIG_DFL) -> changes handlers for SIGINT, SIGTSTP
 * and SIGTTOU to default
 *
 */
void change_def_handlers(__sighandler_t handler) {
    checked_signal(SIGINT, handler);
    checked_signal(SIGTSTP, handler);
    checked_signal(SIGTTOU, handler);
}

/*
 * exec_builtins()
 *
 * - Description: takes in the current argv array and argc count and attempts to
 * execute one of the supported builtin commands cd, ln, rm, or exit. Returns 0
 * if a command was attempted, -1 if the command was not recognized as one of
 * the builtins.
 *
 * - Arguments: argv: an array of pointers to parsed arguments, argc: an int
 * representing the number of arguments
 *
 * - Usage:
 *          if argv[0] is:
 *              "exit" -> exits the program
 *              "cd" -> calls chdir to change working directory to argv[1]
 *              "ln" -> calls link to create a new hardlink between argv[1] and
 *  argv[2]
 *              "rm" -> calls unlink to remove argv[1]
 *          else returns -1
 */
int exec_builtins(char *argv[512], int argc) {
    char *cmd = argv[0];

    // builtin recognized as exit
    if (!strncmp(cmd, "exit", 5)) {
        exit(0);

        // builtin recognized as cd
    } else if (!strncmp(cmd, "cd", 3)) {
        if (argc < 2) {  // no filepath to cd
            write(STDERR_FILENO, "cd: syntax error\n", 17);
        } else if (chdir(argv[1]) < 0) {  // chdir errors
            perror("cd");
        }

        // builtin recognized as ln
    } else if (!strncmp(cmd, "ln", 3)) {
        if (argc < 3) {
            write(STDERR_FILENO, "ln: syntax error\n", 17);
        }
        if (link(argv[1], argv[2]) < 0) {
            perror("ln");
        }

        // builtin recognized as rm
    } else if (!strncmp(cmd, "rm", 3)) {
        if (argc < 2) {
            write(STDERR_FILENO, "rm: syntax error\n", 17);
        } else if (unlink(argv[1]) < 0) {
            perror("rm");
        }

    } else {
        // builtin not recognized, try execv
        return -1;
    }

    // successfully executed builtin
    return 0;
}

/*
 * run_prog()
 *
 * - Description: Attempts to run execv within a child process with the given
 * argv. Supports i/o redirection.
 *
 * - Arguments: argv: array of pointers to parsed arguments, tokens: array of
 * pointers to parsed tokens (including redirection symbols and files), redir:
 * array of ints indicating the index of the redirection file for input, output,
 * or appending respectively within the tokens array.
 *
 * - Usage: argv[0] should contain the name of the binary file to be executed,
 * preceded by a "/" (to prevent conflict with builtins with the same name).
 * The other elements of argv should be the inputs for the executable in
 * question. If redirects are desired, tokens should contain the input, output,
 * and/or append filepaths at the indices specified in redir.
 */
int run_prog(char *argv[512], char *tokens[512], int redir[3]) {
    pid_t pid;
    int status;

    if ((pid = fork()) == 0) {  // start child process
        // reset signal handlers to default
        change_def_handlers(SIG_DFL);

        // change pgid
        if (setpgid(pid, pid) < 0) {
            perror("setpgid");
            exit(1);
        }

        // set terminal control group to pid if this is a fg job
        // TODO: if (!command ends with & symbol)
        if (tcsetgrp(STDIN_FILENO, pid) < 0) {
            perror("tcsetgrp");
            exit(1);
        }



        // later... set up other signal handlers?

        // set up redirection
        if (redir[0]) {         // input redirection
            checked_close(STDIN_FILENO);
            checked_open(tokens[redir[0]], O_RDONLY, 0);
        }

        if (redir[1]) {  // output redirection
            checked_close(STDOUT_FILENO);
            checked_open(tokens[redir[1]], O_WRONLY | O_TRUNC | O_CREAT, 0600);
        } else if (redir[2]) {
            checked_close(STDOUT_FILENO);
            checked_open(tokens[redir[2]], O_WRONLY | O_APPEND | O_CREAT, 0600);
        }

        // find index of full file path in tokens array
        int f_index = 0;
        for (int i = 0; i < 3; i++) {
            if ((redir[i] == 1) || (f_index == 1 && redir[i] == 3)) {
                f_index += 2;
            }
        }

        (argv[0])++;  // remove starting '/' from argv[0]

        // execute
        execv(tokens[f_index], argv);
        perror("execv");
        exit(1);
    }

    if (waitpid(pid, &status, 0) < 1) {  // wait for child to finish
        perror("wait");
        return -1;
    }

    return 0;
}

/*
 * main()
 *
 * - Description: Sets up and executes a fully funcitonal REPL shell with built-
 * in commands rm, ln, cd, and exit with extensive error-checking. Attempts to
 * execute commands that do not correspond to builtins.
 *
 * - Arguments: none
 *
 * - Usage: type in commands to the REPL like you normall would in a shell!
 *          supports cd, rm, ln, exit, exiting with ctrl+D, and executing
 *          programs with execv, along with file redirection using "<", ">", and
 *          ">>".
 */
int main() {
    
    // setting up default signal behaviors for our shell
    change_def_handlers(SIG_IGN);


    do {
        // set up buffers
        char buf[1024];
        memset(buf, 0, 1024);
        char *tokens[512];
        memset(tokens, 0, 512 * sizeof(char *));
        char *argv[512];
        memset(argv, 0, 512 * sizeof(char *));

// prompt user input
#ifdef PROMPT
        if (write(STDOUT_FILENO, "mysh> ", 6) < 0) {
            perror("write");
            return 1;
        }
#endif

        // read in commands
        ssize_t rd_state;
        if ((rd_state = read(STDIN_FILENO, buf, 1024)) < 0) {
            perror("read");
            return 1;
        } else if (rd_state == 0) {  // only ctrl + D was entered
            return 0;
        }

        // null-terminating the buffer
        if (buf[rd_state - 1] != '\n') {  // ctrl + D terminated input
            buf[rd_state] = '\0';
            rd_state++;
        } else if (rd_state == 1) {  // only char is newline
            continue;
        } else {  // some characters + newline
            buf[rd_state - 1] = '\0';
            rd_state++;
        }

        // read was successful, parse input
        int redir[3] = {0, 0, 0};
        int argc;
        if ((argc = parse(buf, tokens, argv, redir)) < 0) {
            // buf was empty or i/o redirection error was found
            continue;
        }

        // execute builtins
        if (exec_builtins(argv, argc) < 0) {
            // if argv[0] not a builtin: attempt to execute program
            run_prog(argv, tokens, redir);
        }

    } while (1);  // continues until ctrl+D is pressed or other fatal error
}
