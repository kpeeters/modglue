/* 

   $Id: shell.hh,v 1.14 2001/08/17 18:39:20 t16 Exp $

	The modglue shell, connecting modglue binaries.
	Copyright (C) 2001  Kasper Peeters <k.peeters@damtp.cam.ac.uk>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
*/

#ifndef shell_hh_
#define shell_hh_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <utility>
#include <sigc++/signal_system.h>
#include <modglue/ext_process.hh>
#include <modglue/pipe.hh>
#include <modglue/main.hh>

namespace modglue {

 class process_info {
	 public:
		 process_info(const std::string& name, const std::string& unique_name);

		 modglue::ext_process process;

		 bool start_on_input;
		 bool restart_after_exit;
		 bool abort_on_failed_write;
		 bool remove_after_exit;
	 private:
		 std::string unique_name_;
 };

 class bond {
	 public:
		 bond(const std::string&);
		 
		 typedef std::set<std::pair<std::string, std::string> >::const_iterator const_iterator;
		 typedef std::set<std::pair<std::string, std::string> >::iterator       iterator;
		 
		 std::string name;
		 std::set<std::pair<std::string, std::string> > pipes;
 };

 class loader : public SigC::Object {
	 public:
		 loader(modglue::main *, modglue::ipipe *, modglue::opipe *, modglue::opipe *);
		 ~loader();

		 void quit(void);
		 void print_prompt(void);
		 bool accept_commands(modglue::ipipe& p);
		 bool accept_commands_old(std::istream& p);

		 bool pipe_input(modglue::ipipe& p);
		 void process_died(modglue::ext_process& pr);
	 private:
		 class groupelem {
			 public:
				 string process;
				 string pipe;
		 };

		 enum status_t { s_initial, s_command, s_skipwhite, s_arg_or_control, 
							  s_scan, s_groupscan, s_curlygroup };
		 std::vector<status_t>    status_;
		 std::vector<std::string> errors_;
		 std::vector<process_info *>  new_processes_;
		 std::string              current_;
		 vector<groupelem>        current_group_;
		 bool                     ingroup_;

		 bool parse_(modglue::ipipe& );
		 void add_bond_(bond *);
		 void remove_bond_(bond *);
		 void print_jobs_(std::ostream& ss);
		 void print_bonds_(std::ostream& ss);

		 main *                       main_;

		 modglue::ipipe              *p_command;
		 modglue::opipe              *p_result;
		 modglue::opipe              *p_error;
		 std::vector<process_info *>  processes_;
		 std::vector<bond *>          bonds_;
 };
 
};

#endif
