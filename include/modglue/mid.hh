
/* Identifiers for socket messages */

#ifndef mid_hh_
#define mid_hh_

#include <string>
#include <iostream>
//#include <proj++/thread.hh>
//#include <proj++/thread_mutex.hh>

namespace modglue {

 class mid {
	 public:
		 mid(void);
		 mid(const mid&);
		 
		 int operator==(const mid&) const;
		 friend std::ostream& operator<<(std::ostream&, const mid&);
	 private:
		 pid_t     process_;
		 int       serial_;

		 static int          highwater_;
 };

 std::ostream& operator<<(std::ostream&, const modglue::mid&);

};

#endif
