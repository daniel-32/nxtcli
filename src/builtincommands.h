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

extern builtincommandsCommand	gBuiltinCommands[];
extern int	gBuiltinCommandsCount;

void builtincommandsls(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsget(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsput(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsrm(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandshelp(cliParameter* cli_parameters, int cli_parameter_count);
void builtincommandsexit(cliParameter* cli_parameters, int cli_parameter_count);
