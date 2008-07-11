/* 

   $Id: main.hh,v 1.20 2006/03/10 14:23:23 kp229 Exp $

	Event loop handler
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

#ifndef modglue_main_hh_
#define modglue_main_hh_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sigc++/sigc++.h>
#include <modglue/pipe.hh>
#include <modglue/ext_process.hh>

static volatile char signature[]="MODGLUE_1.0_BINARY";

namespace modglue {

	class main /* : public SigC::Object */ {
	 public:
		 main();
		 main(int argc, char **argv);
		 ~main();

		 void add(ipipe *, int default_fd=-1);  // main does not get ownership
		 void add(opipe *, int default_fd=-1);
		 void add(ext_process *);
		 void run(int min_pipes);
		 bool check(void);

		 // When using a non-modglue event loop, the file descriptors on which
		 // to listen for modglue can be obtained through the pipe structures.
		 // The user is responsible for making sure that the select_callback() gets
		 // called when data is available or exceptions occur on one of these pipes.

		 void last_read(void);
		 int  fds_to_watch(std::vector<int>& fds) const;
		 bool select_callback(int);            // false if connection should be disconnected 

		 sigc::signal1<bool, ext_process&> process_died;

		 std::ofstream debugout;
	 private:
		 static int                 	sig_chld_pipe_[2];    // used by sig_chld_ to signal died process 
                                                          // is a copy installed by setup_signal_handlers
//		 int                          old_sig_chld_pipe_[2];
		 struct sigaction             old_sigaction_chld_;
		 struct sigaction             old_sigaction_term_;
		 struct sigaction             old_sigaction_int_;
									       
		 bool                       	list_pipes_;
		 std::vector<ipipe *>       	ipipes_;
		 std::vector<opipe *>       	opipes_;
		 int                        	argc_;
		 char                       **argv_;
		 std::map<std::string, int>   pipe_fds_;
		 std::vector<ext_process *>   processes_;
		 bool                         terminate_main_loop_;
		 
		 void          select_loop_(int min_pipes);
		 std::string   build_pipe_list(void) const;
		 void          setup_signal_handlers_(void);
		 void          restore_signal_handlers_(void);
		 int           add_fds_(fd_set& rfds, fd_set& efds, int& maxfd) const;
		 static void   sig_chld_(int signo);
		 static void   sig_term_(int signo);
		 

 };

};

#endif
