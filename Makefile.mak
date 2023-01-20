CFLAGS = -Wall -Werror -Wextra

sshell: sshell.c
        gcc $(CFLAGS) sshell.c -o sshell

clean:
        rm -f sshell