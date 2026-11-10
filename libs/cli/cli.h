#ifndef __CLI_H__
#define __CLI_H__

#include <stdint.h>

#define CLI_MAX_ARGS 8


typedef void (*cli_handler_t)(int argc, char **argv);


typedef struct cli_command 
{
    const char *name;
    const char *help;
    
    cli_handler_t handler;

    struct cli_command *next;

} cli_command_t;


void CLI_Init(void);
void CLI_RegisterCommand(cli_command_t *new_command);


void CLI_ProcessLine(char *line);


#endif