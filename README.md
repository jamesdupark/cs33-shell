# 6-shell-2

Shell program with basic builtins, ability to run programs in foreground or
background, extensive error checking, I/O redirection, signal handling, job 
control, and reaping zombie processes. Program initializes a REPL that reads 
from STIDN, parses into whitespace-separated tokens, and attempts to act upon 
user inputs. exits when it reads the exit command, EOF (ctrl+D), or recieves the
SIGQUIT signal (ignores SIGINT and SIGTSTP). Can be compiled with the -DPROMPT 
macro, which enables a prompt to be printed on each line before accepting user 
input.

### Builtins supported:
- **exit:** exits the shell
- **cd <dir>:** changes working directory to <dir>
- **rm <file>:** removes <file> from current directory
- **ln <file1> <file2>:** creates hardlink between <file1> and <file2>
- **jobs:** prints list of currently running jobs
- **fg %<jid>:** brings job <jid> to foreground; resumes if stopped
- **bg %<jid>:** resumes job <jid> in background

All other inputs are assumed to be attempts to execute programs (i.e. /bin/ls),
and will be attempted with the execv system call.

### C Files in this project:
- **sh.c:** contains main method with shell REPL, functions for executing 
builtins, programs, reaping zombie processes, and handling jobs.
- **jobs.c:** contains code pertaining to the usage and maintenence of jobs,
including a typedef for a joblist_t struct and functions to add,
remove, update, and get information about jobs from the list.
- **parsing.c:** contains code to parse user input, including redirection tokens
and the ampersand (&) operand, which indicates that a job should be
started in the background if it is the last token in a line of input
- **lib_checks.c:** contains commonly used library functions packaged in an
error-checked manner.
