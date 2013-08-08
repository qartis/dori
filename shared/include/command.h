int strequal(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

uint8_t from_hex(char a)
{
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
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
void process_dcim_transfer_request(void *data, char *args[]);

command commands[] = {
    {"GET", 2, process_file_transfer_request},
    {"IMG", 1, process_dcim_transfer_request}
};

static int total_shell_commands = sizeof(commands) / sizeof(command);

static int has_command(char *cmd) {
    int i;
    for(i = 0; i < total_shell_commands; i++)
    {
        if(strequal(cmd, commands[i].name)) {
            return 1;
        }
    }

    return 0;
}

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

