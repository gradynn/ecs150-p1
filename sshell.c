#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

enum error {
        NO_ERR,
        ERR_TOO_MANY_ARGS,
        ERR_MISS_CMD,
        ERR_NO_OUT,
        ERR_CANT_OPEN_OUT,
        ERR_OUT_REDIR,
        ERR_CANT_CD,
        ERR_CMD_NOT_FND
};

struct Command {
	char cmd[CMDLINE_MAX];
	char* args[16];
        char* path;
};

void error_mgmt(enum error err)
{
        switch (err) {
        case ERR_TOO_MANY_ARGS:
                fprintf(stderr, "Error: too many process arguments\n");
                break;
        case ERR_MISS_CMD:
                fprintf(stderr, "Error: missing command\n");
                break;
        case ERR_NO_OUT:
                fprintf(stderr, "Error: no output file\n");
                break;
        case ERR_CANT_OPEN_OUT:
                fprintf(stderr, "Error: cannot open output file\n");
                break;
        case ERR_OUT_REDIR:
                fprintf(stderr, "Error: mislocated output redirection\n");
                break;
        case ERR_CANT_CD:
                fprintf(stderr, "Error: cannot cd into directory\n");
                break;
        case ERR_CMD_NOT_FND:
                fprintf(stderr, "Error: command not found\n");
                break;
        default:
                break;
        }
}

int psuedo_system(struct Command* command)
{
        /* Handles change of directory without creating new process */
        if (!strcmp(command->args[0], "cd")) {
                int ch_status = chdir(command->args[1]);
                return WEXITSTATUS(ch_status);
        }

        if (!strcmp(command->args[0], "pwd")) {
                char cwd[CMDLINE_MAX];
                getcwd(cwd, sizeof(cwd));
                printf("%s\n", cwd);
                return 0;
        }

        int pid = fork();
	int status = 0;

        if (pid == 0) { // child
                /* Support for output redirection */
                if (command->path != NULL) {
                        int file_o = open(command->path, O_RDWR | O_CREAT, 0644);
                        //if (file_o == ERROR OUTPUT OF OPEN)
                                // use this to throw a relevant error
                        dup2(file_o, STDOUT_FILENO);
                        close(file_o);
                }
                execvp(command->args[0], command->args);
                exit(1);
        }
        else { // parent
                waitpid(pid, &status, 0);
        }

	return WEXITSTATUS(status);
}

enum error command_parse(struct Command* command, char* raw_cmd)
{
        /* isolate output redirection path */
        char* w_cmd = strtok(raw_cmd, ">");
        char* redirect_path = strtok(NULL, ">");
        command->path = redirect_path;

        /* isolate/assign command */
        char* token = strtok(w_cmd, " ");
        strcpy(command->cmd, token);
        command->args[0] = token;

        token = strtok(NULL, " ");
        int i = 1;
        do {
                if (i == 16)
                        return ERR_TOO_MANY_ARGS;
                command->args[i] = token;
                token = strtok(NULL, " ");
                i++;
        } while (token != NULL);
        command->args[i] = NULL;

        return 0;
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                char cmd_cpy[CMDLINE_MAX];
                strcpy(cmd_cpy, cmd);

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Iterate through and build list of commands (each parsed 
                        individually */
                int i = 1;
                char* token = strtok(cmd, "|");
                while (token != NULL) {
                        struct Command temp_command;
                        command_parse(&temp_command, token);
                        commands[i] = temp_command;
                        ++i;
                        token = strtok(NULL, "|");
                }

                /* Iterate through list of commands and execute sudo, system
                        each time */
                for(int i = 0; i < cmd_count; i++) {
                        retval = psuedo_system(&commands[i]);
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd_cpy, retval);
                }

                // fprintf(stderr, "+ completed '%s' [%d]\n", cmd_cpy, retval);
        }

        return EXIT_SUCCESS;
}
