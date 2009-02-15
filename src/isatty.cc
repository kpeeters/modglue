/* 

	Wrapper around isatty(3).
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
#include <unistd.h>

int main(int argc, char **argv)
	{
	std::cout << ((isatty(0)==1)?"stdin  is a tty":"stdin  is NOT a tty") << std::endl;
	std::cout << ((isatty(1)==1)?"stdout is a tty":"stdout is NOT a tty") << std::endl;
	std::cout << ((isatty(2)==1)?"stderr is a tty":"stderr is NOT a tty") << std::endl;

	return(2);
	}
