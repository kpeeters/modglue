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

/*

   Simple program to show usage of multiple I/O pipes in addition
   to the standard stdin/stdout/stderr combo.

 */

#include <modglue/pipe.hh>
#include <modglue/main.hh>
#include <sigc++/sigc++.h>
#include <iostream>

using namespace std;

modglue::ipipe foopipe("foo");
modglue::ipipe foo2pipe("foo2");
modglue::opipe barpipe("bar");

bool print(modglue::ipipe& p)
   {
	string txt;
	while(p >> txt) {
		cerr << "from foo: |" << txt << "|" << endl;
		barpipe << "bar:" << txt << endl;
		}
	p.clear();
	return true;
   }

bool print2(modglue::ipipe& p)
   {
	string txt;
	while(p >> txt) {
		barpipe << "thank you 2!" << endl;
		}
	p.clear();
	return true;
   }

int main(int argc, char **argv) 
   {
   modglue::main mm(argc, argv);

   mm.add(&foopipe,0);
   mm.add(&foo2pipe);
   mm.add(&barpipe,2);

   foopipe.receiver.connect(sigc::ptr_fun(print));
   foo2pipe.receiver.connect(sigc::ptr_fun(print2));
   
   mm.run(2);
   }
