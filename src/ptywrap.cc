/*

The misfeature is the lack of an end-to-end flush operation in Unix.
What stdio SHOULD do is to call one after every line of terminal
input, pipe-like programs should honour it and pass it on, and
output programs should honour it (and reflect it, if they are duplex).
All good 1960s technology.

*/

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cassert>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef __sun__
  #if USE_UTIL_H == 1
    #include <util.h>
  #else
    #include <pty.h>
  #endif
#endif
#include <errno.h>
#include <modglue/mid.hh>


/* Once Solaris has openpty(), this is going to be removed. */

#ifdef __sun__
#include <stdarg.h>
#include <sys/mman.h>
#include <stropts.h>

int openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp)
	{
	const char *slave;
	int mfd = -1, sfd = -1;
	
	*amaster = *aslave = -1;
	
	mfd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	if (mfd < 0)
		 goto err;
	
	if (grantpt(mfd) == -1 || unlockpt(mfd) == -1)
		 goto err;
	
	if ((slave = ptsname(mfd)) == NULL)
		 goto err;
	
	if ((sfd = open(slave, O_RDONLY | O_NOCTTY)) == -1)
		 goto err;
	
	if (ioctl(sfd, I_PUSH, "ptem") == -1)
		 goto err;
	
	if (amaster)
		 *amaster = mfd;
	if (aslave)
		 *aslave = sfd;
	
	if (winp)
		 ioctl(sfd, TIOCSWINSZ, winp);
	
	assert(name == NULL);
	assert(termp == NULL);
	
	return 0;
	
	err:
	if (sfd != -1)
		 close(sfd);
	close(mfd);
	return -1;
	}

#endif

// std::pair<int, std::string> open_pty_master(void)
// 	{
// 	unsigned char ptychar='p';
// 	struct stat statbuff;
// 	static char hexdigit[]="0123456789abcdef";
// 	int master_fd=0;
// 
// 	do {
// 		for(unsigned char num=0; num<16; ++num) {
// 			std::stringstream st;
// 			st << "/dev/pty" << ptychar << hexdigit[num] << std::ends;
// 			std::string ptyname=st.str();
// 			if(stat(ptyname.c_str(), &statbuff)<0)
// 				break;
// 			if((master_fd=open(ptyname.c_str(), O_RDWR))>=0) {
// 				std::string ttyname=ptyname;
// 				ttyname[5]='t';
// 				int tmp;
// 				if((tmp=open(ttyname.c_str(), O_RDWR))>=0) {
// 					close(tmp);
// 					return std::pair<int, std::string>(master_fd, ptyname);
// 					}
// 				else {
// 					close(master_fd); // some systems have incorrect permissions on some /dev/ttyp
// 					}
// 				}
// 			}
// 		++ptychar;
// 		} while(ptychar!='z');
// 	throw std::logic_error("cannot open pseudo-tty in master mode");
// 	return std::pair<int, std::string>(-1,"");
// 	}
// 
// int open_pty_slave(std::string ptyname)
// 	{
// 	int slave_fd;
// 	ptyname[5]='t';
// 	slave_fd=open(ptyname.c_str(), O_RDWR);
// 	if(slave_fd<0) {
// 		std::cerr << "slave open for " << ptyname << " failed: " << strerror(errno) << std::endl;
// 		}
// 	assert(slave_fd>=0);
// 
// //	struct termio temp_mode;
// //	if(ioctl(slave_fd, TCGETA, &temp_mode)<0)
// //		throw std::logic_error("cannot get tty mode");
// //
// //	temp_mode.c_iflag = 0;
// //	temp_mode.c_oflag &= ~OPOST;
// //	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO | XCASE);
// //	temp_mode.c_cflag &= ~(CSIZE | PARENB);
// //	temp_mode.c_cflag |= CS8;
// //	temp_mode.c_cc[VMIN]=1;
// //	temp_mode.c_cc[VTIME]=1;
// //	
// //	if(ioctl(slave_fd, TCSETA, &temp_mode)<0) 
// //		throw std::logic_error("cannot set tty mode to raw");
// 
// 	struct termios temp_mode;
// 	if(tcgetattr(slave_fd, &temp_mode)<0)
// 		throw std::logic_error("cannot get tty mode");
// 
// 	temp_mode.c_iflag = 0;
// 	temp_mode.c_oflag &= ~OPOST;
// 	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO); // | XCASE);
// 	temp_mode.c_cflag &= ~(CSIZE | PARENB);
// 	temp_mode.c_cflag |= CS8;
// 	temp_mode.c_cc[VMIN]=1;
// 	temp_mode.c_cc[VTIME]=1;
// 
// 	if(tcsetattr(slave_fd, TCSANOW, &temp_mode)<0)
// 		throw std::logic_error("cannot set tty mode to raw");
// 
// 
// 	return slave_fd;
// 	}



int open_pty_slave(int slave_fd)
	{
	struct termios temp_mode;
	if(tcgetattr(slave_fd, &temp_mode)<0)
		throw std::logic_error("cannot get tty mode");

	temp_mode.c_iflag = 0;
	temp_mode.c_oflag &= ~OPOST;
	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO); // | XCASE);
	temp_mode.c_cflag &= ~(CSIZE | PARENB);
	temp_mode.c_cflag |= CS8;
	temp_mode.c_cc[VMIN]=1;
	temp_mode.c_cc[VTIME]=1;

	if(tcsetattr(slave_fd, TCSANOW, &temp_mode)<0)
		throw std::logic_error("cannot set tty mode to raw");


	return slave_fd;
	}

int sig_chld_pipe[2];
pid_t childpid_;

void sig_kill(int signo)
	{
	// This does of course not catch SIGKILL, that will still leave
	// process adopted by init. But there is no way around that, apparently.
	kill(childpid_, SIGTERM);
	exit(-1);
	}

void sig_chld(int signo)
	{
	int status;
	pid_t childpid;
	
	if((childpid=waitpid(-1, &status, WNOHANG)) < 0) {
		throw std::logic_error("waitpid failed");
		}
	
	if(WIFEXITED(status)) {
		int child_val = WEXITSTATUS(status); 
		write(sig_chld_pipe[1],&child_val,1);
		} 
	else if(WIFSIGNALED(status)) {
		int child_sig = WTERMSIG(status);
		int buf[1];
		buf[0]=-1;
		write(sig_chld_pipe[1],buf,1);
		} 
	}

int main(int argc, char **argv)
	{
	if(argc==1) {
		std::cerr << "usage: ptywrap [program] [options] ...." << std::endl << std::endl;
		std::cerr << "Creates a set of pseudo tty devices for stdin/stdout/stderr of" << std::endl
			  << "the given program, and maps them to the corresponding pipes of" << std::endl
			  << "the ptywrap program, thereby fooling the program to think that" << std::endl
			  << "it is always connected to a pseudo tty." << std::endl << std::endl
			  << "example: isatty <in >out" << std::endl
			  << "         ptywrap isatty <in >out" << std::endl << std::endl
			  << "info: message identifier size = " << sizeof(modglue::mid) << std::endl;
		exit(1);
		}

	std::pair<int,int> p_stdin, p_stdout, p_stderr;
	openpty(&p_stdin.first, &p_stdin.second, 0, 0, 0);
	openpty(&p_stdout.first, &p_stdout.second, 0, 0, 0);
	openpty(&p_stderr.first, &p_stderr.second, 0, 0, 0);

	if(pipe(sig_chld_pipe)!=0)
		throw std::logic_error("cannot create sig_chld_pipe");
	struct sigaction act;
	act.sa_handler = sig_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	if(sigaction(SIGCHLD, &act, NULL) < 0) {
		throw std::logic_error("sigaction failed");
		}
	act.sa_handler = sig_kill;
	sigemptyset(&act.sa_mask);
	if(sigaction(SIGTERM, &act, NULL) < 0) {
		throw std::logic_error("sigaction failed");
		}
	act.sa_handler = sig_kill;
	sigemptyset(&act.sa_mask);
	if(sigaction(SIGINT, &act, NULL) < 0) {
		throw std::logic_error("sigaction failed");
		}
	
	switch(childpid_=fork()) {
		case -1: 
			throw std::logic_error("cannot fork");
			break;
			
		case 0: {
			// close parent side of the pipe
			close(p_stdin.first);
			close(p_stdout.first);
			close(p_stderr.first);
			// dup the default streams and then close the originals
			open_pty_slave(p_stdin.second);
			open_pty_slave(p_stdout.second);
			open_pty_slave(p_stderr.second);
			dup2(p_stdin.second,0);
			dup2(p_stdout.second,1);
			dup2(p_stderr.second,2);
			close(p_stdin.second);
			close(p_stdout.second);
			close(p_stderr.second);
			write(sig_chld_pipe[1], "G", 1);
			// exec the actual program
			execvp(argv[1],&(argv[1]));
			}
		default: 
			break;
		}


	fd_set rfds;
	fd_set efds;
	fcntl(0, F_SETFL, O_NONBLOCK);
	fcntl(p_stdout.first, F_SETFL, O_NONBLOCK);
	fcntl(p_stderr.first, F_SETFL, O_NONBLOCK);

	int retcode=0;
	char buffer[1024];
	read(sig_chld_pipe[0], buffer, 1); // wait until child is ready
	do {
		FD_ZERO(&rfds);
		FD_ZERO(&efds);

		FD_SET(sig_chld_pipe[0],&rfds);
		FD_SET(0, &rfds);
		FD_SET(0, &efds);
		FD_SET(p_stdout.first, &rfds);
		FD_SET(p_stdout.first, &efds);
		FD_SET(p_stderr.first, &rfds);
		FD_SET(p_stderr.first, &efds);

		int retval = select(std::max(sig_chld_pipe[0],
										std::max(p_stdout.first,p_stderr.first))+1, &rfds, NULL, &efds, NULL);

		if(FD_ISSET(sig_chld_pipe[0], &rfds)) {
			read(sig_chld_pipe[0],buffer,1);
			retcode=buffer[0];
			break;
			}
		if(FD_ISSET(0,&rfds)) {
			int length=read(0,buffer,1023);
			if(length>0) {
				// FIXME: strip identifier from input channel and remember it.
				if(write(p_stdin.first,buffer,length)==-1)
					break;
				}
			}
		if(FD_ISSET(p_stdout.first,&rfds)) {
			int length=read(p_stdout.first,buffer,1023);
			if(length>0) {
				// FIXME: add identifier
				if(write(1,buffer,length)==-1)
					break;
				}
			else break;
			}
		if(FD_ISSET(p_stderr.first,&rfds)) {
			int length=read(p_stderr.first,buffer,1023);
			if(length>0) {
				// FIXME: add identifier
				if(write(2,buffer,length)==-1)
					break;
				}
			else break;
			}
		} while(1==1);
	if(retcode!=-1) {
		int length=0;
		while((length=read(p_stdout.first,buffer,1023))>0) {
			// FIXME: add identifier
			write(1,buffer,length);
			}
		while((length=read(p_stderr.first,buffer,1023))>0) {
			// FIXME: add identifier
			write(2,buffer,length);
			}
		}

	exit(retcode);
	}

