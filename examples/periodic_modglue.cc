
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <modglue/modglue.hh>
#include <sigc++/signal_system.h>
#include <iostream.h>

modglue::pipe *foopipe;

main(int argc, char **argv)
	{
   modglue::main *mm=modglue::loader::instance().create_main(argc, argv);
	foopipe=new modglue::pipe("stdout", modglue::pipe::output);
   mm->add(foopipe);

	if(mm->check()) {
		int counter=1;
		char buffer[100];
		
		while(true) {
			sprintf(buffer, "test %d\n", counter);
			foopipe->sender(buffer);
			++counter;
			sleep(1);
			}
		}
	}

