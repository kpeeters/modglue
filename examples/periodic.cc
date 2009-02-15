/* 

	Test program to yield output at given interval.
	Copyright (C) 2001-2009  Kasper Peeters <kasper.peeters@aei.mpg.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
	
*/

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
	{
	int counter=1;
	char buffer[100];

	while(true) {
		if(argc>1) {
			if(write(1, argv[1], strlen(argv[1]))<strlen(argv[1]))
				exit(1);
			}
		else { 
			sprintf(buffer, "test %d", counter);
			if(write(1, buffer, strlen(buffer))<strlen(buffer))
				exit(1);
			}
		++counter;
		write(1, "\n", 1);
		sleep(1);
		}
	return 0;
	}
