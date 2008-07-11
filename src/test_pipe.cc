
#include <modglue/main.hh>
#include <modglue/pipe.hh>

using namespace std;

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
/*
   modglue::main::init(argc, argv);
   
 */

	modglue::main mm(argc, argv);

	mm.add(&ip,0);
	mm.add(&op,1);

	if(mm.check()) {
		ip.receiver.connect(sigc::ptr_fun(inp));
		mm.run(1);
		}

	// Also test that creation of pipes does not leak memory.
	}
