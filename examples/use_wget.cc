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

#include <string>
#include <modglue/main.hh>
#include <modglue/ext_process.hh>

bool receive_html(modglue::ipipe& p)
	{
	std::string str;
	while(getline(p,str)) {
		
		}
	p.clear();
	return true;
	}

bool receive_info(modglue::ipipe& p)
	{
	std::string str;
	while(getline(p,str)) {
		}
	std::cerr << "." << std::flush;
	p.clear();
	return true;
	}

bool receive_exitstatus(modglue::ext_process& pr)
	{
	std::cerr << "exit code:" << pr.exit_code() << std::endl;
	return true;
	}

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	modglue::ext_process wgett("wget");
	wgett << "-O" << "-";

	try {
		modglue::ext_process wget1(wgett);
		wget1 << "http://www.ictp.trieste.it";
		wget1.setup_pipes();
		mm.add(&wget1);
		wget1.input_pipe("stdout")->receiver.connect(sigc::ptr_fun(receive_html));
		wget1.input_pipe("stderr")->receiver.connect(sigc::ptr_fun(receive_info));
		mm.process_died.connect(sigc::ptr_fun(receive_exitstatus));
		wget1.fork();

		/*

		  This would be cleaner when written as:
        wget.set_io(&receive_html,&receive_info);
		  wget.fork();

		 */

		mm.run(1);
		}
	catch(std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		}
	}
