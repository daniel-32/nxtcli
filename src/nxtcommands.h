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

#include <libnxtbt/libnxtbt.h>

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

extern nxtcommandsCommand	gNxtCommands[256];
extern int	gNxtCommandsCount;

void nxtcommandsLoadCommandsFromXML(const char* filename);
void nxtcommandsDoCommand(const char* command);
