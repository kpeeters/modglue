/* 

   $Id: prompt.cc,v 1.29 2006/05/31 19:26:00 peekas Exp $

	Command-line editing front-end for arbitrary programs.
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

/*

   We use some non-standard escape codes:

       ESC $ [text] $    : fill the edit buffer with [text]


 */

#include <string>
#include <fstream>

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

#include <modglue/main.hh>
#include <modglue/process.hh>
#include <modglue/pipe.hh>

using namespace std;

struct termios saved_attributes, raw_attributes;

modglue::main *cmm;
modglue::opipe *t_stdin;
modglue::ipipe s_stdin("s_stdin");
modglue::opipe s_stdout("s_stdout");
modglue::opipe s_stderr("s_stderr");
pid_t thepid;
bool go_to_sleep=false;
enum state_t { initial, escape_read, movement_escape_read, eat_one_char } state;
string combine, last_output_line;
vector<string> history;
unsigned int hispos=0;
bool moving=false;
unsigned int pos=0;


void sigc_handler(int num)
	{
	kill(thepid, SIGINT);
   signal(SIGINT,sigc_handler);
	}

void raw_terminal(void);
void buffered_terminal(void);
void redraw_line(const string& str, unsigned int curpos, unsigned int nextpos);

void sigstp_handler(int num)
	{ 
	signal(SIGTSTP,&sigstp_handler);
	//go_to_sleep=true;
   tcsetattr(0, TCSANOW, &saved_attributes);
	kill(getpid(),SIGSTOP);
	}

void sigcont_handler(int num)
	{
	signal(SIGCONT,&sigcont_handler);
   tcsetattr(0, TCSANOW, &raw_attributes);
	fcntl(0, F_SETFL, O_NONBLOCK);
	s_stdout << last_output_line << std::flush;
	redraw_line(combine, 0, pos);
	}

void buffered_terminal(void)
	{
	tcsetattr(0, TCSANOW, &saved_attributes);
	}

void raw_terminal(void)
	{
   tcgetattr(0, &saved_attributes);
   atexit(buffered_terminal);
   tcgetattr(0, &raw_attributes);
   raw_attributes.c_lflag &= ~(ICANON|ECHO);
   tcsetattr(0, TCSANOW, &raw_attributes);

	signal(SIGTSTP,&sigstp_handler);
	signal(SIGCONT,&sigcont_handler);
	}

// void save_cursor()
// 	{
// 	cout << "\033[s";
// 	}
// 
// void restore_cursor()
// 	{
// 	cout << "\033[u";
// 	}

void erase_line()
	{
	s_stdout << "\033[K";
	}

void move_left(unsigned int num=1)
	{
	if(num>0)
		s_stdout << "\033[" << num << "D" << flush;
	}

void move_right(unsigned int num=1)
	{
	if(num>0)
		s_stdout << "\033[" << num << "C" << flush;
	}

void redraw_line(const string& str, unsigned int curpos, unsigned int nextpos)
	{
	move_left(curpos);
	erase_line();
	s_stdout << str << flush;
	move_left(str.size()-nextpos);
	}

bool died(modglue::ext_process& pr)
	{
//	s_stderr << "********process " << pr.name() << " died" << endl;
	return true;
	}

void process_stdin(const char *buffer, int len)
	{
	for(int curchar=0; curchar<len; ++curchar) {
		switch(state) {
			case eat_one_char:
				state=initial;
				break;
			case initial:
				switch(buffer[curchar]) {
					case 1:
						redraw_line(combine, pos, 0);
						pos=0;
						break;
					case 5:
						redraw_line(combine, pos, combine.size());
						pos=combine.size();
						break;
					case 11:
						if(pos<combine.size()) {
							combine=combine.substr(0,pos);
							redraw_line(combine, pos, pos);
							}
						break;
					case 12:
						redraw_line(combine, pos, pos);
						break;
					case 127:
					case 8:
						if(pos>0) {
							combine=(pos>1?combine.substr(0,pos-1):"")+
								(pos<combine.size()-1?combine.substr(pos):"");
							redraw_line(combine, pos, pos-1);
							--pos;
							break;
							}
						break;
					case '\033':
						state=escape_read;
						// The following line was commented out at some point. Do not do that.
						// It disables cursors entirely. What goes wrong?
						if(curchar==len-1) --curchar; // no more characters, so this was a real escape.
						break;
					case '\n':
						s_stdout << endl;
						history.pop_back();
						history.push_back(combine);
						history.push_back("");
						hispos=history.size()-1;
						(*t_stdin) << combine << endl;
						pos=0;
						combine="";
						break;
					default:
						if(moving) {
							(*t_stdin) << buffer[curchar] << flush;
							}
						else {
							if(pos<combine.size()) {
								combine=combine.substr(0,pos)+buffer[curchar]+combine.substr(pos);
								redraw_line(combine, pos, pos+1);
								++pos;
								}
							else {
								s_stdout << buffer[curchar] << flush;
								++pos;
								combine+=buffer[curchar];
								}
							}
					}
				break;
			case escape_read:
//				s_stdout << "after escape:" << (int)buffer[curchar] << endl;
				switch(buffer[curchar]) {
					case '\033':
						(*t_stdin) << "\\esc" << endl;
						state=initial;
						break;
					case 91:
						state=movement_escape_read;
						break;
					default:
						state=initial;
					}
				break;
			case movement_escape_read:
//				s_stdout << "after movement_escape:" << (int)buffer[curchar] << endl;
				switch(buffer[curchar]) {
					case '5':
						state=eat_one_char;
						if(moving) {
							(*t_stdin) << "\\pageup" << endl;
							}
						break;
					case '6':
						state=eat_one_char;
						if(moving) {
							(*t_stdin) << "\\pagedown" << endl;
							}
						break;
					case 'A':
						if(moving) {
							(*t_stdin) << "\\up" << endl;
							}
						else {
							if(hispos>0) {
								if(hispos+1==history.size()) {
									history.pop_back();
									history.push_back(combine);
									}
								combine=history[--hispos];
								redraw_line(combine, pos, combine.size());
								pos=combine.size();												
								}
							}
						state=initial;
						break;
					case 'B':
						if(moving) {
							(*t_stdin) << "\\down" << endl;
							}
						else {
							if(hispos+1<history.size()) {
								combine=history[++hispos];
								redraw_line(combine, pos, combine.size());
								pos=combine.size();
								}
							}
						state=initial;
						break;
					case 'C':
						if(moving) {
							(*t_stdin) << "\\right" << endl;
							}
						else {
							if(pos<combine.size()) {
								++pos;
								move_right();
								}
							}
						state=initial;
						break;
					case 'D': 
						if(moving) {
							(*t_stdin) << "\\left" << endl;
							}
						else {
							if(pos>0) {
								--pos;
								move_left();
								}
							}
						state=initial;
						break;
					default:
						state=initial;
						break;
					}
				break;
			}
		}
	}

bool receive_stdout(modglue::ipipe& p)
	{
	char buffer[1024];
	int len;
	enum write_t { w_normal, w_fill };
	static write_t wmode=w_normal;

	do {
		p.read(buffer,1023);
		len=p.gcount();
		if(len>0) {
			buffer[len]=0;
			if(len>=3) {
				if(strncmp("\033[u", &(buffer[len-3]), 3)==0)
					moving=true;
				}
			for(unsigned int i=0; i<len; ++i) {
				switch(wmode) {
					case w_normal:
						if(buffer[i]=='\033' && (i<len-2 && buffer[i+1]=='$')) {
							++i;
							wmode=w_fill;
							combine.clear();
							}
						else {
//							cmm->debugout << buffer[i];
							s_stdout << buffer[i];
							if(buffer[i]=='\n') last_output_line.clear();
							else                last_output_line+=buffer[i];
							}
						break;
					case w_fill:
						if(buffer[i]=='$') {
							wmode=w_normal;
							redraw_line(combine, pos, pos-1);
							}
						else combine+=buffer[i];
						break;
					}
				}
			s_stdout << flush;
			}
		} while(len>0);
	p.clear();
	return true;
	}

bool receive_stdin(modglue::ipipe& p)
	{
	char buffer[1024];
	int len;
	do {
		p.read(buffer,1016);
		len=p.gcount();
		if(len>0) 
			process_stdin(buffer, len);
		} while(len>0);

	p.clear();
	return true;
	}

bool receive_stderr(modglue::ipipe& p)
	{
	char buffer[1024];
	int len;
	do {
		p.read(buffer,1016);
		len=p.gcount();
		if(len>0)
			s_stderr << buffer << flush;
		} while(len>0);
	p.clear();
	return true;
	}


int main(int argc, char **argv)
	{
	if(argc==1) {
		cerr << "prompt (";
#ifdef STATICBUILD
		cerr << "static, ";
#endif
		cerr << "built on " << DATETIME << /* " on " << HOSTNAME << */ ")" << std::endl
			  << "Copyright (C) 2001-2006  Kasper Peeters <kasper.peeters@aei.mpg.de>" << endl << endl
			  << "Usage: prompt [program] [args]" << endl;
		exit(-1);
		}
	if(!isatty(0)) {
		cerr << "prompt: stdin is not a tty." << endl;
		exit(-2);
		}

	modglue::main mm(argc, argv);
	mm.add(&s_stdin, 0);
	mm.add(&s_stdout, 1);
	mm.add(&s_stderr, 2);
	cmm=&mm;
	fcntl(0, F_SETFL, O_NONBLOCK);

	if(mm.check()) {
		int start=1;
		string program(argv[start]);
		modglue::ext_process proc(program);
		for(int i=start+1; i<argc; ++i) {
			proc << argv[i]; 
			}
		mm.add(&proc);
		try {
			proc.setup_pipes();
			signal(SIGINT,sigc_handler);
			proc.fork();
			thepid=proc.get_pid();

			s_stdin.receiver.connect(sigc::ptr_fun(receive_stdin));
			proc.input_pipe("stdout")->receiver.connect(sigc::ptr_fun(receive_stdout));
			proc.input_pipe("stderr")->receiver.connect(sigc::ptr_fun(receive_stderr));
			t_stdin=proc.output_pipe("stdin");
			mm.process_died.connect(sigc::ptr_fun(died));
			
			history.push_back("");
			raw_terminal();
			mm.run(2);
			}
		catch(exception& ex) {
			cerr << ex.what() << endl;
			return -1;
			}
		s_stdout << std::flush;
		tcsetattr(0, TCSANOW, &saved_attributes);
		return 0;
		}
	else return -2;
	}
