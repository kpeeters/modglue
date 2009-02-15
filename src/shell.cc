/* 

	Modglue shell.
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
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>

#include <values.h>

#include <sigc++/signal_system.h>
#include <modglue/main.hh>
#include <modglue/pipe.hh>
#include <modglue/ext_process.hh>
#include <modglue/shell.hh>

using namespace modglue;

process_info::process_info(const std::string& name, const std::string& unique_name)
	: process(name), start_on_input(false), restart_after_exit(false), 
	  abort_on_failed_write(false), remove_after_exit(false), unique_name_(unique_name)
	{
	}

// --------------

bond::bond(const std::string& nm)
	: name(nm)
	{
	}

// --------------

void loader::print_prompt(void)
	{
	(*p_result) << ">" << std::flush;
	}

void loader::print_jobs_(std::ostream& ss)
	{
	ss << "process list: (" << processes_.size() << " entries)" << std::endl;
	for(unsigned int i=0; i<processes_.size(); ++i) {
		ss << processes_[i]->process.name();
		if(processes_[i]->process.get_pid()==0)
			ss << " (standby)" << std::endl;
		else
			ss << " (running)" << std::endl;

		for(unsigned int j=0; j<processes_[i]->process.input_pipes().size(); ++j) {
			ss << " " << processes_[i]->process.name() << "::"
				<< processes_[i]->process.input_pipes()[j] << std::endl;
			}
		for(unsigned int j=0; j<processes_[i]->process.output_pipes().size(); ++j) {
			ss << " " << processes_[i]->process.name() << "::"
				<< processes_[i]->process.output_pipes()[j] << std::endl;
			}
		}
	}

void loader::print_bonds_(std::ostream& ss)
	{
	for(unsigned int i=0; i<bonds_.size(); ++i) {
		ss << bonds_[i]->name << ": " ;
		bond::iterator it=bonds_[i]->pipes.begin();
		while(it!=bonds_[i]->pipes.end()) {
//			ss << (*it).first->get_name() << "::"
//				<< (*it).first->pipes[(*it).second]->get_name() << " ";
			++it;
			}
		ss << std::endl;
		}
	}

bool loader::parse_(modglue::ipipe& p)
	{
	char c;
	while(p.get(c)) {
		switch(status_.back()) {
			case s_skipwhite:
				if(!isspace(c)) {
					p.unget();
					status_.pop_back();
					}
				break;
			case s_initial:
				assert(!isspace(c));
				if(c=='|' || c=='^' || c=='&') {
					errors_.push_back("illegal character at begin of input");
					return false;
					}
				else {
					status_.pop_back();
					status_.push_back(s_skipwhite);
					status_.push_back(s_scan);
					p.unget();
					break;
					}
				break;
			case s_scan:
				if(c=='{') {
					ingroup_=true;
					status_.push_back(s_groupscan);
					status_.push_back(s_skipwhite);
					break;
					}
				else {
					status_.push_back(s_command);
					p.unget();
					}
				break;
			case s_groupscan:
				break;
			case s_command:
				if(c=='}') {
					if(!ingroup_) {
						errors_.push_back("encountered superfluous '}' character");
						return false;
						}
					else {
						ingroup_=false;
						break;
						}
					}
				else if(isspace(c)) {
					status_.push_back(s_skipwhite);
					status_.push_back(s_arg_or_control);
					new_processes_.push_back(new process_info(current_, current_));
					current_="";
					break;
					}
				else if(c=='|' || c=='^' || c=='&') {
					p.unget();
					status_.push_back(s_arg_or_control);
					new_processes_.push_back(new process_info(current_, current_));
					current_="";
					break;
					}
				else
					current_+=c;
				break;
			case s_arg_or_control:
				if(c=='>') {
					}
				if(c=='<') {
					}
				if(c=='&') {
					}
				if(c=='^') { 
					}
				if(c=='|' || c=='^' || c=='&') {
					}
				break;
			}
		}
	p.clear();
	return true;
	}


/*

   Process connections (multiple pipes can be connected as one using the 
   grouping symbols):

     viewer &
     ftp &
     cat "open ftp.somewhere.net\n" | ftp::stdin
     cat "binary\n get file -\n"    | ftp::stdin
     { ftp::stdout, ftp::stderr } | viewer::stdin

   Pipe options (always written using a dot immediately following the
   pipe symbol; multiple options can be given by separating them with
   dots):

     ls ::stdout > out.txt |.discard          (default)
     ls ::stdout > out.txt ::stderr |.keep

   Making connections and then starting (curly brackets denote grouping
   and lead, in the second case, to start of both processes before any
   other activities take place):

     { ls, grep } ^
     ls::stdout | grep::stdin
     { ls, grep } @

   Identical programs with different names:

     { ls:binls /bin, ls:tmpls /tmp } ^
     binls::stdout | ::stdin grep file &
     tmpls::stdout | ::stdin grep dir  &
     { binls, tmpls } @

   Alternatively, use the automatically generated names (but beware that
   the numbers can change if you had other processes with the same name
   running already; see 'jobs' for the real names):

     { ls /bin, ls /tmp } ^
     ls::stdout    | ::stdin grep file &
     ls<2>::stdout | ::stdin grep dir  &
     { ls, ls<2> } @

   ::pipe refers to the pipe of the current program; a pipe symbol starts a new
   program group.

   If pipes are omitted, the default settings are used. These are searched
   from top to bottom, and search stops as soon as a match is found.

     default ::stdout | ::stdin
     default ::stdin  | ::stdout
     default ::*      |.discard

     ls > txt.out   (behaves as usual)


   Curly braces group either processes or pipes, but not a combination.
   They can also not be used to group entire commands (ie. like round
   brackets do in sh). Curly braces cannot be nested.


LOOK AT:
http://www.holoweb.net/~simpl/

 */


bool loader::accept_commands_old(std::istream& p)
	{
	return true;
	}

bool loader::accept_commands(modglue::ipipe& p)
	{
	if(parse_(p)==false) {
		p.ignore(MAXINT);
		}
	if(errors_.size()>0) {
		for(unsigned int i=0; i<errors_.size(); ++i) {
			(*p_result) << errors_[i] << std::endl;
			}
		errors_.clear();
		}
	if(new_processes_.size()>0) {
		for(unsigned int i=0; i<new_processes_.size(); ++i) {
			(*p_result) << new_processes_[i]->process.name() << std::endl;
			}
		}
	print_prompt();
	return true;
	}

loader::loader(modglue::main *mn, modglue::ipipe *p_c, modglue::opipe *p_r, modglue::opipe *p_e)
	: ingroup_(false), main_(mn), p_command(p_c), p_result(p_r), p_error(p_e)
	{
	status_.push_back(s_initial);
	status_.push_back(s_skipwhite);
	p_command->receiver.connect(SigC::slot(*this, &modglue::loader::accept_commands));
	}

void loader::quit(void)
	{
	}

loader::~loader()
	{
	for(unsigned int i=0; i<processes_.size(); ++i)
		delete processes_[i];
	}

bool loader::pipe_input(modglue::ipipe& p)
	{
	}

	// Find all the read pipes to which the given process would be
	// able to write data. If there the given process is the _only_ process
	// writing data to such a pipe (ie. if there are no other bonds active
	// for which there is a writing process with non-zero pid)
	// notify the read pipe by writing a ^D.
	// ONLY does this for modglue processes though.
//	  for(unsigned int i=0; i<bonds_.size(); ++i) {
//		  bond::iterator mit=bonds_[i]->pipes.begin();
//		  while(mit!=bonds_[i]->pipes.end()) {
//			  if((*mit).first==proc) {
//				  set<pair<process *, int> >::iterator it=bonds_[i]->pipes.begin();
//				  while(it!=bonds_[i]->pipes.end()) {
//					  if(it!=mit) {
//						  if((*it).first->pipes[(*it).second]->get_inout()==pipeinfo::input &&
//							  !((*it).first->pipes[(*it).second]->get_unix_style()) ) {
//							  int totalwriters=0;
//							  pair<process *, int> scanfor=(*it);
//							  for(unsigned int k=0; k<bonds_.size(); ++k) {
//								  if(bonds_[k]->pipes.count(scanfor)>0) {
//									  set<pair<process *, int> >::iterator wr=bonds_[k]->pipes.begin();
//									  while(wr!=bonds_[k]->pipes.end()) {
//										  if((*wr).first->get_pid()!=0  && 
//											  (*wr).first->pipes[(*wr).second]->get_inout()==pipeinfo::output) {
//											  ++totalwriters;
//											  }
//										  ++wr;
//										  }
//									  }
//								  }
//							  if(totalwriters==0) {
//								  write((*it).first->pipes[(*it).second]->get_fd(),"\004",1);
//								  }
//							  }
//						  }
//					  ++it;
//					  }
//				  }
//			  ++mit;
//			  }
//		  }	

void loader::add_bond_(bond *b)
	{
	bonds_.push_back(b);
	}

void loader::remove_bond_(bond *b)
	{
	std::vector<bond *>::iterator it=bonds_.begin();
	while(it!=bonds_.end()) {
		if((*it)==b) {
			bonds_.erase(it);
			return;
			}
		++it;
		}
	assert(1==0);
	}


/*
pair<process *, int> loader::str2pipe(const string& str)
	{
	pair<process *,int> ret;
	ret.first=0; ret.second=-1;

	string::size_type loc=str.find("::");
	if(loc<str.size()) {
		string procname=str.substr(0,loc);
		string pipename=str.substr(loc+2);
		for(unsigned int i=0; i<processes_.size(); ++i) {
			if(processes_[i]->get_name()==procname) {
				ret.first=processes_[i];
				break;
				}
			}
		if(ret.first) {
			for(unsigned int i=0; i<ret.first->pipes.size(); ++i) {
				if(ret.first->pipes[i]->get_name()==pipename) {
					ret.second=i;
					break;
					}
				}
			}
		}
	return ret;
	}


	vector<string> args;
	// Setup a process entry and the associated loader pipes for the modshell
	// process, so that we don't have to use special cases for forwarding
	// to/from modshell pipes.
	// NOTE: the direction is a bit funny, because we should view this from
	// the point of view of someone connecting _to_ modshell. So stdout is input.
	modglue::process *proc=new modglue::process("_loader",args);
	loader_pipe *lp1=new loader_pipe(main_->pipes()[0]->get_name(), 
												modglue::pipeinfo::output);
	lp1->set_fd(main_->pipes()[0]->get_fd());
	lp1->set_unix_style(); // FIXME: this will make things fail when modshell is started from modshell.
	proc->pipes.push_back(lp1);

	loader_pipe *lp2=new loader_pipe(main_->pipes()[1]->get_name(), 
												modglue::pipeinfo::input);
	lp2->set_fd(main_->pipes()[1]->get_fd());
	lp2->set_unix_style();
	proc->pipes.push_back(lp2);

	loader_pipe *lp3=new loader_pipe(main_->pipes()[2]->get_name(), 
												modglue::pipeinfo::input);
	lp3->set_fd(main_->pipes()[2]->get_fd());
	lp3->set_unix_style();
	proc->pipes.push_back(lp3);

	proc->childpid_=getpid();
	processes_.push_back(proc);
	fd_to_pipe_[lp1->get_fd()]=pair<process *, int>(proc,0);
	fd_to_pipe_[lp2->get_fd()]=pair<process *, int>(proc,1);
	fd_to_pipe_[lp3->get_fd()]=pair<process *, int>(proc,2);

*/
