
#include <string>
#include <iostream.h>
#include <strstream.h>
#include <stdexcept>
#include <algo.h>

#include <pair.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <modglue/mid.hh>


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
		throw logic_error("waitpid failed");
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
		cerr << "usage: modwrap [program] [options] ...." << endl << endl;
		cerr << "Creates a set of pseudo tty devices for stdin/stdout/stderr of" << endl
			  << "the given program, and maps them to the corresponding pipes of" << endl
			  << "the ptywrap program, thereby fooling the program to think that" << endl
			  << "it is always connected to a pseudo tty." << endl << endl
			  << "example: isatty <in >out" << endl
			  << "         ptywrap isatty <in >out" << endl << endl
			  << "info: message identifier size = " << sizeof(modglue::mid) << endl;
		exit(1);
		}

	pair<int,string> p_stdin =open_pty_master();
	pair<int,string> p_stdout=open_pty_master();
	pair<int,string> p_stderr=open_pty_master();

	if(pipe(sig_chld_pipe)!=0)
		throw logic_error("cannot create sig_chld_pipe");
	struct sigaction act;
	act.sa_handler = sig_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
	if(sigaction(SIGCHLD, &act, NULL) < 0) {
		throw logic_error("sigaction failed");
		}
	act.sa_handler = sig_kill;
	sigemptyset(&act.sa_mask);
	if(sigaction(SIGTERM, &act, NULL) < 0) {
		throw logic_error("sigaction failed");
		}
	
	switch(childpid_=fork()) {
		case -1: 
			throw logic_error("cannot fork");
			break;
			
		case 0: {
			dup2(open_pty_slave(p_stdin.second), 0);
			dup2(open_pty_slave(p_stdout.second),1);
			dup2(open_pty_slave(p_stderr.second),2);
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

		int retval = select(max(sig_chld_pipe[0],
										max(p_stdout.first,p_stderr.first))+1, &rfds, NULL, &efds, NULL);

		if(FD_ISSET(sig_chld_pipe[0], &rfds)) {
			read(sig_chld_pipe[0],buffer,1);
			retcode=buffer[0];
			break;
			}
		if(FD_ISSET(0,&rfds)) {
			int length=read(0,buffer,1023);
			if(length>0) {
				// FIXME: strip identifier from input channel and remember it.
				write(p_stdin.first,buffer,length);
				}
			}
		if(FD_ISSET(p_stdout.first,&rfds)) {
			int length=read(p_stdout.first,buffer,1023);
			if(length>0) {
				// FIXME: add identifier
				write(1,buffer,length);
				}
			}
		if(FD_ISSET(p_stderr.first,&rfds)) {
			int length=read(p_stderr.first,buffer,1023);
			if(length>0) {
				// FIXME: add identifier
				write(2,buffer,length);
				}
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

