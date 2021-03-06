/* 

	Simple tests of pipes.
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

#include <modglue/main.hh>
#include <modglue/pipe.hh>

using namespace std;

modglue::ipipe ip("input");
modglue::opipe op("input");

bool inp(modglue::ipipe& p)
	{
	string kasper;
	while(p>>kasper) {
		op << "received: " << kasper << endl;
		}
	p.clear();
	op << "--" << endl;

	return true;
	}

int main(int argc, char **argv)
	{
/*
   modglue::main::init(argc, argv);
   
 */

	modglue::main mm(argc, argv);

	mm.add(&ip,0);
	mm.add(&op,1);

	if(mm.check()) {
		ip.receiver.connect(sigc::ptr_fun(inp));
		mm.run(1);
		}

	// Also test that creation of pipes does not leak memory.
	}
