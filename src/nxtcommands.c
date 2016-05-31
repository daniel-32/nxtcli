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

#include <libxml/parser.h>

#include <libnxtbt/libnxtbt.h>

#include "cli.h"

typedef struct
{
	nxtType	type;
	char	name[256];
	char	description[1024];
	int	length;
} nxtcommandsParameter;

typedef struct
{
	nxtType	type;
	char	name[256];
	char	description[1024];
	int	length;
} nxtcommandsResponse;

typedef struct
{
	nxtCommand	command;
	char	name[256];
	char	description[1024];
	int	parameter_count;
	int	response_count;
	nxtcommandsParameter	parameters[16];
	nxtcommandsResponse	responses[16];
} nxtcommandsCommand;

nxtcommandsCommand	gNxtCommands[256] = {};
int	gNxtCommandsCount = 0;

static xmlNodePtr xml_get_child_by_name(xmlNodePtr parent, const char* name);

static int add_boolean(nxtParameter* parameter);
static int add_ubyte(nxtParameter* parameter);
static int add_sbyte(nxtParameter* parameter);
static int add_uword(nxtParameter* parameter);
static int add_sword(nxtParameter* parameter);
static int add_ulong(nxtParameter* parameter);
static int add_slong(nxtParameter* parameter);
static int add_bytes(nxtParameter* parameter);
static int add_string(nxtParameter* parameter);
static int add_filename(nxtParameter* parameter);

static char* get_boolean(nxtResponse response);
static char* get_ubyte(nxtResponse response);
static char* get_sbyte(nxtResponse response);
static char* get_uword(nxtResponse response);
static char* get_sword(nxtResponse response);
static char* get_ulong(nxtResponse response);
static char* get_slong(nxtResponse response);
static char* get_bytes(nxtResponse response);
static char* get_string(nxtResponse response);
static char* get_filename(nxtResponse response);

static char* hexdump(uint8_t* bytes, int length);

void nxtcommandsLoadCommandsFromXML(const char* filename)
{
	#define xml_error()\
	gNxtCommandsCount = 0;\
	xmlFreeDoc(xml_file);\
	return;

	xmlDocPtr	xml_file;
	xmlNodePtr	current_command;
	xmlNodePtr	current_parameter;
	xmlNodePtr	current_response;
	xmlChar*	value;
	xmlChar*	type;
	xmlChar*	name;
	xmlChar*	description;
	xmlChar*	length;

	xml_file = xmlParseFile(filename);

	if (xmlStrcmp(xmlDocGetRootElement(xml_file)->name, "nxtcommands") != 0)
	{
		xml_error()
	}

	gNxtCommandsCount = 0;
	current_command = xmlDocGetRootElement(xml_file)->xmlChildrenNode;
	while (current_command != NULL)
	{
		if (xmlStrcmp(current_command->name, "command") != 0)
		{
			current_command = current_command->next;
			continue;
		}

		// set command byte
		value = xmlGetProp(current_command, "value");
		if (value != NULL)
		{
			gNxtCommands[gNxtCommandsCount].command = (uint8_t) strtoull(value, NULL, 16);
			xmlFree(value);
		}
		else
		{
			xml_error()
		}

		// set name
		name = xmlGetProp(current_command, "name");
		if (name != NULL)
		{
			strcpy(gNxtCommands[gNxtCommandsCount].name, name);
			xmlFree(name);
		}
		else
		{
			xml_error()
		}

		// set description
		strcpy(gNxtCommands[gNxtCommandsCount].description, "");
		if (xml_get_child_by_name(current_command, "description") != NULL)
		{
			if (xml_get_child_by_name(current_command, "description")->xmlChildrenNode != NULL)
			{
				description = xmlNodeListGetString(xml_file, xml_get_child_by_name(current_command, "description")->xmlChildrenNode, 1);
				if (description != NULL)
				{
					strcpy(gNxtCommands[gNxtCommandsCount].description, description);
					xmlFree(description);
				}
			}
		}

		// read parameters
		gNxtCommands[gNxtCommandsCount].parameter_count = 0;
		if (xml_get_child_by_name(current_command, "parameters") != NULL)
		{
			current_parameter = xml_get_child_by_name(current_command, "parameters")->xmlChildrenNode;
			while (current_parameter != NULL)
			{
				if (xmlStrcmp(current_parameter->name, "parameter") != 0)
				{
					current_parameter = current_parameter->next;
					continue;
				}

				// set type
				type = xmlGetProp(current_parameter, "type");
				if (type != NULL)
				{
					if (xmlStrcmp(type, "boolean") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_BOOLEAN;
					}
					else if (xmlStrcmp(type, "ubyte") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_UBYTE;
					}
					else if (xmlStrcmp(type, "sbyte") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_SBYTE;
					}
					else if (xmlStrcmp(type, "uword") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_UWORD;
					}
					else if (xmlStrcmp(type, "sword") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_SWORD;
					}
					else if (xmlStrcmp(type, "ulong") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_ULONG;
					}
					else if (xmlStrcmp(type, "slong") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_SLONG;
					}
					else if (xmlStrcmp(type, "bytes") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_BYTES;
					}
					else if (xmlStrcmp(type, "string") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_STRING;
					}
					else if (xmlStrcmp(type, "filename") == 0)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type = NXT_TYPE_FILENAME;
					}
					else
					{
						xmlFree(type);
						xml_error()
					}
					xmlFree(type);
				}
				else
				{
					xml_error()
				}

				// set name
				name = xmlGetProp(current_parameter, "name");
				if (name != NULL)
				{
					strcpy(gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].name, name);
					xmlFree(name);
				}
				else
				{
					xml_error()
				}

				// set description
				strcpy(gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].description, "");
				if (xml_get_child_by_name(current_parameter, "description") != NULL)
				{
					if (xml_get_child_by_name(current_parameter, "description")->xmlChildrenNode != NULL)
					{
						description = xmlNodeListGetString(xml_file, xml_get_child_by_name(current_parameter, "description")->xmlChildrenNode, 1);
						if (description != NULL)
						{
							strcpy(gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].description, description);
							xmlFree(description);
						}
					}
				}

				// set length
				if (gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type == NXT_TYPE_BYTES || gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].type == NXT_TYPE_STRING)
				{
					length = xmlGetProp(current_parameter, "length");
					if (length != NULL)
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].length = (int) strtoull(length, NULL, 10);
						xmlFree(length);
					}
					else
					{
						gNxtCommands[gNxtCommandsCount].parameters[gNxtCommands[gNxtCommandsCount].parameter_count].length = -1;
					}
				}

				gNxtCommands[gNxtCommandsCount].parameter_count += 1;
				current_parameter = current_parameter->next;
			}
		}

		// read responses
		gNxtCommands[gNxtCommandsCount].response_count = 0;
		if (xml_get_child_by_name(current_command, "responses") != NULL)
		{
			current_response = xml_get_child_by_name(current_command, "responses")->xmlChildrenNode;
			while (current_response != NULL)
			{
				if (xmlStrcmp(current_response->name, "response") != 0)
				{
					current_response = current_response->next;
					continue;
				}

				// set type
				type = xmlGetProp(current_response, "type");
				if (type != NULL)
				{
					if (xmlStrcmp(type, "boolean") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_BOOLEAN;
					}
					else if (xmlStrcmp(type, "ubyte") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_UBYTE;
					}
					else if (xmlStrcmp(type, "sbyte") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_SBYTE;
					}
					else if (xmlStrcmp(type, "uword") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_UWORD;
					}
					else if (xmlStrcmp(type, "sword") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_SWORD;
					}
					else if (xmlStrcmp(type, "ulong") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_ULONG;
					}
					else if (xmlStrcmp(type, "slong") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_SLONG;
					}
					else if (xmlStrcmp(type, "bytes") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_BYTES;
					}
					else if (xmlStrcmp(type, "string") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_STRING;
					}
					else if (xmlStrcmp(type, "filename") == 0)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type = NXT_TYPE_FILENAME;
					}
					else
					{
						xmlFree(type);
						xml_error()
					}
					xmlFree(type);
				}
				else
				{
					xml_error()
				}

				// set name
				name = xmlGetProp(current_response, "name");
				if (name != NULL)
				{
					strcpy(gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].name, name);
					xmlFree(name);
				}
				else
				{
					xml_error()
				}

				// set description
				strcpy(gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].description, "");
				if (xml_get_child_by_name(current_response, "description") != NULL)
				{
					if (xml_get_child_by_name(current_response, "description")->xmlChildrenNode != NULL)
					{
						description = xmlNodeListGetString(xml_file, xml_get_child_by_name(current_response, "description")->xmlChildrenNode, 1);
						if (description != NULL)
						{
							strcpy(gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].description, description);
							xmlFree(description);
						}
					}
				}

				// set length
				if (gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type == NXT_TYPE_BYTES || gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].type == NXT_TYPE_STRING)
				{
					length = xmlGetProp(current_response, "length");
					if (length != NULL)
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].length = (int) strtoull(length, NULL, 10);
						xmlFree(length);
					}
					else
					{
						gNxtCommands[gNxtCommandsCount].responses[gNxtCommands[gNxtCommandsCount].response_count].length = -1;
					}
				}

				gNxtCommands[gNxtCommandsCount].response_count += 1;
				current_response = current_response->next;
			}
		}

		gNxtCommandsCount += 1;
		current_command = current_command->next;
	}

	xmlFreeDoc(xml_file);
}

void nxtcommandsDoCommand(const char* command)
{
	int	command_index;
	int	parameter_index;
	int	response_index;

	nxtParameter	(*parameters)[];
	nxtResponse	(*responses)[];

	int	result_code;

	command_index = 0;
	while (command_index < gNxtCommandsCount)
	{
		if (strcmp(gNxtCommands[command_index].name, command) == 0)
		{
			break;
		}

		command_index += 1;
	}

	if (command_index == gNxtCommandsCount)
	{
		printf("NXT command \"%s\" not found\n", command);
		return;
	}

	parameters = calloc(gNxtCommands[command_index].parameter_count, sizeof(nxtParameter));
	responses = calloc(gNxtCommands[command_index].response_count, sizeof(nxtResponse));

	parameter_index = 0;
	while (parameter_index < gNxtCommands[command_index].parameter_count)
	{
		#define add_parameter(parameter_type)\
		if (add_ ## parameter_type(&((*parameters)[parameter_index])) == false)\
		{\
			printf("Error in parameter %d (%s): invalid or missing value (expected " #parameter_type ")\n", parameter_index, gNxtCommands[command_index].parameters[parameter_index].name);\
			return;\
		}

		(*parameters)[parameter_index].type = gNxtCommands[command_index].parameters[parameter_index].type;

		switch (gNxtCommands[command_index].parameters[parameter_index].type)
		{
			case NXT_TYPE_BOOLEAN:
				add_parameter(boolean)
				break;
			case NXT_TYPE_UBYTE:
				add_parameter(ubyte)
				break;
			case NXT_TYPE_SBYTE:
				add_parameter(sbyte)
				break;
			case NXT_TYPE_UWORD:
				add_parameter(uword)
				break;
			case NXT_TYPE_SWORD:
				add_parameter(sword)
				break;
			case NXT_TYPE_ULONG:
				add_parameter(ulong)
				break;
			case NXT_TYPE_SLONG:
				add_parameter(slong)
				break;
			case NXT_TYPE_BYTES:
				(*parameters)[parameter_index].length = gNxtCommands[command_index].parameters[parameter_index].length;
				add_parameter(bytes)
				break;
			case NXT_TYPE_STRING:
				(*parameters)[parameter_index].length = gNxtCommands[command_index].parameters[parameter_index].length;
				add_parameter(string)
				break;
			case NXT_TYPE_FILENAME:
				add_parameter(filename)
				break;
		}

		parameter_index += 1;
	}

	response_index = 0;
	while (response_index < gNxtCommands[command_index].response_count)
	{
		(*responses)[response_index].type = gNxtCommands[command_index].responses[response_index].type;
		if (gNxtCommands[command_index].responses[response_index].type == NXT_TYPE_BYTES || gNxtCommands[command_index].responses[response_index].type == NXT_TYPE_STRING)
		{
			(*responses)[response_index].length = gNxtCommands[command_index].responses[response_index].length;
		}

		response_index += 1;
	}

	result_code = nxtDoCommand(gNxtCommands[command_index].command, (*parameters), (*responses), gNxtCommands[command_index].parameter_count, gNxtCommands[command_index].response_count);

	parameter_index = 0;
	while (parameter_index < gNxtCommands[command_index].parameter_count)
	{
		switch (gNxtCommands[command_index].parameters[parameter_index].type)
		{
			case NXT_TYPE_BYTES:
				if ((*parameters)[parameter_index].value.bytes != NULL)
				{
					free((*parameters)[parameter_index].value.bytes);
				}
				break;
			case NXT_TYPE_STRING:
				if ((*parameters)[parameter_index].value.string != NULL)
				{
					free((*parameters)[parameter_index].value.string);
				}
				break;
			case NXT_TYPE_FILENAME:
				if ((*parameters)[parameter_index].value.filename != NULL)
				{
					free((*parameters)[parameter_index].value.filename);
				}
				break;
		}

		parameter_index += 1;
	}

	if (result_code >= 0)
	{
		response_index = 0;
		while (response_index < result_code && response_index < gNxtCommands[command_index].response_count)
		{
			#define get_response(response_type)\
			printf("%s (" #response_type "): ", gNxtCommands[command_index].responses[response_index].name);\
			{\
			char*	response_string;\
			response_string = get_ ## response_type((*responses)[response_index]);\
			if (response_string != NULL)\
			{\
				if (response_index == 0 && strcmp(gNxtCommands[command_index].responses[response_index].name, "Status") == 0 && strcmp(#response_type, "ubyte") == 0)\
				{\
					char*	status_string;\
					status_string = nxtStatusString(atoi(response_string));\
					printf("%s", status_string);\
					free(status_string);\
				}\
				else\
				{\
					printf("%s", response_string);\
				}\
				free(response_string);\
			}\
			else\
			{\
				printf("error interpreting response value");\
			}\
			}\
			printf("\n");

			switch (gNxtCommands[command_index].responses[response_index].type)
			{
				case NXT_TYPE_BOOLEAN:
					get_response(boolean)
					break;
				case NXT_TYPE_UBYTE:
					get_response(ubyte)
					break;
				case NXT_TYPE_SBYTE:
					get_response(sbyte)
					break;
				case NXT_TYPE_UWORD:
					get_response(uword)
					break;
				case NXT_TYPE_SWORD:
					get_response(sword)
					break;
				case NXT_TYPE_ULONG:
					get_response(ulong)
					break;
				case NXT_TYPE_SLONG:
					get_response(slong)
					break;
				case NXT_TYPE_BYTES:
					get_response(bytes)
					if ((*responses)[response_index].value.bytes != NULL)
					{
						free((*responses)[response_index].value.bytes);
					}
					break;
				case NXT_TYPE_STRING:
					get_response(string)
					if ((*responses)[response_index].value.string != NULL)
					{
						free((*responses)[response_index].value.string);
					}
					break;
				case NXT_TYPE_FILENAME:
					get_response(filename)
					if ((*responses)[response_index].value.filename != NULL)
					{
						free((*responses)[response_index].value.filename);
					}
					break;
			}

			response_index += 1;
		}

		if (result_code != gNxtCommands[command_index].response_count)
		{
			printf("Expected %d extra responses\n", gNxtCommands[command_index].response_count - result_code);
		}
	}
	else
	{
		char*	error_string;

		error_string = nxtLibErrorString(result_code);
		printf("libnxt error: %s\n", error_string);
		free(error_string);
	}
}

// PRIVATE FUNCTIONS

static xmlNodePtr xml_get_child_by_name(xmlNodePtr parent, const char* name)
{
	xmlNodePtr	child;

	if (parent != NULL)
	{
		child = parent->xmlChildrenNode;
		while (child != NULL)
		{
			if (xmlStrcmp(child->name, name) == 0)
			{
				break;
			}

			child = child->next;
		}

		return child;
	}

	return NULL;
}

#define get_parameter_string()\
char*	parameter_string = cliGetNextSegment();\
if (parameter_string == NULL)\
{\
	return false;\
}

#define get_number(min, max)\
long long	number;\
get_parameter_string()\
number = atoll(parameter_string);\
if (number < min || number > max)\
{\
	return false;\
}

static int add_boolean(nxtParameter* parameter)
{
	get_parameter_string()

	if (strcmp(parameter_string, "TRUE") == 0)
	{
		parameter->value.boolean = true;
	}
	else if (strcmp(parameter_string, "FALSE") == 0)
	{
		parameter->value.boolean = false;
	}
	else
	{
		return false;
	}

	return true;
}

static int add_ubyte(nxtParameter* parameter)
{
	get_number(NXT_UBYTE_MIN, NXT_UBYTE_MAX)

	parameter->value.ubyte = number & 0xFF;

	return true;
}

static int add_sbyte(nxtParameter* parameter)
{
	get_number(NXT_SBYTE_MIN, NXT_SBYTE_MAX)

	if (number < 0)
	{
		parameter->value.sbyte = (number & 0xFF) | 0x80;
	}
	else
	{
		parameter->value.sbyte = number & 0xFF;
	}

	return true;
}

static int add_uword(nxtParameter* parameter)
{
	get_number(NXT_UWORD_MIN, NXT_UWORD_MAX)

	parameter->value.uword = number & 0xFFFF;

	return true;
}

static int add_sword(nxtParameter* parameter)
{
	get_number(NXT_SWORD_MIN, NXT_SWORD_MAX)

	if (number < 0)
	{
		parameter->value.sword = (number & 0xFFFF) | 0x8000;
	}
	else
	{
		parameter->value.sword = number & 0xFFFF;
	}

	return true;
}

static int add_ulong(nxtParameter* parameter)
{
	get_number(NXT_ULONG_MIN, NXT_ULONG_MAX)

	parameter->value.ulong = number & 0xFFFFFFFF;

	return true;
}

static int add_slong(nxtParameter* parameter)
{
	get_number(NXT_SLONG_MIN, NXT_SLONG_MAX)

	if (number < 0)
	{
		parameter->value.slong = (number & 0xFFFFFFFF) | 0x80000000;
	}
	else
	{
		parameter->value.slong = number & 0xFFFFFFFF;
	}

	return true;
}

static int add_bytes(nxtParameter* parameter)
{
	return false;
}

static int add_string(nxtParameter* parameter)
{
	get_parameter_string();

	if (parameter->length >= 0)
	{
		if (strlen(parameter_string) > parameter->length - 1)
		{
			return false;
		}
	}

	parameter->value.string = strdup(parameter_string);

	return true;
}

static int add_filename(nxtParameter* parameter)
{
	get_parameter_string();

	if (strlen(parameter_string) > 19)
	{
		return false;
	}

	parameter->value.string = strdup(parameter_string);

	return true;
}

static char* get_boolean(nxtResponse response)
{
	switch (response.value.boolean)
	{
		case true:
			return strdup("TRUE");
		case false:
			return strdup("FALSE");
		default:
			return NULL;
	}
}

static char* get_ubyte(nxtResponse response)
{
	char*	result;

	result = malloc(4);
	sprintf(result, "%u", response.value.ubyte);

	return result;
}

static char* get_sbyte(nxtResponse response)
{
	char*	result;

	result = malloc(5);
	sprintf(result, "%+d", response.value.sbyte);

	return result;
}

static char* get_uword(nxtResponse response)
{
	char*	result;

	result = malloc(6);
	sprintf(result, "%u", response.value.uword);

	return result;
}

static char* get_sword(nxtResponse response)
{
	char*	result;

	result = malloc(7);
	sprintf(result, "%+d", response.value.sword);

	return result;
}

static char* get_ulong(nxtResponse response)
{
	char*	result;

	result = malloc(11);
	sprintf(result, "%u", response.value.ulong);

	return result;
}

static char* get_slong(nxtResponse response)
{
	char*	result;

	result = malloc(12);
	sprintf(result, "%+d", response.value.slong);

	return result;
}

static char* get_bytes(nxtResponse response)
{
	return hexdump(response.value.bytes, response.length);
}

static char* get_string(nxtResponse response)
{
	char*	result;

	result = malloc(strlen(response.value.string) + 3);
	sprintf(result, "\"%s\"", response.value.string);

	return result;
}

static char* get_filename(nxtResponse response)
{
	char*	result;

	if (strlen(response.value.string) > 19)
	{
		return NULL;
	}

	result = malloc(strlen(response.value.string) + 3);
	sprintf(result, "\"%s\"", response.value.string);

	return result;
}

static char* hexdump(uint8_t* bytes, int length)
{
	#define result_append_string(string)\
	sprintf(temp_string, string);\
	result = realloc(result, strlen(result) + strlen(temp_string) + 1);\
	strcat(result, temp_string);

	#define result_append_value(format, value)\
	sprintf(temp_string, format, value);\
	result = realloc(result, strlen(result) + strlen(temp_string) + 1);\
	strcat(result, temp_string);

	int	current_position;
	char*	temp_string;
	char*	result;

	temp_string = malloc(16);

	result = malloc(1);
	result[0] = 0;

	current_position = 0;
	while (current_position < length)
	{
		if ((current_position % 16) == 0)
		{
			result_append_value("\n %08x  ", current_position)
		}
		else if ((current_position % 8) == 0)
		{
			result_append_string(" ")
		}

		result_append_value("%02x ", bytes[current_position])

		current_position += 1;

		if ((current_position % 16) == 0)
		{
			result_append_string(" |")

			current_position = current_position - 16;
			if (bytes[current_position] >= 0x20 && bytes[current_position] < 0x7F)
			{
				result_append_value("%c", (char) bytes[current_position])
			}
			else
			{
				result_append_string(".")
			}
			current_position += 1;
			while ((current_position % 16) != 0)
			{
				if (bytes[current_position] >= 0x20 && bytes[current_position] < 0x7F)
				{
					result_append_value("%c", (char) bytes[current_position])
				}
				else
				{
					result_append_string(".")
				}
				current_position += 1;
			}

			result_append_string("|")
		}
	}

	if ((current_position % 16) != 0)
	{
		while ((current_position % 16) != 0)
		{
			if ((current_position % 8) == 0)
			{
				result_append_string(" ")
			}
			result_append_string("   ")
			current_position += 1;
		}

		result_append_string(" |")

		current_position = current_position - 16;
		if (bytes[current_position] >= 0x20 && bytes[current_position] < 0x7F)
		{
			result_append_value("%c", (char) bytes[current_position])
		}
		else
		{
			result_append_string(".")
		}
		current_position += 1;
		while (current_position < length)
		{
			if (bytes[current_position] >= 0x20 && bytes[current_position] < 0x7F)
			{
				result_append_value("%c", (char) bytes[current_position])
			}
			else
			{
				result_append_string(".")
			}
			current_position += 1;
		}

		result_append_string("|")
	}

	free(temp_string);

	return result;
}
