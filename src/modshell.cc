/* 

	Modglue shell.
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

#include <iostream>
#include <fstream>
#include <modglue/shell.hh>

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	modglue::ipipe p_command("stdin");
	modglue::opipe p_result("stdout");
	modglue::opipe p_error("stderr");

	mm.add(&p_command,0);
	mm.add(&p_result,1);
	mm.add(&p_error,2);

	if(mm.check()) {
		modglue::loader ld(&mm, &p_command, &p_result, &p_error);
		
		if(argc>1) { // read from script before going interactive
			std::ifstream ff(argv[1]);
			ld.accept_commands_old(ff);
			ff.close();
			}

		ld.print_prompt();
		try {
			mm.run(1);
			}
		catch(std::exception& ex) {
			std::cerr << ex.what() << std::endl;
			}
		}

	return 0;
	}
