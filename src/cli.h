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

void cliInit(cliMode mode, int argc, char** argv);
void cliDoCommand();
char* cliGetNextSegment();

char* cliGetParameterValue(cliParameter* parameters, int parameter_count, char* parameter_name);
int cliGetParameterGiven(cliParameter* parameters, int parameter_count, char* parameter_name);
