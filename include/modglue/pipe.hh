
/* 

   $Id: pipe.hh,v 1.16 2006/03/10 14:23:23 kp229 Exp $

	Pipe class (generalises Unix standard pipes and the pipe() call)
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

#ifndef pipe_hh_
#define pipe_hh_

#include <string>
#include <string.h>
#include <sigc++/sigc++.h>
#include <iostream>

namespace modglue { 

 class pipebuf; 
 class ext_process;

 class pipe {
	 public:
		 virtual ~pipe();
		 std::string   name(void) const;
		 int      fd(void) const;
		 int      fd_external(void) const;
		 bool     is_broken(void) const;  // ie. is the other end closed?
		 void     open(int fd=-1, int fd_external=-1);
		 void     set_unix_style(void);
		 void     set_blocking();
		 void     close(void);
		 void     close_external(void);

		 void     first_read_after_select();
		 
		 void     send_with_ack(const char *, int length);
		 int      read_with_ack(char *, int maxlen);

		 ext_process *process;
	 protected:
		 pipe(const std::string& name);
		 virtual void do_fcntl_(void) const=0;
		 int          read_nonblocking_(char *data, int maxlen);    // returns as 'read'
		 int          send_blocking_(const char *data, int len);    // returns as 'write'
		 int          fd_, fd_external_;
		 bool         broken_;
		 bool         first_read_after_select_;
		 bool         unix_style_pipe_; // uni-directional, no acknowledgement
		 bool         make_blocking_;
	 private:
		 std::string  name_;
 };

 class pipebuf : public std::streambuf {
	 public:
		 pipebuf(pipe *);
		 ~pipebuf();

		 void     put_buffer(void);
		 void     put_char(int);
	 protected:
		 int      overflow(int=EOF);
		 int      underflow();
		 int      sync();
		 int      flushbuffer(void);
	 private:
		 pipe    *pipe_;
		 static const int bufsiz = 1024;
		 static const int putbackarea = 8;
		 char     i_buf[bufsiz]; // input buffer;
		 char     o_buf[bufsiz]; // output buffer
 };

 class opipe : public pipe, public std::ostream {
	 public:
		 opipe(const std::string& name);
	 private:
		 virtual void do_fcntl_(void) const;
 };

 class ipipe : /* public sigc::object, */ public pipe, public std::istream {
	 public:
		 ipipe(const std::string& name);

		 sigc::signal1<bool, ipipe&> receiver;
	 private:
		 virtual void do_fcntl_(void) const;
 };

	// sockets are always bidirectional... How is this going to fit in?

 //class server_isocket : public ipipe {
 //	 public:
 //};
 //
 //class server_osocket : public ipipe {
 //	 public:
 //};
 //
 //class client_isocket : public ipipe {
 //	 public:
 //};
 //
 //class client_osocket : public opipe {
 //	 public:
 //};

};

#endif
