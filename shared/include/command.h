int strequal(const char *a, const char *b)
{
    return !strcmp(a, b);
}

// Shell commands and functions used by gateway and shell
// These should be overwritten by each application
// for their respective needs
typedef struct {
    char *name;
    int args;
    void (*func)(void *data, char *argv[]);
} command;

void process_file_transfer_request(void *data, char *args[]);

command commands[] = {
    {"GET", 2, process_file_transfer_request}
};

static int total_shell_commands = sizeof(commands) / sizeof(command);

static int run_command(char *cmd, int argc, char *argv[])
{
    int i;
    for(i = 0; i < total_shell_commands; i++)
    {
        if(strequal(cmd, commands[i].name))
        {
            if(argc != commands[i].args)
            {
                printf("Invalid number of arguments for command %s. Expected %d, got %d.\n", cmd, commands[i].args, argc);
                return -1;
            }
            commands[i].func(&commands[i], argv);
            return 0;
        }
    }

    printf("Unrecognized command: %s\n", cmd);
    return -1;
}

