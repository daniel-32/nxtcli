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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <libnxtbt/libnxtbt.h>

#include "cli.h"
#include "nxtcommands.h"

typedef struct
{
	char	description[1024];
	cliParameter	cli_parameter;
} builtincommandsParameter;

typedef struct
{
	char	name[256];
	char	description[1024];
	void	(*handler)(cliParameter* cli_parameters, int cli_parameter_count);
	builtincommandsParameter	parameters[8];
	int	parameter_count;
} builtincommandsCommand;

extern int	gDone;

void builtincommandsls(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsget(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsput(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsrm(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandshelp(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsexit(cliParameter* cli_parameters, int cli_parameter_count);

builtincommandsCommand	gBuiltinCommands[] =
{
	{"ls", "Lists all files on the NXT.", builtincommandsls, {
		{"A wildcard to restrict the listing to particular files. The wildcard may take one of the following formats:\n*.*             Matches all files\n*.ext           Matches all files with the extension \"ext\"\nfilename.*      Matches all files with the name \"filename\" and any extension\nfilename.ext    Matches only the file with the exact name \"filename\" and extension \"ext\"", {"wildcard", "*.*", CLI_TYPE_STRING, false, false}},
	}, 1},
	{"get", "Uploads a file from the NXT.", builtincommandsget, {
		{"The name of the file to upload from the NXT.", {"filename", "", CLI_TYPE_STRING, true, false}},
		{"If specified, the filename to use locally, otherwise the name of the file as it appears on the NXT will be used.", {"local filename", "", CLI_TYPE_STRING, false, false}},
	}, 2},
	{"put", "Downloads a file to the NXT.", builtincommandsput, {
		{"The name of the file to download to the NXT.", {"filename", "", CLI_TYPE_STRING, true, false}},
		{"If specified, the filename to use on the NXT, otherwise the name of the file as it appears locally will be used. This name must be a valid NXT filename (at most 15 characters, a dot, and a 3-character extension). (If the local name is not a valid NXT filename, this parameter will need to be used.)", {"remote filename", "", CLI_TYPE_STRING, false, false}},
		{"The mode to use when creating the file on the NXT. May be one of the following values:\nnormal  The file may be fragmented, and no additional data can be added to the file\nlinear  The file will be stored in a contiguous block in the NXT's flash storage (required for program (.rxe) and sound (.rso) files), but no additional data can be added to the file\ndata    The file may be fragmented, but additional data can be added up to the size specified by the \"freespace\" parameter\nappend  Instead of creating a new file, the data is appended to an existing file created using the \"data\" mode (the existing file must have enough free space remaining)", {"mode", "normal", CLI_TYPE_VALUE, false, false}},
		{"The amount of free space to leave at the end of the file when using the \"data\" mode.", {"freespace", "0", CLI_TYPE_VALUE, false, false}},
	}, 4},
	{"rm", "Deletes a file on the NXT.", builtincommandsrm, {
		{"", {"filename", "", CLI_TYPE_STRING, true, false}},
	}, 1},
	{"help", "Lists available commands, or gets additional help on a specific command.", builtincommandshelp, {
		{"If specified, requests help on a specific command, otherwise all commands are listed.", {"command", "", CLI_TYPE_STRING, false, false}},
	}, 1},
	{"exit", "Exits the program.", builtincommandsexit, {
	}, 0},
};
int	gBuiltinCommandsCount = 6;

static void print_wrap(const char* prefix, const char* text);

#define print_result_code(result_code, message, cleanup)\
if (result_code < 0)\
{\
	char*	error_string;\
	error_string = nxtLibErrorString(result_code);\
	printf("libnxt error while %s: %s\n", #message, error_string);\
	free(error_string);\
	cleanup\
	return;\
}

#define print_status(status, message, cleanup)\
if (status != NXT_STS_SUCCESS)\
{\
	char*	status_string;\
	status_string = nxtStatusString(status);\
	printf("NXT returned error while %s: %s\n", #message, status_string);\
	free(status_string);\
	cleanup\
	return;\
}

void builtincommandsls(cliParameter* cli_parameters, int cli_parameter_count)
{
	char*	wildcard;

	nxtParameter	nxt_parameters[1];
	nxtResponse	nxt_responses[4];
	int	result_code;
	uint8_t	nxt_status;
	uint8_t	nxt_handle;

	wildcard = cliGetParameterValue(cli_parameters, cli_parameter_count, "wildcard");

	nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
	nxt_responses[1].type = NXT_TYPE_UBYTE;	// handle
	nxt_responses[2].type = NXT_TYPE_FILENAME;	// filename
	nxt_responses[3].type = NXT_TYPE_ULONG;	// file size

	nxt_parameters[0].type = NXT_TYPE_FILENAME;
	nxt_parameters[0].value.filename = wildcard;
	result_code = nxtDoCommand(NXT_CMD_FINDFIRST, nxt_parameters, nxt_responses, 1, 4);
	print_result_code(result_code, finding first file, free(wildcard);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, finding first file, free(wildcard); free(nxt_responses[2].value.filename);)
	nxt_handle = nxt_responses[1].value.ubyte;

	while (nxt_status != NXT_STS_FILE_NOT_FOUND)
	{
		printf("%19s %d\n", nxt_responses[2].value.filename, nxt_responses[3].value.ulong);
		free(nxt_responses[2].value.filename);

		nxt_parameters[0].type = NXT_TYPE_UBYTE;
		nxt_parameters[0].value.ubyte = nxt_handle;
		result_code = nxtDoCommand(NXT_CMD_FINDNEXT, nxt_parameters, nxt_responses, 1, 4);
		print_result_code(result_code, finding next file, free(wildcard);)
		nxt_status = nxt_responses[0].value.ubyte;
		if (nxt_status != NXT_STS_FILE_NOT_FOUND)
		{
			print_status(nxt_status, finding next file, free(wildcard); free(nxt_responses[2].value.filename);)
		}
		nxt_handle = nxt_responses[1].value.ubyte;
	}
	free(nxt_responses[2].value.filename);

	nxt_parameters[0].type = NXT_TYPE_UBYTE;
	nxt_parameters[0].value.ubyte = nxt_handle;
	result_code = nxtDoCommand(NXT_CMD_CLOSE, nxt_parameters, nxt_responses, 1, 2);
	print_result_code(result_code, closing handle, free(wildcard);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, closing handle, free(wildcard);)

	free(wildcard);
}

void builtincommandsget(cliParameter* cli_parameters, int cli_parameter_count)
{
	char*	local_filename;
	int	local_handle;

	char*	nxt_filename;
	nxtParameter	nxt_parameters[2];
	nxtResponse	nxt_responses[4];
	int	result_code;
	uint8_t	nxt_status;
	uint8_t	nxt_handle;
	uint32_t	nxt_file_size;
	uint16_t	nxt_bytes_read;

	nxt_filename = cliGetParameterValue(cli_parameters, cli_parameter_count, "filename");
	if (cliGetParameterGiven(cli_parameters, cli_parameter_count, "local filename") == true)
	{
		local_filename = cliGetParameterValue(cli_parameters, cli_parameter_count, "local filename");
	}
	else
	{
		local_filename = strdup(nxt_filename);
	}

	nxt_parameters[0].type = NXT_TYPE_FILENAME;
	nxt_parameters[0].value.filename = nxt_filename;
	nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
	nxt_responses[1].type = NXT_TYPE_UBYTE;	// handle
	nxt_responses[2].type = NXT_TYPE_ULONG;	// file size
	result_code = nxtDoCommand(NXT_CMD_OPENREAD, nxt_parameters, nxt_responses, 1, 3);
	print_result_code(result_code, opening file, free(local_filename); free(nxt_filename);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, opening file, free(local_filename); free(nxt_filename);)
	nxt_handle = nxt_responses[1].value.ubyte;
	nxt_file_size = nxt_responses[2].value.ulong;

	local_handle = open(local_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (local_handle == -1)
	{
		printf("Error opening local file \"%s\"\n", local_filename);
		free(local_filename);
		free(nxt_filename);
		return;
	}

	free(local_filename);
	free(nxt_filename);

	while (nxt_file_size > 0)
	{
		nxt_parameters[0].type = NXT_TYPE_UBYTE;
		nxt_parameters[0].value.ubyte = nxt_handle;
		nxt_parameters[1].type = NXT_TYPE_UWORD;
		if (nxt_file_size > 32)
		{
			nxt_parameters[1].value.uword = 32;
		}
		else
		{
			nxt_parameters[1].value.uword = nxt_file_size;
		}
		nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
		nxt_responses[1].type = NXT_TYPE_UBYTE;	// handle
		nxt_responses[2].type = NXT_TYPE_UWORD;	// bytes read
		nxt_responses[3].type = NXT_TYPE_BYTES;	// data
		nxt_responses[3].length = -1;
		result_code = nxtDoCommand(NXT_CMD_READ, nxt_parameters, nxt_responses, 2, 4);
		print_result_code(result_code, reading file, close(local_handle);)
		nxt_status = nxt_responses[0].value.ubyte;
		print_status(nxt_status, reading file, free(nxt_responses[3].value.bytes); close(local_handle);)
		nxt_handle = nxt_responses[1].value.ubyte;
		nxt_bytes_read = nxt_responses[2].value.uword;
		nxt_file_size = nxt_file_size - nxt_bytes_read;
		write(local_handle, nxt_responses[3].value.bytes, nxt_bytes_read);
		free(nxt_responses[3].value.bytes);
	}

	nxt_parameters[0].type = NXT_TYPE_UBYTE;
	nxt_parameters[0].value.ubyte = nxt_handle;
	result_code = nxtDoCommand(NXT_CMD_CLOSE, nxt_parameters, nxt_responses, 1, 2);
	print_result_code(result_code, closing handle, close(local_handle);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, closing handle, close(local_handle);)

	close(local_handle);
}

void builtincommandsput(cliParameter* cli_parameters, int cli_parameter_count)
{
	char*	local_filename;
	int	local_handle;
	uint32_t	local_file_size;

	char*	nxt_filename;
	char*	nxt_open_mode;
	nxtCommand	nxt_open_command;
	char*	nxt_open_freespace_string;
	uint32_t	nxt_open_freespace;
	nxtParameter	nxt_parameters[2];
	nxtResponse	nxt_responses[3];
	int	result_code;
	uint8_t	nxt_status;
	uint8_t	nxt_handle;
	uint16_t	nxt_bytes_written;

	uint8_t	buffer[32];

	local_filename = cliGetParameterValue(cli_parameters, cli_parameter_count, "filename");
	if (cliGetParameterGiven(cli_parameters, cli_parameter_count, "remote filename") == true)
	{
		nxt_filename = cliGetParameterValue(cli_parameters, cli_parameter_count, "remote filename");
	}
	else
	{
		nxt_filename = strdup(local_filename);
	}

	nxt_open_mode = cliGetParameterValue(cli_parameters, cli_parameter_count, "mode");
	if (strcmp(nxt_open_mode, "normal") == 0)
	{
		nxt_open_command = NXT_CMD_OPENWRITE;
		nxt_open_freespace = 0;
	}
	else if (strcmp(nxt_open_mode, "linear") == 0)
	{
		nxt_open_command = NXT_CMD_OPENWRITELINEAR;
		nxt_open_freespace = 0;
	}
	else if (strcmp(nxt_open_mode, "data") == 0)
	{
		long long	open_freespace;

		nxt_open_command = NXT_CMD_OPENWRITEDATA;
		nxt_open_freespace_string = cliGetParameterValue(cli_parameters, cli_parameter_count, "freespace");
		open_freespace = atoll(nxt_open_freespace_string);
		free(nxt_open_freespace_string);
		if (open_freespace < 0 || open_freespace > NXT_ULONG_MAX)
		{
			printf("Value out of range\n");
			free(local_filename);
			free(nxt_filename);
			return;
		}
		nxt_open_freespace = open_freespace;
	}
	else if (strcmp(nxt_open_mode, "append") == 0)
	{
		nxt_open_command = NXT_CMD_OPENAPPENDDATA;
		nxt_open_freespace = 0;
	}
	else
	{
		printf("Unknown mode \"%s\"\n", nxt_open_mode);
		free(local_filename);
		free(nxt_filename);
		free(nxt_open_mode);
		return;
	}
	free(nxt_open_mode);

	local_handle = open(local_filename, O_RDONLY);
	if (local_handle == -1)
	{
		printf("Error opening local file \"%s\"\n", local_filename);
		free(local_filename);
		free(nxt_filename);
		return;
	}
	local_file_size = lseek(local_handle, 0, SEEK_END);
	lseek(local_handle, 0, SEEK_SET);

	nxt_parameters[0].type = NXT_TYPE_FILENAME;
	nxt_parameters[0].value.filename = nxt_filename;
	nxt_parameters[1].type = NXT_TYPE_ULONG;
	nxt_parameters[1].value.ulong = local_file_size + nxt_open_freespace;
	nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
	nxt_responses[1].type = NXT_TYPE_UBYTE;	// handle
	nxt_responses[2].type = NXT_TYPE_ULONG;	// available file size (only used if nxt_open_command == NXT_CMD_OPENAPPENDDATA)
	result_code = nxtDoCommand(nxt_open_command, nxt_parameters, nxt_responses, nxt_open_command == NXT_CMD_OPENAPPENDDATA ? 1 : 2, nxt_open_command == NXT_CMD_OPENAPPENDDATA ? 3 : 2);
	print_result_code(result_code, opening file, close(local_handle); free(local_filename); free(nxt_filename);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, opening file, close(local_handle); free(local_filename); free(nxt_filename);)
	nxt_handle = nxt_responses[1].value.ubyte;

	// check if there's enough free space if we're appending to a file
	if (nxt_open_command == NXT_CMD_OPENAPPENDDATA)
	{
		if (nxt_responses[2].value.ulong < local_file_size)
		{
			printf("Not enough free space in \"%s\"\n", nxt_filename);
			free(local_filename);
			free(nxt_filename);
			nxt_parameters[0].type = NXT_TYPE_UBYTE;
			nxt_parameters[0].value.ubyte = nxt_handle;
			result_code = nxtDoCommand(NXT_CMD_CLOSE, nxt_parameters, nxt_responses, 1, 2);
			print_result_code(result_code, closing handle, close(local_handle);)
			nxt_status = nxt_responses[0].value.ubyte;
			print_status(nxt_status, closing handle, close(local_handle);)
			close(local_handle);
			return;
		}
	}

	free(local_filename);
	free(nxt_filename);

	while (local_file_size > 0)
	{
		nxt_parameters[0].type = NXT_TYPE_UBYTE;
		nxt_parameters[0].value.ubyte = nxt_handle;
		nxt_parameters[1].type = NXT_TYPE_BYTES;
		if (local_file_size > 32)
		{
			read(local_handle, buffer, 32);
			nxt_parameters[1].length = 32;
			nxt_parameters[1].value.bytes = buffer;
		}
		else
		{
			read(local_handle, buffer, local_file_size);
			nxt_parameters[1].length = local_file_size;
			nxt_parameters[1].value.bytes = buffer;
		}
		nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
		nxt_responses[1].type = NXT_TYPE_UBYTE;	// handle
		nxt_responses[2].type = NXT_TYPE_UWORD;	// bytes written
		result_code = nxtDoCommand(NXT_CMD_WRITE, nxt_parameters, nxt_responses, 2, 3);
		print_result_code(result_code, writing file, close(local_handle);)
		nxt_status = nxt_responses[0].value.ubyte;
		print_status(nxt_status, writing file, close(local_handle);)
		nxt_handle = nxt_responses[1].value.ubyte;
		nxt_bytes_written = nxt_responses[2].value.uword;
		if (local_file_size > 32)
		{
			if (nxt_bytes_written != 32)
			{
				printf("NXT error while writing file\n");
				close(local_handle);
				return;
			}
		}
		else
		{
			if (nxt_bytes_written != local_file_size)
			{
				printf("NXT error while writing file\n");
				close(local_handle);
				return;
			}
		}
		local_file_size = local_file_size - nxt_bytes_written;
	}

	nxt_parameters[0].type = NXT_TYPE_UBYTE;
	nxt_parameters[0].value.ubyte = nxt_handle;
	result_code = nxtDoCommand(NXT_CMD_CLOSE, nxt_parameters, nxt_responses, 1, 2);
	print_result_code(result_code, closing handle, close(local_handle);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, closing handle, close(local_handle);)

	close(local_handle);
}

void builtincommandsrm(cliParameter* cli_parameters, int cli_parameter_count)
{
	char*	nxt_filename;
	nxtParameter	nxt_parameters[1];
	nxtResponse	nxt_responses[2];
	int	result_code;
	uint8_t	nxt_status;

	nxt_filename = cliGetParameterValue(cli_parameters, cli_parameter_count, "filename");

	nxt_parameters[0].type = NXT_TYPE_FILENAME;
	nxt_parameters[0].value.filename = nxt_filename;
	nxt_responses[0].type = NXT_TYPE_UBYTE;	// status
	nxt_responses[1].type = NXT_TYPE_FILENAME;	// filename
	result_code = nxtDoCommand(NXT_CMD_DELETE, nxt_parameters, nxt_responses, 1, 2);
	print_result_code(result_code, Deleting file, free(nxt_responses[1].value.filename); free(nxt_filename);)
	nxt_status = nxt_responses[0].value.ubyte;
	print_status(nxt_status, Deleting file, free(nxt_responses[1].value.filename); free(nxt_filename);)
	free(nxt_responses[1].value.filename);

	free(nxt_filename);
}

void builtincommandshelp(cliParameter* cli_parameters, int cli_parameter_count)
{
	if (cliGetParameterGiven(cli_parameters, cli_parameter_count, "command") == true)
	{
		char*	command;
		int	command_list;
		int	command_index;
		char*	name;
		char*	description;

		command = cliGetParameterValue(cli_parameters, cli_parameter_count, "command");
		command_index = 0;
		while (command_index < gBuiltinCommandsCount)
		{
			if (strcmp(gBuiltinCommands[command_index].name, command) == 0)
			{
				name = gBuiltinCommands[command_index].name;
				description = gBuiltinCommands[command_index].description;
				command_list = 0;
				break;
			}
			command_index += 1;
		}
		if (command_index == gBuiltinCommandsCount)
		{
			command_index = 0;
			while (command_index < gNxtCommandsCount)
			{
				if (strcmp(gNxtCommands[command_index].name, command) == 0)
				{
					name = gNxtCommands[command_index].name;
					description = gNxtCommands[command_index].description;
					command_list = 1;
					break;
				}
				command_index += 1;
			}
			if (command_index == gNxtCommandsCount)
			{
				printf("Unknown command \"%s\"\nUse \"help\" to see available commands\n", command);
				free(command);
				return;
			}
		}
		free(command);

		printf("%s - ", name);
		switch (command_list)
		{
			case 0:
				printf("built-in command\n");
				break;
			case 1:
				printf("NXT command\n");
				break;
		}

		if (description != NULL)
		{
			if (strcmp(description, "") != 0)
			{
				print_wrap("  ", description);
			}
		}

		if ((command_list == 0 && gBuiltinCommands[command_index].parameter_count > 0) || (command_list == 1 && gNxtCommands[command_index].parameter_count > 0))
		{
			int	parameter_count;
			int	parameter_index;

			printf("\nParameters:\n");

			if (command_list == 0)
			{
				parameter_count = gBuiltinCommands[command_index].parameter_count;
			}
			else
			{
				parameter_count = gNxtCommands[command_index].parameter_count;
			}

			parameter_index = 0;
			while (parameter_index < parameter_count)
			{
				if (command_list == 0)
				{
					name = gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter.name;
					description = gBuiltinCommands[command_index].parameters[parameter_index].description;
				}
				else
				{
					name = gNxtCommands[command_index].parameters[parameter_index].name;
					description = gNxtCommands[command_index].parameters[parameter_index].description;
				}
				if (command_list == 0)
				{
					switch (gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter.type)
					{
						case CLI_TYPE_STRING:
							printf("- %s (", name);
							break;
						case CLI_TYPE_FLAG:
							printf("- --%s (", name);
							break;
						case CLI_TYPE_VALUE:
							printf("- --%s=VALUE (", name);
							break;
					}
					if (gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter.required == true)
					{
						printf("required");
					}
					else
					{
						printf("optional");
						if (strcmp(gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter.value, "") != 0)
						{
							printf(", default value: \"%s\"", gBuiltinCommands[command_index].parameters[parameter_index].cli_parameter.value);
						}
					}
					printf(")");
				}
				else
				{
					printf("- %s (", name);
					switch (gNxtCommands[command_index].parameters[parameter_index].type)
					{
						case NXT_TYPE_BOOLEAN:
							printf("boolean");
							break;
						case NXT_TYPE_UBYTE:
							printf("ubyte");
							break;
						case NXT_TYPE_SBYTE:
							printf("sbyte");
							break;
						case NXT_TYPE_UWORD:
							printf("uword");
							break;
						case NXT_TYPE_SWORD:
							printf("sword");
							break;
						case NXT_TYPE_ULONG:
							printf("ulong");
							break;
						case NXT_TYPE_SLONG:
							printf("slong");
							break;
						case NXT_TYPE_BYTES:
							printf("bytes");
							break;
						case NXT_TYPE_STRING:
							printf("string");
							break;
						case NXT_TYPE_FILENAME:
							printf("filename");
							break;
					}
					printf(")");
				}
				printf("\n");

				if (description != NULL)
				{
					if (strcmp(description, "") != 0)
					{
						print_wrap("    ", description);
					}
				}

				parameter_index += 1;
			}
		}

		if (command_list == 1 && gNxtCommands[command_index].response_count > 0)
		{
			int	response_count;
			int	response_index;

			printf("\nResponses:\n");

			response_count = gNxtCommands[command_index].response_count;

			response_index = 0;
			while (response_index < response_count)
			{
				name = gNxtCommands[command_index].responses[response_index].name;
				description = gNxtCommands[command_index].responses[response_index].description;
				printf("- %s (", name);
				switch (gNxtCommands[command_index].responses[response_index].type)
				{
					case NXT_TYPE_BOOLEAN:
						printf("boolean");
						break;
					case NXT_TYPE_UBYTE:
						printf("ubyte");
						break;
					case NXT_TYPE_SBYTE:
						printf("sbyte");
						break;
					case NXT_TYPE_UWORD:
						printf("uword");
						break;
					case NXT_TYPE_SWORD:
						printf("sword");
						break;
					case NXT_TYPE_ULONG:
						printf("ulong");
						break;
					case NXT_TYPE_SLONG:
						printf("slong");
						break;
					case NXT_TYPE_BYTES:
						printf("bytes");
						break;
					case NXT_TYPE_STRING:
						printf("string");
						break;
					case NXT_TYPE_FILENAME:
						printf("filename");
						break;
				}
				printf(")\n");

				if (description != NULL)
				{
					if (strcmp(description, "") != 0)
					{
						print_wrap("    ", description);
					}
				}

				response_index += 1;
			}
		}
	}
	else
	{
		int	command_index;

		printf("Built-in commands:\n");
		command_index = 0;
		while (command_index < gBuiltinCommandsCount)
		{
			printf("  %s\n", gBuiltinCommands[command_index].name);
			command_index += 1;
		}

		printf("\nNXT commands:\n");
		command_index = 0;
		while (command_index < gNxtCommandsCount)
		{
			printf("  %s\n", gNxtCommands[command_index].name);
			command_index += 1;
		}

		printf("\nUse \"help <command>\" for more information about a specific command.\n");
	}
}

void builtincommandsexit(cliParameter* cli_parameters, int cli_parameter_count)
{
	gDone = true;
}

// PRIVATE FUNCTIONS

static void print_wrap(const char* prefix, const char* text)
{
	char*	text_copy;
	char*	text_block;
	char*	text_marker;

	text_copy = strdup(text);
	text_block = text_copy;

	text_marker = strsep(&text_block, "\n");

	while (text_marker != NULL)
	{
		if (isatty(STDOUT_FILENO) == true)
		{
			char	temp_char;
			struct winsize	terminal_size;
			int	wrap_length;
			int	current_position;

			ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
			wrap_length = terminal_size.ws_col - strlen(prefix);

			while (text_marker[0] == ' ')
			{
				text_marker += 1;
			}

			while (strlen(text_marker) > wrap_length)
			{
				temp_char = text_marker[wrap_length + 1];
				text_marker[wrap_length + 1] = 0;

				if (strrchr(text_marker, ' ') != NULL)
				{
					strrchr(text_marker, ' ')[0] = 0;
					text_marker[wrap_length + 1] = temp_char;
					temp_char = ' ';
				}
				else
				{
					text_marker[wrap_length + 1] = temp_char;
					temp_char = text_marker[wrap_length];
					text_marker[wrap_length] = 0;
				}

				printf("%s%s\n", prefix, text_marker);

				text_marker += strlen(text_marker);
				text_marker[0] = temp_char;
				while (text_marker[0] == ' ')
				{
					text_marker += 1;
				}
			}
			printf("%s%s\n", prefix, text_marker);
		}
		else
		{
			printf("%s%s\n", prefix, text_marker);
		}

		text_marker = strsep(&text_block, "\n");
	}

	free(text_copy);
}
