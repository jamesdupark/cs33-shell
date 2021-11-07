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

// initialize our job list
job_list_t *my_jobs;
int next_job = 1;

/*
 * handle_signals()
 * 
 * - Description: Handles asynchronous signals sent to foreground processes
 * shell by printing an informative message about what the signal was and what
 * it did
 * 
 * - Arguments: status: indicates the status of the child process as set by the
 * waitpid() function, pid: indicates the id of the child process group in
 * question, cmd: indicates the command used to create the child process (for
 * adding to jobs list)
 * 
 * - Usage: called after returning from waitpid(). If a child process terminated
 * or was stopped due to a signal, uses the signal macros to identify and report
 * it.
 */
void handle_signals(int status, pid_t pgid, char *cmd) {
    char *act;
    int job = get_job_pid(my_jobs, pgid);
    int sig = 0; // there is no zero signal
    if (WIFSIGNALED(status)) { // process terminated by signal
        sig = WTERMSIG(status);
        act = "terminated by signal";

        if (job < 0) { // job is new
            job = next_job;
        }
    } else if (WIFSTOPPED(status)) { // process stopped by signal
        sig = WSTOPSIG(status);
        act = "suspended by signal";

        if (job < 0) { // job is new
            // add job to list
            add_job(my_jobs, next_job, pgid, STOPPED, cmd);
            job = next_job;
            next_job++;
        }
    } 

    if (sig) { // there was some signal sent
        // print message
        char output[64];
        snprintf(output, 64, "[%d] (%d) %s %d\n", job, pgid, act, sig);
        checked_stdwrite(output);
    }
}

/*
 * reap()
 * 
 * - Description: Called whenever a child process (background) has changed state
 * in order to report change in process state and reap zombie processes that
 * have terminated
 * 
 * - Arguments: status: int containing status information as set by waitpid()
 * 
 * - Usage: called after a positive (some child process) return to waitpid()
 * with the -1 (all child processes) argument and the WNOHANG, WUNTRACED, and
 * WIFCONTINUED flags. Reports the change in the process state and updates job
 * list appropriately.
 *
 */
void reap(int status, pid_t pgid) {
    int code = 0;
    char act[64];
    int job = get_job_jid(my_jobs, pgid);

    if (WIFSIGNALED(status)) { // process terminated by signal
        code = WTERMSIG(status);
        snprintf(act, 32, "terminated by signal %d", code);

        // remove job from list
        remove_job_pid(my_jobs, pgid);
    } else if (WIFSTOPPED(status)) { // process stopped by signal
        code = WSTOPSIG(status);
        snprintf(act, 32,  "suspended by signal %d", code);

        // update job status
        update_job_pid(my_jobs, pgid, STOPPED);
    } else if (WIFCONTINUED(status)) {
        code = 1;
        snprintf(act, 16, "resumed");

        // update status in job list
        update_job_pid(my_jobs, pgid, RUNNING);
    } else if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
        snprintf(act, 64, "terminated with exit status %d", code);
        code = 1;

        // remove from job list
        remove_job_pid(my_jobs, pgid);
    }

    if (code) {
        char output[128];
        snprintf(output, 128, "[%d] (%d) %s\n", job, pgid, act);
        checked_stdwrite(output);
    }
}

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
        if (argc != 1) {
            write(STDERR_FILENO, "exit: syntax error\n", 20);
        } else {
            cleanup_job_list(my_jobs);
            exit(0);
        }

        // builtin recognized as cd
    } else if (!strncmp(cmd, "cd", 3)) {
        if (argc != 2) {  // no filepath to cd
            write(STDERR_FILENO, "cd: syntax error\n", 17);
        } else if (chdir(argv[1]) < 0) {  // chdir errors
            perror("cd");
        }

        // builtin recognized as ln
    } else if (!strncmp(cmd, "ln", 3)) {
        if (argc != 3) {
            write(STDERR_FILENO, "ln: syntax error\n", 17);
        }
        if (link(argv[1], argv[2]) < 0) {
            perror("ln");
        }

        // builtin recognized as rm
    } else if (!strncmp(cmd, "rm", 3)) {
        if (argc != 2) {
            write(STDERR_FILENO, "rm: syntax error\n", 17);
        } else if (unlink(argv[1]) < 0) {
            perror("rm");
        }

        // builtin recognized as jobs
    } else if (!strncmp(cmd, "jobs", 5)) {
        if (argc != 1) {
            write(STDERR_FILENO, "jobs: syntax error\n", 20);
        } else {
            jobs(my_jobs);
        }

        //builtin recognized as fg
    } else if (!strncmp(cmd, "fg", 3)) {
        if (argc != 2) {
            write(STDERR_FILENO, "fg: syntax error\n", 18);
        } else if (*argv[1] != '%') { // leading %
            write(STDERR_FILENO, "fg: job input does not begin with %\n", 37);
        } else {
            // get jid
            char *jid_str = argv[1];
            jid_str++;
            int jid = atoi(jid_str); // some number or -1

            // get pid
            pid_t pid;
            int status;
            if ((pid = get_job_pid(my_jobs, jid)) < 0) {
                write(STDERR_FILENO, "job not found\n", 15);
            } else {
                kill(pid, SIGCONT); // continue
                update_job_pid(my_jobs, pid, RUNNING); // update job list
                checked_waitpid(pid, &status, WUNTRACED); // wait
                handle_signals(status, pid, NULL);
            }
        }

        //builtin recognized as bg
    } else if (!strncmp(cmd, "bg", 3)) {
        if (argc != 2) {
            write(STDERR_FILENO, "bg: syntax error\n", 18);
        } else if (*argv[1] != '%') { // leading %
            write(STDERR_FILENO, "bg: job input does not begin with %\n", 37);
        } else {
            // get jid
            char *jid_str = argv[1];
            jid_str++;
            int jid = atoi(jid_str); // some number or -1

            // get pid
            pid_t pid;
            if ((pid = get_job_pid(my_jobs, jid)) < 0) {
                write(STDERR_FILENO, "job not found\n", 15);
            } else {
                kill(pid, SIGCONT); // continue
                update_job_pid(my_jobs, pid, RUNNING); // update job list
            }
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
int *run_prog(char *argv[512], char *tokens[512], int redir[4]) {
    pid_t pid;
    int status;
    int bg = redir[3];

    // find index of full file path in tokens array
    int f_index = 0;
    for (int i = 0; i < 3; i++) {
        if ((redir[i] == 1) || (f_index == 1 && redir[i] == 3)) {
            f_index += 2;
        }
    }

    if ((pid = fork()) == 0) {  // start child process
        // change pgid
        if ((pid = getpid()) < 0) {
            perror("getpid"); // no need to clean up job list in child process
            exit(1);
        } else if (setpgid(pid, pid) < 0) {
            perror("setpgid");
            exit(1);
        }

        // set terminal control group to pid if this is a fg job
        // TODO: if (!command ends with & symbol)
        if (!bg) { // give terminal control to foreground processes
            checked_setpgrp(pid);
        }

        // TODO: later... set up other signal handlers?
        // reset signal handlers to default
        change_def_handlers(SIG_DFL);

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

        if (!strncmp(argv[0], "/", 1)) {
            (argv[0])++;  // remove starting '/' from argv[0]
        }

        // execute
        execv(tokens[f_index], argv);
        perror("execv");

        exit(1);
    }

    if (bg) { // job set up in background
        // add job to job list
        add_job(my_jobs, next_job, pid, RUNNING, tokens[f_index]);
        next_job++;

        // print job and process id
        char output[32];
        int job = get_job_jid(my_jobs, pid);
        snprintf(output, 32, "[%d] (%d)\n", job, pid);
        checked_stdwrite(output);
    } else {
        // wait for child process
        checked_waitpid(pid, &status, WUNTRACED);
        handle_signals(status, pid, tokens[f_index]);
    }

    // return terminal control to parent
    pid_t old;
    if ((old = getpgrp()) < 0) {
        perror("getpgid");
        cleanup_job_list(my_jobs);
        exit(1);
    }

    checked_setpgrp(old);

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

    my_jobs = init_job_list();

    do {
        // setting up default signal behaviors for our shell
        change_def_handlers(SIG_IGN);

        // set up buffers
        char buf[1024];
        memset(buf, 0, 1024);
        char *tokens[512];
        memset(tokens, 0, 512 * sizeof(char *));
        char *argv[512];
        memset(argv, 0, 512 * sizeof(char *));

        // check for changes in child process status and reap zombie processes
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            reap(status, pid);
        }

        // prompt user input
        #ifdef PROMPT
                checked_stdwrite("mysh> ");
        #endif

        // read in commands
        ssize_t rd_state;
        if ((rd_state = read(STDIN_FILENO, buf, 1024)) < 0) {
            perror("read");
            cleanup_job_list(my_jobs);
            return 1;
        } else if (rd_state == 0) {  // only ctrl + D was entered
            cleanup_job_list(my_jobs);
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
        int redir[4] = {0, 0, 0, 0};
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
