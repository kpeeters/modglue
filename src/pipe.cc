/* 

	Modglue pipes.
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
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <algorithm>
#include <modglue/pipe.hh>
#include <cassert>

using namespace modglue;


modglue::pipe::pipe(const std::string& name)
	: process(0), name_(name), fd_(-1), fd_external_(-1), first_read_after_select_(false), 
	  broken_(true), unix_style_pipe_(false), make_blocking_(false)
	{
	}

modglue::pipe::~pipe()
	{
	close();
	close_external();
	}

std::string modglue::pipe::name(void) const
	{
	return name_;
	}

int modglue::pipe::fd(void) const
	{
	return fd_;
	}

int modglue::pipe::fd_external(void) const
	{
	return fd_external_;
	}

void modglue::pipe::set_unix_style(void)
	{
	unix_style_pipe_=true;
	}

void modglue::pipe::set_blocking(void)
	{
	make_blocking_=true;
	}

bool modglue::pipe::is_broken(void) const
	{
	return broken_;
	}

void modglue::pipe::open(int fd, int fd_external)
	{
	if(fd_!=-1 || fd_external_!=-1) {
		throw std::logic_error("pipe already open");
		}
	
	if(fd==-1 && fd_external==-1) {
		int socks[2];
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, socks)!=0) {
			throw std::logic_error("cannot open sockets");
			}
		fd_=socks[0];
		fd_external_=socks[1];
		}
	else {
		fd_=fd;
		fd_external_=fd_external;
		}
	broken_=false;
	do_fcntl_();
	}

void modglue::pipe::close(void)
	{
	if(fd_!=-1) {
		::close(fd_);
		fd_=-1;
		broken_=true;
		}
	}

void modglue::pipe::close_external(void)
	{
	if(fd_external_!=-1) {
		::close(fd_external_);
		fd_external_=-1;
		}
	}

void modglue::pipe::first_read_after_select()
	{
	first_read_after_select_=true;
	}

void modglue::pipe::send_with_ack(const char *txt, int length)
	{
//	FILE *fp=fopen("/tmp/send_with_ack","a");
	if(/* fd_!=-1 && */ !broken_) {
//		fprintf(fp, "%s", txt);
		if(send_blocking_(txt, length)==false) {
			throw std::logic_error("modglue::pipe::send_with_ack: error writing data, " 
									+ std::string(strerror(errno)));
			}
		}
//		  if(!unix_style_pipe_) {
//			  std::string result;
//			  if(read_blocking_(result, 5)==false || result!="OK") {
//				  cerr << "error reading data on " << fd_ 
//						 << ", error " << errno << ", " << strerror(errno) << endl;
//				  broken_=true;
//				  }
//			  cerr << "send ack received for " << fd_ << endl;
//			  }

//	fclose(fp);
	}

int modglue::pipe::read_with_ack(char *txt, int maxlen)
	{
	int ret=read_nonblocking_(txt, maxlen);
//	std::cerr << "read with ack on " <<  name_ << " " << ret;
//	  if(ret<0) 
//		  std::cerr << " " << strerror(errno);
//	  std::cerr << std::endl;
	// We cannot set the pipe to broken here, because somebody may
	// have called read without coming from a select (this is typically
	// what happens when doing 'while(p>>var)' inside a callback).
	// A pipe is set to broken when the associated process dies, or
	// when the acknowledgement fails to come back (the latter has not
	// yet been implemented, have to think first).
	if(ret==0 && first_read_after_select_) {
		broken_=true;
		}
	first_read_after_select_=false;

//	  if(!unix_style_pipe_) 
//		  if(ret>0) 
//			  send_blocking_("OK", 2);

	if(ret==0) return -1;
	return ret;
	}

int modglue::pipe::read_nonblocking_(char *data, int maxlen)
	{
	if(fd_==-1 || broken_) {
		return -1;
		}

	if(1==1 /* unix_style_pipe_*/ ) {
		int ret;
		do {
			ret=read(fd_, data, maxlen);
			} while(ret<0 && errno==EINTR);
		return ret;
		}
	else {
		// CHECKED: recmsg is not the reason for dropped bytes
		char cbuf[CMSG_SPACE(sizeof(int))]; 
		struct msghdr mh = {0};
		struct iovec iov; 
		
		mh.msg_iov = &iov; 
		mh.msg_iovlen = 1;
		iov.iov_base = data; 
		iov.iov_len = maxlen;
		mh.msg_control = cbuf;
		mh.msg_controllen = sizeof cbuf; 
		
		int ret;
		do {
			ret = recvmsg(fd_, &mh, 0);
			} while(ret<0 && errno==EINTR);
		assert(!(ret<0 && errno!=EAGAIN));
		return ret;
		}
	}

int modglue::pipe::send_blocking_(const char *data, int length) 
	{
	assert(data);
	assert(length>0);

	if(fd_==-1 || broken_)
		return false;

	if(1==1 /*unix_style_pipe_*/) {
		if(length>0) {
			// write fails on large blocks.
			unsigned int len=0;
			while(len<length) {
				unsigned int chunk=std::min((unsigned int)512, length-len);
				int ret=write(fd_, &(data[len]), chunk);
				if(ret<0 && errno!=EAGAIN)
					return -1;
//				assert(!(ret<0 && errno!=EAGAIN));
//				  if((ret<0 && errno!=EAGAIN)) {
//					  std::cout << "error1" << std::endl;
//					  assert(1==0);
//					  }
				if(ret>0) 
					len+=ret;
				}
			}
		return length;
		}
	else {
		// CHECKED: sendmsg is not the reason for dropped bytes
		struct iovec iov[1];
		struct msghdr msg;
		struct {
				struct cmsghdr cm;
				int  fd;
		} cmsg;
		
		memset( &msg, 0, sizeof(msg) );
		memset( &cmsg, 0, sizeof(cmsg) );
	
		msg.msg_control=0;
		msg.msg_controllen=0;
		msg.msg_iov  = iov;  /* msg header */
		msg.msg_iovlen  = 1;
		msg.msg_name  = 0;
		
		iov[0].iov_base = (void *)data;
		iov[0].iov_len = length;
		
		int bytes_sent;
		if((bytes_sent=sendmsg(fd_,&msg,0))==-1) {
			throw std::logic_error("modglue::pipe::send_blocking_: sendmsg error "
									+ std::string(strerror(errno)));
			}
		assert((unsigned int)bytes_sent==length);
//		std::cerr << "send " << bytes_sent << std::endl;
		return bytes_sent;
		}
	}


/* Pipebuf */

pipebuf::pipebuf(pipe *p)
	: pipe_(p)
	{
	setg(i_buf + putbackarea, i_buf + putbackarea, i_buf + putbackarea);
	setp(o_buf, o_buf + bufsiz - 1);
	}

pipebuf::~pipebuf()
	{
	sync();
	}

int pipebuf::underflow(void)
	{
	//still before end of buffer?
	if (gptr() < egptr()) {
		return *gptr(); 
		}

	int numPutback = gptr() - eback();
	if (numPutback > putbackarea) {
		numPutback = putbackarea;
		}
	
	//copy characters into put back area.
	memcpy(i_buf + (putbackarea - numPutback), gptr() - numPutback, numPutback);    

	//read new characters
	int num;
	// CHECKED: the total 'num' gives the correct number of characters, so we
	// do read everything in (when things come from a unix pipe).
	num = pipe_->read_with_ack(i_buf+putbackarea, bufsiz-putbackarea);
	if (num <= 0)
		return EOF;
//	std::cerr << "read " << num << std::endl;
	
	// setg(eback, gptr, egptr)
	setg(i_buf + (putbackarea - numPutback), 
		  i_buf + putbackarea, 
		  i_buf + putbackarea + num);
	
	//return next character
	return *gptr();
	}	
	
int pipebuf::sync(void)
	{
	if(flushbuffer()==EOF) 
		return -1;
	else 
		return 0;
	}

int pipebuf::overflow(int ch)
  {
  if (ch != EOF) {
	  *pptr() = ch;
	  pbump(1);
	  }
  if(flushbuffer()==EOF) return EOF;
  else                   return ch;
  }

int pipebuf::flushbuffer(void)
	{
	int length = pptr() - pbase();
	if (length > 0) {
		pipe_->send_with_ack(o_buf, length);
		pbump(-length);
		}
	return length;
	}
		

/* Output pipe */

modglue::opipe::opipe(const std::string& name)
	: pipe(name), std::ostream(new pipebuf(this))
	{
	}

void modglue::opipe::do_fcntl_(void) const
	{
	// FIXME: the problem is that this is not inherited. The 
	// blocking flag does something funny on both sides. You have
	// to first fork, and then set the fd blocking, before execing
	// the child process.
	//
	// Done here, it does not do anything.
//	if(make_blocking_) 
//		fcntl(fd_, F_SETFL, 0);
	}


/* Input pipe */

modglue::ipipe::ipipe(const std::string& name)
	: pipe(name), std::istream(new pipebuf(this))
	{
	}

void modglue::ipipe::do_fcntl_(void) const
	{
	// FIXME: MAIN MYSTERY: setting fd 0 to NONBLOCK leads to
	// loss of _output_ on std::cout and std::cerr. I don't get it.
	// Can we live without this NONBLOCK?
	if(fd_!=0)
		fcntl(fd_, F_SETFL, O_NONBLOCK);
	}

