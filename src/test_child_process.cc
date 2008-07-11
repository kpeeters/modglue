
#include <fstream>
#include <iostream>
#include <modglue/process.hh>
#include <modglue/ext_process.hh>

using namespace std;

int main(int argc, char **argv)
	{
	string result;

	modglue::child_process ls_proc("ls");
	ls_proc << "-la" << "/tmp";
	ls_proc.call("", result);
	
	std::cout << "result of ls call: |" << result << "|" << std::endl;


	// like libexecstream: http://libexecstream.sourceforge.net/
	modglue::ext_process cat_proc("cat");
	cat_proc.setup_pipes();
	cat_proc.output_pipe("stdin")->set_blocking();
	cat_proc.fork();
	std::cout << cat_proc.output_pipe("stdin")->fd() << std::endl;
	*(cat_proc.output_pipe("stdin"))	<< "hi there" << std::endl;
	while(std::getline(*(cat_proc.input_pipe("stdout")), result)) {
		std::cout << "received: |" << result << "|" << std::endl;
		}
	cat_proc.terminate();
	}
