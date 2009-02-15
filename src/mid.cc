/* 

	Message identifiers.
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

#include <modglue/mid.hh>
#include <string>
#include <unistd.h>

using namespace modglue;

int          mid::highwater_=0;

mid::mid(void)
	{
	process_=getpid();
	++highwater_;
	serial_=highwater_;
	}

mid::mid(const mid& other)
	{
	serial_=other.serial_;
	process_=other.process_;
	}

int mid::operator==(const mid& other) const
	{
	return (process_==other.process_ && serial_==other.serial_);
	}

std::ostream& modglue::operator<<(std::ostream& str, const modglue::mid & m) 
   {
   return (str << "mid=" << m.process_ << ", " << m.serial_);
   }


