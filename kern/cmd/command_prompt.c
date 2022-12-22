/*	Simple command-line kernel prompt useful for
	controlling the kernel and exploring the system interactively.


KEY WORDS
==========
CONSTANTS:	WHITESPACE, NUM_OF_COMMANDS
VARIABLES:	Command, commands, name, description, function_to_execute, number_of_arguments, arguments, command_string, command_line, command_found
FUNCTIONS:	readline, cprintf, execute_command, run_command_prompt, command_kernel_info, command_help, strcmp, strsplit, start_of_kernel, start_of_uninitialized_data_section, end_of_kernel_code_section, end_of_kernel
=====================================================================================================================================================================================================
 */


#include <kern/cmd/command_prompt.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/kdebug.h>
#include <kern/cons/console.h>
#include "commands.h"

//TODO: [PROJECT MS1] [COMMAND PROMPT] auto-complete
int auto_complete(char **arguments) {
    int j , length = strlen(arguments[0]), index = 0, found = 0;
    for (j = 0; j < NUM_OF_COMMANDS; j++) {
    	char substring[strlen(arguments[0] + 5)];
    	char *curr_command = commands[j].name;
    	while(index < length) {
            substring[index] = curr_command[index];
            index++;
    	}
    	substring[index]='\0';
    	index = 0;
    	if(strcmp(arguments[0], substring) == 0) {
            found = 1;
            cprintf("%s\n", commands[j].name);
            cprintf("\n");
    	}
    }
    return found;
}
//invoke the command prompt
void run_command_prompt()
{
	char command_line[1024];

	while (1==1)
	{
		//get command line
		readline("FOS> ", command_line);

		//parse and execute the command
		if (command_line != NULL)
			if (execute_command(command_line) < 0)
				break;
	}
}

/***** Kernel command prompt command interpreter *****/

//define the white-space symbols
#define WHITESPACE "\t\r\n "

//Function to parse any command and execute it
//(simply by calling its corresponding function)
int execute_command(char *command_string)
{
	// Split the command string into whitespace-separated arguments
	int number_of_arguments;
	//allocate array of char * of size MAX_ARGUMENTS = 16 found in string.h
	char *arguments[MAX_ARGUMENTS];


	strsplit(command_string, WHITESPACE, arguments, &number_of_arguments) ;
	if (number_of_arguments == 0)
		return 0;

	// Lookup in the commands array and execute the command
	int command_found = 0;
	int i ;
	for (i = 0; i < NUM_OF_COMMANDS; i++)
	{
		if (strcmp(arguments[0], commands[i].name) == 0)
		{
			command_found = 1;
			break;
		}
	}

	if(command_found)
	{
		int return_value;
		return_value = commands[i].function_to_execute(number_of_arguments, arguments);
		return return_value;
	}
	else
	{
		int foundprefix;
		foundprefix= auto_complete(arguments);
		//if not found, then it's unknown command
		if(foundprefix==0)
		cprintf("Unknown command '%s'\n", arguments[0]);
		return 0;
	}
}
