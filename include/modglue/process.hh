/* 

   $Id: process.hh,v 1.9 2007/02/19 11:01:16 peekas Exp $

	Process class (for forking external processes)
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

#ifndef process_hh_
#define process_hh_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unistd.h>
#include <sigc++/sigc++.h>
//#include <proj++/thread.hh>
//#include <proj++/thread_mutex.hh>

namespace modglue {

 /// A class to start external processes and communicate with them in a blocking
 /// way. If you need asynchronous communication, use the ext_process class.

 class child_process {
	 public:
		 child_process(const std::string&);
		 child_process(const std::string&, const std::vector<std::string>&);
		 ~child_process();

		 /// Higher-level communication with the external process.
		 void  call(const std::string& stdin_txt, std::string& stdout_txt);
		 void  call(const std::string& stdin_txt, std::string& stdout_txt, std::string& stderr_txt);
		 void  write(const std::string&);
		 void  read(std::string&);

		 /// Compatibility functions mimicking Unix read/write.
		 ssize_t  read(void *, size_t len);
		 ssize_t  write(void *, size_t len);

		 /// Starting and stopping.
		 void  fork();             // start the process
		 void  terminate();        // terminate the process with kill
		 void  close();            // close all pipes
		 void  wait();             // wait until process ends
		 pid_t get_pid() const;
		 
		 /// Name and arguments of the child process
		 std::string         name;
		 std::vector<std::string> args;
		 child_process& operator<<(const std::string&);

		 /// Pipe related members
		 void  standard_pipes();
		 class fd_pair {
			 public:
				 enum direction_t { child_output, child_input };
				 fd_pair(int, direction_t);

				 int child_fd;  // better call this `external_fd'
				 int parent_fd; // and this `our_fd'
				 direction_t direction;
		 };
		 std::vector<fd_pair> pipes;
	 private:
		 pid_t pid_;
 };

};


#endif
