
#include <fstream>
#include <unistd.h>
#include <modglue/main.hh>
#include <modglue/ext_process.hh>
#include <modglue/process.hh>

using namespace std;

modglue::ext_process ls_proc("ls");
modglue::ext_process mi_proc("examples/multi_io");

bool print(modglue::ipipe& p)
	{
	string str;
	while(getline(p,str)) {
		cerr << "from ls: |" << str << "|" << endl;
//		  cerr << "forwarding to " 
//				 << mi_proc.output_pipe("foo")->name() << " "
//				 << mi_proc.output_pipe("foo")->fd() << endl;
		*(mi_proc.output_pipe("foo")) << str << endl << flush;
		}
	p.clear();
	return true;
	}

bool printmio(modglue::ipipe& p)
	{
	string str;
	while(getline(p,str)) {
		cerr << "from mi: |" << str << "|" << endl;
		}
	p.clear();
	return true;
	}

int counter=1;

bool died(modglue::ext_process& pr)
	{
	cerr << "********process " << pr.name() << " died" << endl;
	if(counter<2) {
		++counter;
		pr.fork();
		return false;
		}
	return true;
	}

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	try {
		ls_proc.setup_pipes();
		mi_proc.setup_pipes();

		mm.add(&ls_proc);
		mm.add(&mi_proc);

		ls_proc.input_pipe("stdout")->receiver.connect(sigc::ptr_fun(print));
		mi_proc.input_pipe("bar")->receiver.connect(sigc::ptr_fun(printmio));

		mm.process_died.connect(sigc::ptr_fun(died));

		ls_proc.fork();
		mi_proc.fork();
		mm.run(0);
		}
	catch(exception& ex) {
		cerr << ex.what() << endl;
		}
	}
