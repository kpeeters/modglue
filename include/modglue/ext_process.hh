/* 

   $Id: ext_process.hh,v 1.11 2006/03/10 14:23:23 kp229 Exp $

	Extended process class (with pipe interface instead of bare
   file descriptors).
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

#ifndef ext_process_hh_
#define ext_process_hh_

#include <string>
#include <vector>
#include <modglue/pipe.hh>

namespace modglue {

	class ext_process /*: public SigC::Object */ {
		public:
			ext_process(const std::string&);
			ext_process(const std::string&, const std::vector<std::string>&);
			ext_process(const ext_process& ); // only copies name and arguments
			~ext_process();

			void  setup_pipes(void);
			ext_process& operator<<(const std::string&);

			void  fork();
			void  terminate(int exit_code=0);
			void  pause();
			void  restart();

			std::string  name(void) const;
			pid_t        get_pid() const;
			int          exit_code() const;

			const std::vector<std::string>& args(void) const;
			const std::vector<ipipe *>&     input_pipes(void) const;
			const std::vector<opipe *>&     output_pipes(void) const; 
			ipipe*                          input_pipe(const std::string&) const;
			opipe*                          output_pipe(const std::string&) const;

			const std::vector<std::string>& output(void) const; // FIXME: remove
		private:
			void determine_path_(void);
			void determine_binary_type_(void);
			void setup_pipes_from_string_(const std::string& desc);
			void open_pipes_(void);
			void dup_unix_pipes_(void);
			void close_parentside_(void);
			void close_childside_(void);
			bool receive_output_(ipipe&);

			std::string              name_;
			std::vector<std::string> args_;

			std::string              full_path_;
			bool                     modglue_binary_;
			std::vector<ipipe *>     input_pipes_;
			std::vector<opipe *>     output_pipes_;
			pid_t                    pid_;
			bool                     paused_;
			int                      exit_code_;
			bool                     pipes_done_;
			std::vector<std::string> output_;
	};
};


#endif
