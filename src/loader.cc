
#include <string>
#include <iostream>
#include <vector>
#include <modglue/modglue.hh>
#include <stdexcept>


void build_module_table(int argc, char **argv)
	{
	}

main(int argc, char **argv)
	{
	build_module_table(argc, argv);
	
	modglue::loader::instance().run();
	}

