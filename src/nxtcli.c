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

#include <signal.h>

#include <libnxtbt/libnxtbt.h>

#include "cli.h"
#include "nxtcommands.h"

int	gDone;

static void cleanup();
void signal_handler(int signal);

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Must specify NXT device file\n");
		exit(1);
	}

	signal(SIGINT, signal_handler);

	nxtOpen(argv[1]);
	nxtcommandsLoadCommandsFromXML(HERE "/commands.xml");

	if (argc > 2)
	{
		cliInit(CLI_MODE_ARGS, argc, argv);
		cliDoCommand();
	}
	else
	{
		cliInit(CLI_MODE_INTERACTIVE, 0, NULL);

		gDone = false;
		while (gDone == false)
		{
			cliDoCommand();
		}
	}

	cleanup();
}

static void cleanup()
{
	nxtClose();
	exit(0);
}

void signal_handler(int signal)
{
	cleanup();
}
