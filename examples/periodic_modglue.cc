/* 

	Modglue example program.
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <modglue/modglue.hh>
#include <sigc++/signal_system.h>
#include <iostream.h>

modglue::pipe *foopipe;

main(int argc, char **argv)
	{
   modglue::main *mm=modglue::loader::instance().create_main(argc, argv);
	foopipe=new modglue::pipe("stdout", modglue::pipe::output);
   mm->add(foopipe);

	if(mm->check()) {
		int counter=1;
		char buffer[100];
		
		while(true) {
			sprintf(buffer, "test %d\n", counter);
			foopipe->sender(buffer);
			++counter;
			sleep(1);
			}
		}
	}

