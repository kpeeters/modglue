/* 

	Modglue library entry points.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <modglue/main.hh>

using namespace modglue;

int modglue::main::sig_chld_pipe_[2];

modglue::main *themm;

void main::setup_signal_handlers_(void)
	{
	themm=this;

	if(::pipe(sig_chld_pipe_)) {
		throw std::logic_error("cannot create sig_chld_pipe");
		}
	fcntl(sig_chld_pipe_[0], F_SETFL, O_NONBLOCK);

	struct sigaction act;

	act.sa_handler = &main::sig_chld_;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	if(sigaction(SIGCHLD, &act, &old_sigaction_chld_) < 0) {
		throw std::logic_error("sigaction failed");
		}
	
	act.sa_handler = &main::sig_term_;
	sigemptyset(&act.sa_mask);
	if(sigaction(SIGTERM, &act, &old_sigaction_term_) < 0) {
		throw std::logic_error("sigaction failed");
		}


   // When we are in the middle of a write subroutine and the process
	// to which we write terminates, we catch the SIG_CLD signal but
	// we cannot react to that before a certain sync point. If we happen
	// to write to the sockets, we get a SIGPIPE. Since we handle pipe
	// closing afterwards, everything will be fine if we ignore this
	// signal.
	signal(SIGPIPE, SIG_IGN); 
	}

void main::restore_signal_handlers_(void)
	{
	close(sig_chld_pipe_[0]);
	close(sig_chld_pipe_[1]);
	sigaction(SIGCHLD, &old_sigaction_chld_, NULL);
	sigaction(SIGTERM, &old_sigaction_term_, NULL);
	}

void main::sig_chld_(int signo)
	{
	int status;
	pid_t childpid;

//	std::cerr << "*****" << std::endl;

	char buffer[100];
	if((childpid=waitpid(-1, &status, WNOHANG)) < 0) {
		if(errno==ECHILD) 		// No more children left, return immediately.
			return;
		else {
			sprintf(buffer, "waitpid failed, errno = %d", errno);
			throw std::logic_error(buffer);
			}
		}
	
	if(WIFEXITED(status)) {
//		std::cerr << "***** exited" << std::endl;
		int child_val = WEXITSTATUS(status); 
		sprintf(buffer, "%ld %d", (long)childpid, child_val);
		write(main::sig_chld_pipe_[1], buffer, strlen(buffer));
		} 
	else if(WIFSIGNALED(status)) {
//		std::cerr << "***** signalled" << std::endl;
		sprintf(buffer, "%ld", (long)childpid);
		write(main::sig_chld_pipe_[1], buffer, strlen(buffer));
		} 
	}

void main::sig_term_(int signo)
	{
	// This does of course not catch SIGKILL, that will still leave
	// process adopted by init. But there is no way around that, apparently.

	write(main::sig_chld_pipe_[1], "T", 1);
	}

//-----------------

main::main(void)
	: list_pipes_(false), argc_(0), argv_(0), terminate_main_loop_(false)
	{
	setup_signal_handlers_();
	}

main::main(int argc, char **argv)
	: list_pipes_(false), argc_(argc), argv_(argv), terminate_main_loop_(false)
	{
	setup_signal_handlers_();

	int ac=1;
	while(ac<argc_ && argv_[ac][0] == '-') {
	   std::string argbit=argv[ac];
		if(argbit.substr(0,12)=="--list-pipes") {
			list_pipes_=true;
			}
		else if(argbit.substr(0,7)=="--pipe=") {
			unsigned int pos=argbit.find_first_of(",");
			std::string name=argbit.substr(7,pos-7);
			int    fd  =atoi(argbit.substr(pos+1).c_str());
			pipe_fds_[name]=fd;
			}
		++ac;
		}
	}

void main::add(opipe *p, int default_fd)
	{
	opipes_.push_back(p);

	if(p->fd()==-1) {
		std::map<std::string, int>::const_iterator it=pipe_fds_.find(p->name());
		if(it==pipe_fds_.end()) {
			if(p->name()=="stdin") {
				throw std::logic_error("pipe name stdin and type output incompatible");
				}
			else if(p->name()=="stdout") {
				p->open(1);
				p->set_unix_style();
				}
			else if(p->name()=="stderr") {
				p->open(2);
				p->set_unix_style();
				}
			else if(default_fd!=-1) {
				p->open(default_fd);
				p->set_unix_style();
				}
			}
		else p->open((*it).second);
		}
	}

void main::add(ipipe *p, int default_fd)
	{
	ipipes_.push_back(p);

	if(p->fd()==-1) {
		std::map<std::string, int>::const_iterator it=pipe_fds_.find(p->name());
		if(it==pipe_fds_.end()) {
			if(p->name()=="stdin") {
				p->open(0);
				p->set_unix_style();
				}
			else if(p->name()=="stdout") {
				throw std::logic_error("pipe name stdout and type input incompatible");
				}
			else if(p->name()=="stderr") {
				throw std::logic_error("pipe name stdout and type input incompatible");
				}
			else if(default_fd!=-1) {
				p->open(default_fd);
				p->set_unix_style();
				}
			}
		else p->open((*it).second);
		}
	}

void main::add(ext_process *proc)
	{
	processes_.push_back(proc);
	for(unsigned int i=0; i<proc->input_pipes().size(); ++i) {
		proc->input_pipes()[i]->process=proc;
		}
	for(unsigned int i=0; i<proc->output_pipes().size(); ++i) {
		proc->output_pipes()[i]->process=proc;
		}
	}

main::~main()
	{
	// FIXME: should we terminate all processes? Not sure, we do 
	// not own them after all.
	restore_signal_handlers_();
	}

void main::run(int min_pipes)
	{
	if(check())
		select_loop_(min_pipes);
	}

std::string main::build_pipe_list(void) const
	{
	std::stringstream str;
	for(unsigned int i=0; i<ipipes_.size(); ++i) {
		str << "{ \"" << ipipes_[i]->name() << "\" , "
			 << "input"
			 << " }" << std::endl;
		}
	for(unsigned int i=0; i<opipes_.size(); ++i) {
		str << "{ \"" << opipes_[i]->name() << "\" , "
			 << "output"
			 << " }" << std::endl;
		}
	str << std::ends;
	return str.str();
	}

bool main::check(void)
	{
	if(list_pipes_) {
		std::cout << build_pipe_list() << std::flush;
		return false;
		}
	else return true;
	}

void main::last_read(void)
	{
	for(unsigned int i=0; i<ipipes_.size(); ++i) {
		if(ipipes_[i]->is_broken()==false) {
			ipipes_[i]->first_read_after_select();
			ipipes_[i]->receiver(*(ipipes_[i]));
			}
		}
	for(unsigned int i=0; i<processes_.size(); ++i) {
		for(unsigned int k=0; k<processes_[i]->input_pipes().size(); ++k) {
			if(processes_[i]->input_pipes()[k]->is_broken()==false) {
				processes_[i]->input_pipes()[k]->first_read_after_select();
				processes_[i]->input_pipes()[k]->receiver(*(processes_[i]->input_pipes()[k]));
				}
			}
		}
	}

bool main::select_callback(int fd)
	{
//	debugout << "sc: " << fd << std::endl;
	if(fd==sig_chld_pipe_[0]) {
//		std::cerr << "sig_chld_pipe" << std::endl;
		char buffer[1024];
		int pos=-1;
		do {
			pos=read(fd,buffer,1023-1);
			} while(pos<0 && (errno==EINTR || errno==EAGAIN));
		if(pos>0) {
//			std::cerr << "sig_chld_pipe " << buffer[0] << std::endl;
			if(buffer[0]=='T') {
				for(unsigned int i=0; i<processes_.size(); ++i) {
					processes_[i]->terminate();
					}
				exit(-1);
				}
			else {
				buffer[pos]=0;
				pid_t childpid=0;
				int   exit_code=0;
//				std::cerr << buffer << std::endl;
				sscanf(buffer, "%ld %d", &childpid, &exit_code);
				// This often gets reached after cerr has gone away. Why?
				for(unsigned int i=0; i<processes_.size(); ++i) {
//					std::cerr << processes_[i]->get_pid() << std::endl;
					if(processes_[i]->get_pid()==childpid) {
//						std::cerr << "process " << processes_[i]->name() << " exited" << std::endl;
						// process all unprocessed data from this process
						for(unsigned int lp=0; lp<processes_[i]->input_pipes().size(); ++lp) {
							processes_[i]->input_pipes()[lp]->receiver(*(processes_[i]->input_pipes()[lp]));
							}
						processes_[i]->terminate(exit_code);
						if(process_died(*(processes_[i])))
							terminate_main_loop_=true;
						break;
						}
					}
				}
			}
		else {
			throw std::logic_error("main::select_callback: read failed");
			}
		return true;
		}

	for(unsigned int i=0; i<ipipes_.size(); ++i) {
		if(ipipes_[i]->is_broken()==false && ipipes_[i]->fd()==fd) {
			ipipes_[i]->first_read_after_select();
			return ipipes_[i]->receiver(*(ipipes_[i]));
			}
		}
	for(unsigned int i=0; i<processes_.size(); ++i) {
		for(unsigned int k=0; k<processes_[i]->input_pipes().size(); ++k) {
			if(processes_[i]->input_pipes()[k]->is_broken()==false && 
				processes_[i]->input_pipes()[k]->fd()==fd) {
				processes_[i]->input_pipes()[k]->first_read_after_select();
				processes_[i]->input_pipes()[k]->receiver(*(processes_[i]->input_pipes()[k]));
				return true;
				}
			}
		}
   // Unknown fd, happens sometimes when processes end and a last
	// select woke up on an already closed pipe. Should figure out 
	// whether this is an exception.
	return false; 
	}

int main::fds_to_watch(std::vector<int>& fds) const
	{
	int maxfd=0;
	// control pipe
	maxfd=std::max(maxfd, sig_chld_pipe_[0]);
	// separate pipes
	for(unsigned int i=0; i<ipipes_.size(); ++i) {
		if(ipipes_[i]->is_broken()==false)
			if(ipipes_[i]->fd()!=-1) {
				fds.push_back(ipipes_[i]->fd());
				maxfd=std::max(maxfd, ipipes_[i]->fd());
				}
		}
	// pipes associated to processes
	for(unsigned int i=0; i<processes_.size(); ++i) {
		// if one of the pipes is broken, the process is going away, and we
		// do not have to select anymore (we do a final read on all pipes
		// when the process really dies).
		bool at_least_one_broken=false;
		for(unsigned int k=0; k<processes_[i]->input_pipes().size(); ++k) {
			if(processes_[i]->input_pipes()[k]->is_broken()) {
				at_least_one_broken=true;
				break;
				}
			}
		if(!at_least_one_broken) {
			for(unsigned int k=0; k<processes_[i]->input_pipes().size(); ++k) {
				fds.push_back(processes_[i]->input_pipes()[k]->fd());
				maxfd=std::max(maxfd, processes_[i]->input_pipes()[k]->fd());
				}
			}
		}
	fds.push_back(sig_chld_pipe_[0]);
//	std::cerr << fds.size() << std::endl;
	return maxfd;
	}

int main::add_fds_(fd_set& rfds, fd_set& efds, int& maxfd) const
	{
	FD_ZERO(&rfds);
	FD_ZERO(&efds);

	std::vector<int> fds;
	maxfd=fds_to_watch(fds);
	for(unsigned int i=0; i<fds.size(); ++i) {
		FD_SET(fds[i], &rfds);
		FD_SET(fds[i], &efds);
		}
	return fds.size();
	}


// What do we do with select_callback which returns false? This is related
// to the issue of buffering input before a callback connection is made.
// Do we need this? Or can we just ignore all incoming data which is 
// not going to be broadcast anyway?


// FIXME: we now simply wait for terminate_main_loop_. 
// This is ok, but we really need to wait for all other processes
// too before exiting the main loop. 
void main::select_loop_(int min_pipes)
	{
	fd_set rfds;
	fd_set efds;
	int retval;
	int maxfd;

	do {
		int pipes_left=add_fds_(rfds, efds, maxfd);
		if(pipes_left==1)
			break;
		retval = select(maxfd+1, &rfds, NULL, &efds, NULL);
		if(retval<0) {
			if(errno==EINTR) {
				continue;
//				throw std::logic_error("main::select_loop: dont know how to handle EINTR"); 
				break;
				}
			else
				throw std::logic_error("main::select_loop: select failed.");
			}
		for(unsigned int fd=0; fd<maxfd+1; ++fd) {
			if(FD_ISSET(fd, &rfds))
				select_callback(fd);
			}
		} while(!terminate_main_loop_);
	}

