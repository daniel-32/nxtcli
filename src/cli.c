/*
	This file is part of nxtcli.

	nxtcli is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	nxtcli is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with nxtcli.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "nxtcommands.h"

typedef enum
{
	CLI_TYPE_STRING,
	CLI_TYPE_FLAG,
	CLI_TYPE_VALUE,
} cliParameterType;

typedef struct
{
	char	name[256];
	char	value[256];
	cliParameterType	type;
	int	required;
	int	given;
} cliParameter;

typedef enum
{
	CLI_MODE_INTERACTIVE,
	CLI_MODE_ARGS,
} cliMode;

#include "builtincommands.h"	// I know this is hackish but this header requires cliParameter to be defined first

static cliMode	mMode;

static int	mArgc;
static char**	mArgv;
static int	mArgPosition;

static char*	mCommand;
static char*	mPosition;
static char*	mCommandEnd;

char* cliGetNextSegment();

static void handle_command(const char* command);
char* my_completion_generator(const char* text, int state);

void cliInit(cliMode mode, int argc, char** argv)
{
	switch (mode)
	{
		case CLI_MODE_INTERACTIVE:
			rl_completion_entry_function = my_completion_generator;
			break;
		case CLI_MODE_ARGS:
			mArgc = argc;
			mArgv = argv;
			mArgPosition = 1;
			break;
	}

	mMode = mode;
}

void cliDoCommand()
{
	if (mMode == CLI_MODE_INTERACTIVE)
	{
		mCommand = readline("nxtcli> ");
		if (mCommand != NULL)
		{
			if (strlen(mCommand) > 0)
			{
				int	current_position;

				current_position = 0;
				while (mCommand[current_position] != 0)
				{
					if (mCommand[current_position] != ' ')
					{
						break;
					}
					current_position += 1;
				}

				if (mCommand[current_position] != 0)
				{
					add_history(mCommand);
					mPosition = mCommand - 1;
					mCommandEnd = mCommand + strlen(mCommand);
					handle_command(cliGetNextSegment());
				}
			}
			free(mCommand);
		}
	}
	else if (mMode == CLI_MODE_ARGS)
	{
		handle_command(cliGetNextSegment());
	}
}

char* cliGetNextSegment()
{
	if (mMode == CLI_MODE_INTERACTIVE)
	{
		int	current_position;
		char	terminator;
		char*	result;

		if (mPosition == mCommandEnd)
		{
			return NULL;
		}
		current_position = mPosition - mCommand + 1;

		while (mCommand[current_position] != 0)
		{
			if (mCommand[current_position] != ' ')
			{
				break;
			}
			current_position += 1;
		}
		if (mCommand[current_position] == 0)
		{
			return NULL;
		}

		if (mCommand[current_position] == '"' || mCommand[current_position] == '\'')
		{
			terminator = mCommand[current_position];
			current_position = current_position + 1;
		}
		else
		{
			terminator = ' ';
		}

		result = mCommand + current_position;

		while (mCommand[current_position] != 0)
		{
			if (mCommand[current_position] == terminator)
			{
				break;
			}
			current_position += 1;
		}
		mPosition = mCommand + current_position;
		mPosition[0] = 0;

		return result;
	}
	else if (mMode == CLI_MODE_ARGS)
	{
		if (mArgPosition + 1 < mArgc)
		{
			mArgPosition += 1;
			return mArgv[mArgPosition];
		}
		else
		{
			return NULL;
		}
	}

	return NULL;
}

char* cliGetParameterValue(cliParameter* parameters, int parameter_count, char* parameter_name)
{
	int	parameter_index;

	parameter_index = 0;
	while (parameter_index < parameter_count)
	{
		if (strcmp(parameters[parameter_index].name, parameter_name) == 0)
		{
			return strdup(parameters[parameter_index].value);
		}
		parameter_index += 1;
	}

	return NULL;
}

int cliGetParameterGiven(cliParameter* parameters, int parameter_count, char* parameter_name)
{
	int	parameter_index;

	parameter_index = 0;
	while (parameter_index < parameter_count)
	{
		if (strcmp(parameters[parameter_index].name, parameter_name) == 0)
		{
			return parameters[parameter_index].given;
		}
		parameter_index += 1;
	}

	return false;
}

// PRIVATE FUNCTIONS

static void handle_command(const char* command)
{
	int	command_index;

	command_index = 0;
	while (command_index < gBuiltinCommandsCount)
	{
		if (strcmp(gBuiltinCommands[command_index].name, command) == 0)
		{
			break;
		}
		command_index += 1;
	}

	if (command_index < gBuiltinCommandsCount)
	{
		cliParameter*	parameters;
		int	parameter_index;
		char*	parameter_string;

		parameters = calloc(gBuiltinCommands[command_index].parameter_count, sizeof(cliParameter));
		parameter_index = 0;
		while (parameter_index < gBuiltinCommands[command_index].parameter_count)
		{
			memcpy(&(parameters[parameter_index]), &(gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter), sizeof(cliParameter));
			parameter_index += 1;
		}

		while ((parameter_string = cliGetNextSegment()) != NULL)
		{
			if (parameter_string[0] == '-' && parameter_string[1] == '-')
			{
				parameter_string = parameter_string + 2;

				if (strchr(parameter_string, '=') == NULL)
				{
					parameter_index = 0;
					while (parameter_index < gBuiltinCommands[command_index].parameter_count)
					{
						if (strcmp(parameters[parameter_index].name, parameter_string) == 0 && parameters[parameter_index].type == CLI_TYPE_FLAG && parameters[parameter_index].given == false)
						{
							break;
						}
						parameter_index += 1;
					}
					if (parameter_index == gBuiltinCommands[command_index].parameter_count)
					{
						printf("Unknown or duplicated option \"--%s\"\n", parameter_string);
						free(parameters);
						return;
					}
				}
				else
				{
					strchr(parameter_string, '=')[0] = 0;

					parameter_index = 0;
					while (parameter_index < gBuiltinCommands[command_index].parameter_count)
					{
						if (strcmp(parameters[parameter_index].name, parameter_string) == 0 && parameters[parameter_index].type == CLI_TYPE_VALUE && parameters[parameter_index].given == false)
						{
							break;
						}
						parameter_index += 1;
					}
					if (parameter_index == gBuiltinCommands[command_index].parameter_count)
					{
						printf("Unknown or duplicated option \"--%s\"\n", parameter_string);
						parameter_string[strlen(parameter_string)] = '=';
						free(parameters);
						return;
					}

					parameter_string[strlen(parameter_string)] = '=';
					parameter_string = strchr(parameter_string, '=') + 1;
					if (strlen(parameter_string) == 0)
					{
						printf("Missing value for option \"--%s\"\n", parameters[parameter_index].name);
						free(parameters);
						return;
					}
				}
			}
			else
			{
				if (strlen(parameter_string) == 0)
				{
					printf("Empty string found, ignoring\n");
					continue;
				}

				parameter_index = 0;
				while (parameter_index < gBuiltinCommands[command_index].parameter_count)
				{
					if (parameters[parameter_index].type == CLI_TYPE_STRING && parameters[parameter_index].given == false)
					{
						break;
					}
					parameter_index += 1;
				}
				if (parameter_index == gBuiltinCommands[command_index].parameter_count)
				{
					printf("Unknown parameter \"%s\"\n", parameter_string);
					free(parameters);
					return;
				}
			}

			if (strlen(parameter_string) > 255)
			{
				printf("Option too long\n");
				free(parameters);
				return;
			}
			strcpy(parameters[parameter_index].value, parameter_string);
			parameters[parameter_index].given = true;
		}

		parameter_index = 0;
		while (parameter_index < gBuiltinCommands[command_index].parameter_count)
		{
			if (parameters[parameter_index].required == true && parameters[parameter_index].given == false)
			{
				printf("Missing mandatory option \"%s\"\n", parameters[parameter_index].name);
				free(parameters);
				return;
			}
			parameter_index += 1;
		}

		(*(gBuiltinCommands[command_index].handler))(parameters, gBuiltinCommands[command_index].parameter_count);

		free(parameters);

		return;
	}

	command_index = 0;
	while (command_index < gNxtCommandsCount)
	{
		if (strcmp(gNxtCommands[command_index].name, command) == 0)
		{
			break;
		}
		command_index += 1;
	}

	if (command_index < gNxtCommandsCount)
	{
		nxtcommandsDoCommand(command);

		return;
	}

	printf("Unknown command \"%s\"\nUse \"help\" to see available commands\n", command);
}

char* my_completion_generator(const char* text, int state)
{
	static int	command_list;
	static int	command_index;

	if (state == 0)
	{
		command_list = 0;
		command_index = 0;
	}

	if (command_list == 0)
	{
		while (command_index < gBuiltinCommandsCount)
		{
			command_index += 1;

			if (strncmp(gBuiltinCommands[command_index - 1].name, text, strlen(text)) == 0)
			{
				return strdup(gBuiltinCommands[command_index - 1].name);
			}
		}

		command_list = 1;
		command_index = 0;
	}

	if (command_list == 1)
	{
		while (command_index < gNxtCommandsCount)
		{
			command_index += 1;

			if (strncmp(gNxtCommands[command_index - 1].name, text, strlen(text)) == 0)
			{
				return strdup(gNxtCommands[command_index - 1].name);
			}
		}
	}

	return NULL;
}
