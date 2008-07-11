
/*

   Simple program to show usage of multiple I/O pipes in addition
   to the standard stdin/stdout/stderr combo.

 */

#include <modglue/pipe.hh>
#include <modglue/main.hh>
#include <sigc++/sigc++.h>
#include <iostream>

using namespace std;

modglue::ipipe foopipe("foo");
modglue::ipipe foo2pipe("foo2");
modglue::opipe barpipe("bar");

bool print(modglue::ipipe& p)
   {
	string txt;
	while(p >> txt) {
		cerr << "from foo: |" << txt << "|" << endl;
		barpipe << "bar:" << txt << endl;
		}
	p.clear();
	return true;
   }

bool print2(modglue::ipipe& p)
   {
	string txt;
	while(p >> txt) {
		barpipe << "thank you 2!" << endl;
		}
	p.clear();
	return true;
   }

int main(int argc, char **argv) 
   {
   modglue::main mm(argc, argv);

   mm.add(&foopipe,0);
   mm.add(&foo2pipe);
   mm.add(&barpipe,2);

   foopipe.receiver.connect(sigc::ptr_fun(print));
   foo2pipe.receiver.connect(sigc::ptr_fun(print2));
   
   mm.run(2);
   }
