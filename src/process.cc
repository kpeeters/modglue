/* 

	Process class.
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

#include <cassert>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <modglue/process.hh>
#include <iostream>

using namespace modglue;

child_process::fd_pair::fd_pair(int fd, direction_t dir)
	: child_fd(fd), parent_fd(-1), direction(dir)
	{
	}

child_process::child_process(const std::string& nm)
	: name(nm), pid_(0)
	{
	}

child_process::child_process(const std::string& nm, const std::vector<std::string>& ar)
	: name(nm), args(ar), pid_(0)
	{
	}

child_process::~child_process()
	{
	close();
	if(pid_ > 0)
		kill(pid_, SIGTERM);
	}

child_process& child_process::operator<<(const std::string& st)
	{
	args.push_back(st);
	return (*this);
	}

void child_process::standard_pipes()
	{
	assert(pid_==0); // cannot be called after program has been forked
	pipes.clear();
	pipes.push_back(fd_pair(0,fd_pair::child_input));
	pipes.push_back(fd_pair(1,fd_pair::child_output));
	pipes.push_back(fd_pair(2,fd_pair::child_output));
	}

void child_process::call(const std::string& inp, std::string& outp)
	{
	std::string outerr;
	call(inp, outp, outerr);
	}

void child_process::call(const std::string& inp, std::string& outp, std::string& outerr)
	{
	standard_pipes();
	fork();
	// FIXME: for very long input std::strings, this leads to failure.
	if(inp.size()>0)
		::write(pipes[0].parent_fd, inp.c_str(), inp.size());
	char buffer[1024];
	int pos;
	stdoutagain:
	while((pos=::read(pipes[1].parent_fd, buffer, 1023))>0) {
		buffer[pos]=0;
		outp+=buffer;
		}
	if(pos<0 && errno==EINTR) goto stdoutagain;

	stderragain:
	while((pos=::read(pipes[2].parent_fd, buffer, 1023))>0) {
		buffer[pos]=0;
		outerr+=buffer;
		}
	if(pos<0 && errno==EINTR) goto stderragain;

	close();
	terminate();
	}

void child_process::terminate()
	{
	if(pid_ > 0) {
		kill(pid_, SIGTERM);
		pid_=0;
		}
	}

void child_process::wait()
	{
	if(pid_ > 0) {
		waitpid(pid_, 0, 0);
		pid_=0;
		}
	}

void child_process::write(const std::string& str) 
	{
	::write(pipes[0].parent_fd, str.c_str(), str.size());
	}

void child_process::read(std::string& str) 
	{
	int pos=0;
	char buffer[2];
	while((pos=::read(pipes[1].parent_fd, buffer, 1))>0) {
		if(buffer[0]=='\n') break;
		str+=buffer[0];
		}
	}

ssize_t child_process::read(void *buffer, size_t len) 
	{
	return ::read(pipes[1].parent_fd, buffer, len);
	}

ssize_t child_process::write(void *buffer, size_t len)
	{
	return ::write(pipes[0].parent_fd, buffer, len);
	}


void child_process::fork()
	{
	if(pipes.size()==0) standard_pipes();

   const char *cargs[args.size()+2];
	cargs[0]=name.c_str();
	for(unsigned int i=0; i<args.size(); ++i) {
		cargs[i+1]=args[i].c_str();
		}
	cargs[args.size()+1]=0;

	int pipefds[pipes.size()][2];
	for(unsigned int i=0; i<pipes.size(); ++i) {
		if(::pipe(pipefds[i])) {
			throw std::logic_error("modglue::child_process::fork: cannot create pipes");
			}
		}

	switch(pid_=::fork()) {
		case -1:
			throw std::logic_error("modglue::child_process::fork: cannot fork");
			break;
			
		case 0: { // we are the child
			// make the fds given by the parent override the stin/stdout/stderr fds
			for(unsigned int i=0; i<pipes.size(); ++i) {
				if(pipes[i].direction==fd_pair::child_input) {
					if(::dup2(pipefds[i][0], pipes[i].child_fd)<0)
						throw std::logic_error("modglue::child_process::fork: dup2 failed");
					::close(pipefds[i][1]);
					// FIXME: CORRECT?
					::close(pipefds[i][0]);
					}
				else {
					if(::dup2(pipefds[i][1], pipes[i].child_fd)<0)
						throw std::logic_error("modglue::child_process::fork: dup2 failed");
					::close(pipefds[i][0]);
					::close(pipefds[i][1]);
					}
				}
//			write(1,"{ \"stdin\" , input }\n{ \"stdout\" , output }\n{ \"stderr\" , output }\n",61);
			execvp(name.c_str(), const_cast<char *const *>(cargs));
			throw std::logic_error("modglue::child_process::fork: execlp failed");
			break;
			}
		default: // we are the parent
			for(unsigned int i=0; i<pipes.size(); ++i) {
				if(pipes[i].direction==fd_pair::child_input) {
					::close(pipefds[i][0]);
					pipes[i].parent_fd=pipefds[i][1];
					}
				else {
					::close(pipefds[i][1]);
					pipes[i].parent_fd=pipefds[i][0];
					}
				}
			break;
		}
	}

void child_process::close()
	{
	for(unsigned int i=0; i<pipes.size(); ++i) {
		if(pipes[i].parent_fd!=-1) {
			::close(pipes[i].parent_fd);
			pipes[i].parent_fd=-1;
			}
		}
	}

pid_t child_process::get_pid() const
	{
	return pid_;
	}


