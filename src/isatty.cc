
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
	{
	std::cout << ((isatty(0)==1)?"stdin  is a tty":"stdin  is NOT a tty") << std::endl;
	std::cout << ((isatty(1)==1)?"stdout is a tty":"stdout is NOT a tty") << std::endl;
	std::cout << ((isatty(2)==1)?"stderr is a tty":"stderr is NOT a tty") << std::endl;

	return(2);
	}
