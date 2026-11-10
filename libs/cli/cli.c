#include "cli.h"
#include <stdio.h>
#include <string.h>

static cli_command_t *command_list = NULL;


void CLI_RegisterCommand(cli_command_t *new_command) 
{
    if (new_command == NULL) 
        return;
    
    new_command->next = command_list;
    command_list = new_command;
}


void CLI_ProcessLine(char *line) 
{
    char *argv[CLI_MAX_ARGS];
    int argc = 0;

    char *token = strtok(line, " ");
    while (token != NULL && argc < CLI_MAX_ARGS) 
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) 
        return;

    
    cli_command_t *current = command_list;
    while (current != NULL) 
    {
        if (strcmp(current->name, argv[0]) == 0) 
        {
            current->handler(argc, argv);
            return;
        }

        current = current->next;
    }

    printf("Error: Unknown command. Type 'help'.\n");
}