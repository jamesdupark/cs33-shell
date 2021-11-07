#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "jobs.h"

job_list_t *my_jobs;

/*
 * checked_open()
 *
 * - Description: attempts to write to stdout with the given string
 *
 * - Arguments: str: string to try writing to stdout
 *
 * - Usage: Automates the error-checking process of the library function write()
 *              check man 2 write for more information on the library function
 *
 */
void checked_stdwrite(char* str) {
    size_t len = strnlen(str, 1024);
    if (write(STDOUT_FILENO, str, len) < 0) {
        perror("write:");
        cleanup_job_list(my_jobs);
        exit(1);
    }
}

/*
 * checked_close()
 *
 * - Description: attempts to close the given file descriptor. Prints an error
 * message if close fails and exits the program.
 *
 * - Arguments: fd: the file descriptor to try closing
 *
 * - Usage: Automates the error-checking process of the library function close()
 *              check man 2 close for more information on the library function
 */
void checked_close(int fd) {
    if (close(fd) < 0) {
        perror("close");
        cleanup_job_list(my_jobs);
        exit(1);
    }
}

/*
 * checked_open()
 *
 * - Description: attempts to open the given file descriptor with the given
 * flags and mode. Prints an error message if open fails and exits the program.
 *
 * - Arguments: pathname: full filepath to the file to try opening, flags: flags
 * to be passed to the call to open(), mode: the file permissions mode for files
 * that are newly created by open(), to be &ed with ~umask
 *
 * - Usage: Automates the error-checking process of the library function open()
 *              check man 2 open for more info on the library function
 */
void checked_open(const char *pathname, int flags, mode_t mode) {
    if (open(pathname, flags, mode) < 0) {
        perror("open");
        cleanup_job_list(my_jobs);
        exit(1);
    }
}

/*
 * checked_signal()
 * 
 * - Description: attempts to set the signal handler to the given handler using
 * the library function signal(). Prints error message if signal() fails and
 * exits the program
 * 
 * - Arguments: signum: int representing the signal whose handler behavior
 * should be changed, handler: sighandler_t to change the handler for the given
 * signal to (i.e. SIG_DFL, SIG_IGN)
 * 
 * - Usage: automates the error-checking process of the library function 
 *              signal(). Check man signal for more info on the library function
 * 
 */
void checked_signal(int signum, __sighandler_t handler) {
    if (signal(signum, handler) == SIG_ERR) {
        perror("signal");
        cleanup_job_list(my_jobs);
        exit(1);
    }
}

/*
 * checked_setpgrp()
 * 
 * - Description: attempts to set process group id to the given pgid using the
 * tcsetgrp library call. Exits program if it fails.
 * 
 * - Arguments: pgrp: id of the process group to transfer control to
 * 
 * - Usage: automates the error-checking process of the library function 
 *              tcscetpgrp(). Check man tcsetpgrp for more info
 * 
 */
void checked_setpgrp(pid_t pgrp) {
    if (tcsetpgrp(STDIN_FILENO, pgrp) < 0) {
        perror("tcsetgrp");
        cleanup_job_list(my_jobs);
        exit(1);
    }
}

/*
 * checked_waitpid()
 * 
 * - Description: calls waipid on the given inputs and prints an error message
 * if it fails and exits the program. Returns the pid of the relevant process
 * otherwise. Not compatible with the WNOHANG flag (exits if no children found)
 * 
 * - Arguments: pid: process id of the process to wait for, status: pointer to
 * an uninitialized int that can be used to store the status of the wait
 * operation, options: options for waitpid
 * 
 * - Usage: automates the error-checking process of the library function 
 *              waitpid(). Check man waitpid for more info.
 * 
 */
pid_t checked_waitpid(pid_t pid, int *status, int options) {
    pid_t ret;
    if ((ret = waitpid(pid, status, options)) < 0) {
        perror("wait");
        cleanup_job_list(my_jobs);
        exit(1);
    }

    return ret;
}
