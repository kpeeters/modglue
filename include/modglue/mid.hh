/*

	Identifiers for messages.
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

#ifndef mid_hh_
#define mid_hh_

#include <string>
#include <iostream>
//#include <proj++/thread.hh>
//#include <proj++/thread_mutex.hh>

namespace modglue {

 class mid {
	 public:
		 mid(void);
		 mid(const mid&);
		 
		 int operator==(const mid&) const;
		 friend std::ostream& operator<<(std::ostream&, const mid&);
	 private:
		 pid_t     process_;
		 int       serial_;

		 static int          highwater_;
 };

 std::ostream& operator<<(std::ostream&, const modglue::mid&);

};

#endif
