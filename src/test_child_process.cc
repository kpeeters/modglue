/* 

	Testing the process class.
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

#include <fstream>
#include <iostream>
#include <modglue/process.hh>
#include <modglue/ext_process.hh>

using namespace std;

int main(int argc, char **argv)
	{
	string result;

	modglue::child_process ls_proc("ls");
	ls_proc << "-la" << "/tmp";
	ls_proc.call("", result);
	
	std::cout << "result of ls call: |" << result << "|" << std::endl;


	// like libexecstream: http://libexecstream.sourceforge.net/
	modglue::ext_process cat_proc("cat");
	cat_proc.setup_pipes();
	cat_proc.output_pipe("stdin")->set_blocking();
	cat_proc.fork();
	std::cout << cat_proc.output_pipe("stdin")->fd() << std::endl;
	*(cat_proc.output_pipe("stdin"))	<< "hi there" << std::endl;
	while(std::getline(*(cat_proc.input_pipe("stdout")), result)) {
		std::cout << "received: |" << result << "|" << std::endl;
		}
	cat_proc.terminate();
	}
