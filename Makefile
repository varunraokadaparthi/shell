#
# Makefile for shell
#

CFLAGS = -ggdb3 -Wall -pedantic -g -fstack-protector-all -fsanitize=address
shell56: shell56.c parser.c fork.c redirection.c runcommands.c
	gcc shell56.c parser.c fork.c redirection.c runcommands.c -o shell56 $(CFLAGS)

clean:
	rm -f *.o shell56
