#include <modglue/mid.hh>
#include <string>
#include <unistd.h>

using namespace modglue;

int          mid::highwater_=0;

mid::mid(void)
	{
	process_=getpid();
	++highwater_;
	serial_=highwater_;
	}

mid::mid(const mid& other)
	{
	serial_=other.serial_;
	process_=other.process_;
	}

int mid::operator==(const mid& other) const
	{
	return (process_==other.process_ && serial_==other.serial_);
	}

std::ostream& modglue::operator<<(std::ostream& str, const modglue::mid & m) 
   {
   return (str << "mid=" << m.process_ << ", " << m.serial_);
   }


