CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror
SHHEADERS = parsing.h
SHFILES = sh.c parsing.c
EXECS = 33sh 33noprompt

PROMPT = -DPROMPT

.PHONY: all alltest clean tests sanitize

all: $(EXECS)

alltest: $(EXECS) tests sanitize

33sh: $(SHFILES) $(SHHEADERS)
	gcc $(CFLAGS) $(PROMPT) $(SHFILES) -o $@

33noprompt: $(SHFILES) $(SHHEADERS)
	gcc $(CFLAGS) $(SHFILES) -o $@

tests: ./cs0330_shell_1_test 33noprompt ./shell_1_tests
	./$< -s 33noprompt -u ./shell_1_tests

sanitize: ./cs0330_cleanup_shell
	$<

clean:
	rm -f $(EXECS)