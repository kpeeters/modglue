
/*

  This is the same program as test_pipe.cc, but now using an
  external event loop (for this particular example, we use the
  Gtk event loop). 

 */

#include <modglue/main.hh>
#include <modglue/pipe.hh>

modglue::ipipe ip("input");
modglue::opipe op("input");

bool inp(modglue::ipipe& p)
	{
	string kasper;
	while(p>>kasper) {
		op << "received: " << kasper << endl;
		}
	p.clear();
	op << "--" << endl;

	return true;
	}

int main(int argc, char **argv)
	{
//	Gtk::Main

	modglue::main mm(argc, argv);

	mm.add(&ip,0);
	mm.add(&op,1);

	if(mm.check()) {
		ip.receiver.connect(SigC::slot(inp));
		mm.run();
		}

	// Also test that creation of pipes does not leak memory.
	}
