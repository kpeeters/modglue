
#include <iostream>
#include <fstream>
#include <modglue/shell.hh>

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	modglue::ipipe p_command("stdin");
	modglue::opipe p_result("stdout");
	modglue::opipe p_error("stderr");

	mm.add(&p_command,0);
	mm.add(&p_result,1);
	mm.add(&p_error,2);

	if(mm.check()) {
		modglue::loader ld(&mm, &p_command, &p_result, &p_error);
		
		if(argc>1) { // read from script before going interactive
			std::ifstream ff(argv[1]);
			ld.accept_commands_old(ff);
			ff.close();
			}

		ld.print_prompt();
		try {
			mm.run(1);
			}
		catch(std::exception& ex) {
			std::cerr << ex.what() << std::endl;
			}
		}

	return 0;
	}
