/* 

	Extended process class.
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

#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sstream>
#include <fstream>
#include <modglue/process.hh>
#include <modglue/ext_process.hh>
#include <modglue/main.hh>
#include <stdexcept>
#include <cassert>

using namespace modglue;

ext_process::ext_process(const std::string& name)
	: name_(name), modglue_binary_(false), pid_(0), paused_(false), exit_code_(0), pipes_done_(false)
	{
	}

ext_process::ext_process(const std::string& name, const std::vector<std::string>& args)
	: name_(name), args_(args), modglue_binary_(false), pid_(0), paused_(false), exit_code_(0), 
	  pipes_done_(false)
	{
	}

ext_process::ext_process(const ext_process& other)
	: name_(other.name_), args_(other.args_), modglue_binary_(false), pid_(0), paused_(false), 
	exit_code_(0), pipes_done_(false)
	{
	}

ext_process::~ext_process()
	{
	for(unsigned int i=0; i<input_pipes_.size(); ++i) {
		delete input_pipes_[i];
		}
	for(unsigned int i=0; i<output_pipes_.size(); ++i) {
		delete output_pipes_[i];
		}
	}

std::string ext_process::name() const
	{
	return name_;
	}

pid_t ext_process::get_pid() const
	{
	return pid_;
	}

int ext_process::exit_code() const
	{
	return exit_code_;
	}

const std::vector<std::string>& ext_process::output() const
	{
	return output_;
	}

const std::vector<ipipe *>& ext_process::input_pipes() const
	{
	return input_pipes_;
	}

const std::vector<opipe *>& ext_process::output_pipes() const
	{
	return output_pipes_;
	}

ipipe* ext_process::input_pipe(const std::string& name) const
	{
	for(unsigned int i=0; i<input_pipes_.size(); ++i) {
		if(input_pipes_[i]->name()==name)
			return input_pipes_[i];
		}
	throw std::logic_error("ext_process::input_pipe: pipe "+name+" not known");
	}

opipe* ext_process::output_pipe(const std::string& name) const
	{
	for(unsigned int i=0; i<output_pipes_.size(); ++i) {
		if(output_pipes_[i]->name()==name)
			return output_pipes_[i];
		}
	throw std::logic_error("ext_process::output_pipe: pipe "+name+" not known");
	}

ext_process& ext_process::operator<<(const std::string& st)
	{
	args_.push_back(st);
	return (*this);
	}

void ext_process::fork()
	{
	assert(pid_==0);
	if(!pipes_done_) 
		setup_pipes();
	open_pipes_();
//	std::cerr << "pipes open for " << name_ << std::endl;

	std::vector<std::string> realargs=args_;
   // add '--pipe' arguments for modglue binaries
	if(modglue_binary_) { 
		for(unsigned int i=0; i<input_pipes_.size(); ++i) {
			std::stringstream tmp;
			tmp << "--pipe=" << input_pipes_[i]->name() << ",";
			tmp << input_pipes_[i]->fd_external() << std::ends;
			realargs.push_back(tmp.str());
			}
		for(unsigned int i=0; i<output_pipes_.size(); ++i) {
			std::stringstream tmp;
			tmp << "--pipe=" << output_pipes_[i]->name() << ",";
			tmp << output_pipes_[i]->fd_external() << std::ends;
			realargs.push_back(tmp.str());
			}
		}

	// convert args to something that execvp groks
   const char *cargs[realargs.size()+2];
	cargs[0]=name_.c_str();
	for(unsigned int i=0; i<realargs.size(); ++i) {
		cargs[i+1]=realargs[i].c_str();
		}
	cargs[realargs.size()+1]=0;

	paused_=false;
	std::cerr << std::flush;
	std::cout << std::flush;
	switch(pid_=::fork()) {
		case -1:
			throw std::logic_error("modglue::child_process::fork: cannot fork");
			break;
			
		case 0: // we are the child
			close_parentside_();
			if(!modglue_binary_)
				dup_unix_pipes_();
			// FIXME: cerr gets completely fucked up when we set cin to be
			// non-blocking, but that's only true when we do that in the parent. 
			// Still, the child should not block, so we just do this 'the hard way'.

			// FIXME: sometimes we do not want this blocking! For instance when
			// we want to use this like libexecstream.

			fcntl(0, F_SETFL, O_NONBLOCK);

/*			for(unsigned long i=0; i<100000L; ++i)
				fprintf(stdout, "long string which should not get eaten %d\n", i);
			fflush(stdout);
			fflush(stderr);
			FILE *fp;
			if(fclose(stdout)!=0)
				fp=fopen("/tmp/onefailed", "w");
			if(fclose(stdin)!=0)
				fp=fopen("/tmp/twofailed", "w");
			sleep(10);
			exit(1); */
			execvp(name_.c_str(), const_cast<char *const *>(cargs));
			throw std::logic_error("modglue::ext_process::fork: execvp failed");
			break;
			
		default: // we are the parent
			close_childside_();
			break;
		}
	}

void ext_process::close_childside_(void)
	{
	for(unsigned int i=0; i<output_pipes_.size(); ++i)
		output_pipes_[i]->close_external();
	for(unsigned int i=0; i<input_pipes_.size(); ++i)
		input_pipes_[i]->close_external();
	}

void ext_process::close_parentside_(void)
	{
	for(unsigned int i=0; i<output_pipes_.size(); ++i)
		output_pipes_[i]->close();
	for(unsigned int i=0; i<input_pipes_.size(); ++i)
		input_pipes_[i]->close();
	}


void ext_process::terminate(int exit_code)
	{
	for(unsigned int i=0; i<input_pipes_.size(); ++i)
		input_pipes_[i]->close();
	for(unsigned int i=0; i<output_pipes_.size(); ++i)
		output_pipes_[i]->close();

	exit_code_=exit_code;
	if(pid_!=0) {
		if(kill(pid_, 0))
			kill(pid_, SIGTERM);
		pid_=0;
		}
	}

void ext_process::pause()
	{
	if(pid_!=0 && !paused_) {
		if(kill(pid_, 0))
			kill(pid_, SIGSTOP);
		paused_=true;
		}
	}

void ext_process::restart()
	{
	if(pid_!=0 && paused_) {
		if(kill(pid_, 0))
			kill(pid_, SIGCONT);
		paused_=false;
		}
	}

bool ext_process::receive_output_(ipipe& p)
	{
//	std::cerr << "received output" << std::endl;
	for(unsigned int i=0; i<input_pipes_.size(); ++i) {
		if(&p==input_pipes_[i]) {
			std::string buffer;
			while(getline(p, buffer)) {
				output_[i]=output_[i]+buffer;
				}
			break;
			}
		}
	p.clear();
	return true;
	}

void ext_process::determine_path_(void)
	{
	full_path_="";
	if(name_.find("/")!=-1) { // full path given
		struct stat mystat;
		if(stat(name_.c_str(), &mystat)==0) {
			full_path_=name_;
			}
		}
	else {
		// FIXME: use the which library
		// RESTORE!!!
		child_process pr_which("/usr/bin/which");
		pr_which << name_;
		pr_which.call("", full_path_);
//		std::cout << full_path_ << std::endl;
//		full_path_="/usr/bin/ls";

		if(full_path_.find("no "+name_+" in")==-1) {
			char realpth[MAXPATHLEN];
			realpath(full_path_.substr(0,full_path_.size()-1).c_str(), realpth);
			full_path_=realpth;
			}
		else {
			full_path_="";
			} 
		}
	if(full_path_=="") {
		std::string msg("modglue::ext_process::determine_path_: executable ");
		msg+=name_+" not found";
		throw std::logic_error(msg);
		}
	}

void ext_process::determine_binary_type_(void)
	{
	assert(full_path_!="");

	std::ifstream exec(full_path_.c_str());
	char c;
	const char comp[]="MODGLUE_1.0_BINARO";
	modglue_binary_=false;
	int hit=0;
	while(exec.get(c)) {
		if(c==comp[hit]) {
			++hit;
			if(hit==strlen(comp)) {
				modglue_binary_=true;
				break;
				}
			}
		else hit=0;
		}
	}

void ext_process::setup_pipes_from_string_(const std::string& desc)
	{
	char pipename[100];
	char iotype[100];
	std::istringstream str(desc.c_str());
	std::string line;
	while(getline(str, line)) {
		if(sscanf(line.c_str(), "{ %99s , %99s }\n", pipename, iotype)==2) {
			std::string s1=&(pipename[1]);
			s1=s1.substr(0,s1.size()-1);
			// direction has to be flipped since we are at the other side
//			std::cerr << "pipe " << pipename << std::endl;
			if(strcmp(iotype,"output")==0) {
				input_pipes_.push_back(new ipipe(s1));
				}
			else if(strcmp(iotype, "input")==0){
				output_pipes_.push_back(new opipe(s1));
				}
			else throw std::logic_error("modglue::ext_process::setup_pipes_from_string_: parse error");
			}
		}
	}

void ext_process::open_pipes_(void)
	{
	for(unsigned int i=0; i<input_pipes_.size(); ++i) {
		input_pipes_[i]->open();
		}
	for(unsigned int i=0; i<output_pipes_.size(); ++i) {
		output_pipes_[i]->open();
		}
	}

void ext_process::dup_unix_pipes_(void)
	{
	assert(!modglue_binary_);

	for(unsigned int i=0; i<output_pipes_.size(); ++i) {
		if(output_pipes_[i]->name()=="stdin") {
			if(::dup2(output_pipes_[i]->fd_external(), 0)<0)
				throw std::logic_error("modglue::ext_process::fork: dup2 failed");
			::close(output_pipes_[i]->fd_external());  // is dupped, so no longer needed
			::close(output_pipes_[i]->fd()); // parent side
			}
		}
	for(unsigned int i=0; i<input_pipes_.size(); ++i) {
		if(input_pipes_[i]->name()=="stdout") {
			if(::dup2(input_pipes_[i]->fd_external(), 1)<0)
				throw std::logic_error("modglue::child_process::fork: dup2 failed");
			::close(input_pipes_[i]->fd_external());
			::close(input_pipes_[i]->fd()); // parent side
			}
		else if(input_pipes_[i]->name()=="stderr") {
			if(::dup2(input_pipes_[i]->fd_external(), 2)<0)
				throw std::logic_error("modglue::child_process::fork: dup2 failed");
			::close(input_pipes_[i]->fd_external());
			::close(input_pipes_[i]->fd()); // parent side
			}
		}
	}

void ext_process::setup_pipes(void)
	{
	assert(output_pipes_.size()==0);
	assert(input_pipes_.size()==0);

	determine_path_();
	determine_binary_type_();
//	std::cerr << "path: |" << full_path_ << "|" << std::endl
//		  << "type: " << (modglue_binary_?"modglue":"unix") << std::endl;
	
	if(modglue_binary_) {
		output_pipes_.push_back(new opipe("startup_args"));
		child_process pr_tst(full_path_);
		pr_tst.args.push_back("--list-pipes");
		std::string result;
		pr_tst.call("", result);
//		std::cout << "|" << result << "|" << std::endl;
		setup_pipes_from_string_(result);
		if(input_pipes_.size()==0 && output_pipes_.size()==1)
			throw std::logic_error("moglue::ext_process::setup_pipes: process reports no pipes.");
		}
	else {
		// direction has to be flipped since we are at the other side
		output_pipes_.push_back(new opipe("startup_args"));
		output_pipes_.push_back(new opipe("stdin"));
		input_pipes_.push_back(new ipipe("stdout"));
		input_pipes_.push_back(new ipipe("stderr"));
		for(unsigned int i=0; i<input_pipes_.size(); ++i)
			input_pipes_[i]->set_unix_style();
		for(unsigned int i=0; i<output_pipes_.size(); ++i)
			input_pipes_[i]->set_unix_style();
		}
	pipes_done_=true;
	}
