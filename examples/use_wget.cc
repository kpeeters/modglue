
#include <string>
#include <modglue/main.hh>
#include <modglue/ext_process.hh>

bool receive_html(modglue::ipipe& p)
	{
	std::string str;
	while(getline(p,str)) {
		
		}
	p.clear();
	return true;
	}

bool receive_info(modglue::ipipe& p)
	{
	std::string str;
	while(getline(p,str)) {
		}
	std::cerr << "." << std::flush;
	p.clear();
	return true;
	}

bool receive_exitstatus(modglue::ext_process& pr)
	{
	std::cerr << "exit code:" << pr.exit_code() << std::endl;
	return true;
	}

int main(int argc, char **argv)
	{
	modglue::main mm(argc, argv);

	modglue::ext_process wgett("wget");
	wgett << "-O" << "-";

	try {
		modglue::ext_process wget1(wgett);
		wget1 << "http://www.ictp.trieste.it";
		wget1.setup_pipes();
		mm.add(&wget1);
		wget1.input_pipe("stdout")->receiver.connect(sigc::ptr_fun(receive_html));
		wget1.input_pipe("stderr")->receiver.connect(sigc::ptr_fun(receive_info));
		mm.process_died.connect(sigc::ptr_fun(receive_exitstatus));
		wget1.fork();

		/*

		  This would be cleaner when written as:
        wget.set_io(&receive_html,&receive_info);
		  wget.fork();

		 */

		mm.run(1);
		}
	catch(std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		}
	}
