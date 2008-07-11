
#include <modglue/main.hh>
#include <modglue/pipe.hh>
#include <fstream>

using namespace std;

modglue::ipipe ip("stdin");
modglue::opipe op("stdout");
modglue::opipe op2("stderr");

std::ofstream out("test_writes.out");

bool inp(modglue::ipipe& p)
	{
	static bool done=false;
	
	if(!done) {
		std::ifstream inp("out");
		char buffer[2024];
		int num;
		for(;;) {
			inp.read(buffer, 2023);
			num=inp.gcount();
			if(num>0) {
				op.write(buffer,num);
				}
			else break;
			}
		op << std::flush;
		done=true;
		}
/*	
	char buffer[1024];
	streamsize num;
	for(;;) { 
		p.read(buffer,1016); // CHECKED: if we do it this way, all characters get received
		num=p.gcount();
		if(num>0) {
			std::cerr << "recv " << num << std::endl;
			op.write(buffer, num);
			out.write(buffer, num);
//			cerr.write(buffer, num);
			}
		else break;
		}
	p.clear();
	return true;

	std::string lng;
	for(unsigned int i=0; i<500; ++i)
		lng+="testing 1 2 3 4 5 ";

	for(;;) {
		op << lng << std::endl;
		}
		p.clear(); */
	return true;
	}

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	mm.add(&ip,0);
	mm.add(&op,1);
	mm.add(&op2,2);

	if(mm.check()) {
		ip.receiver.connect(sigc::ptr_fun(inp));
		mm.run(1);
		}
   // IMPORTANT: remember to flush before closing.
	op << std::flush;
	}
