#include <stdio.h>
#include <signal.h>
#include <unistd.h>



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
 * - Usage: automates the error-checking process of the library funciton 
 *              signal(). Check man signal for more info on the library function
 * 
 */
void checked_signal(int signum, __sigahandler_t handler) {
    if (signal(signum, handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
}