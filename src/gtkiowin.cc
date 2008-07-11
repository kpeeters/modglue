
/*

   Modglue & Gtk-- based program to view traffic on pipes.

 */

#include <gtk--/main.h>
#include <gtk--/window.h>
#include <gtk--/entry.h>
#include <gtk--/button.h>
#include <gtk--/togglebutton.h>
#include <gtk--/box.h>
#include <gtk--/text.h>
#include <gtk--/scrolledwindow.h>
#include <gdk--/color.h>
#include <gdk--/font.h>
#include <modglue/modglue.hh>
#include <sigc++/signal_system.h>

class PipeView : public Gtk::Window {
	public:
		PipeView(void);
		void print(const string& txt);
		void excprint(const string& txt);
		void cb_erase(void);
		void cb_send(void);

		int delete_event_impl(GdkEventAny *event);
	private:
		Gtk::VBox   vb1_;
		Gtk::HBox   hb1_;
		Gtk::ToggleButton tb1_, tb2_;
		Gtk::Entry  out_;
		Gtk::ScrolledWindow sw_;
		Gtk::Text   monitor_;
		Gtk::Button but_;
		Gdk_Color   black,white,red;
		Gdk_Font    fixed_font;
};

PipeView::PipeView(void)
	: tb1_("send double lf"), tb2_("strip input lf"), but_("clear")
	{
	add(vb1_);
	vb1_.pack_start(out_,false);
	vb1_.pack_start(hb1_,false);
	hb1_.pack_start(tb1_,true);
	hb1_.pack_start(tb2_,true);
	vb1_.pack_start(sw_,true);
	sw_.add(monitor_);
	monitor_.set_usize(200,300);
	vb1_.pack_start(but_,false);
	vb1_.show_all();
	red.set_rgb(0xffff,0,0);
	white.set_rgb(0xffff,0xffff,0xffff);
	black.set_rgb(0,0,0); 
	monitor_.set_editable(false);
	but_.clicked.connect(slot(this,&PipeView::cb_erase));
	out_.activate.connect(slot(this,&PipeView::cb_send));
	}

int PipeView::delete_event_impl(GdkEventAny *)
	{
	Gtk::Main::quit();
	return 0;
	}

void PipeView::cb_erase(void)
	{
	monitor_.set_point(0);
	monitor_.forward_delete(monitor_.get_length());
	}

void PipeView::print(const string& txt)
	{
	string tmp=txt;
	monitor_.freeze();
	if(tb2_.get_active())
		if(txt[txt.size()-1]=='\n')
			tmp=txt.substr(0,txt.size()-1);
	if(txt[tmp.size()-1]=='\004') 
		tmp=tmp.substr(0,tmp.size()-1)+"\n--EOF--\n";
	monitor_.insert(fixed_font, black, white, tmp, -1);
	monitor_.thaw();
	}

void PipeView::excprint(const string& txt)
	{
	monitor_.freeze();
//	cerr << "|" << txt << "| " << txt.size() << endl;
	monitor_.insert(fixed_font, red, white, txt, -1);
	monitor_.thaw();
	}

modglue::pipe *outpipe;;
modglue::pipe *foopipe;
modglue::pipe *excpipe;

map<int, SigC::Connection> connections;

void PipeView::cb_send(void)
	{
	try {
		if(tb1_.get_active())
			outpipe->sender(out_.get_text()+"\n\n");
		else
			outpipe->sender(out_.get_text()+"\n");
		}
	catch(exception &ex) {
//		cerr << "caught writing exception: " << ex.what() << endl;
		}
	}

void callmm(int fd, GdkInputCondition, modglue::main *mm) 
	{
	if(!(mm->select_callback(fd))) {
		connections[fd].disconnect();
		}
	}

int main(int argc, char **argv)
	{
   modglue::main *mm=new modglue::main(argc, argv);
	Gtk::Main     mymain(&argc, &argv);

	outpipe =new modglue::pipe("stdout", modglue::pipe::output, 1);
	foopipe =new modglue::pipe("stdin", modglue::pipe::input, 0);
	excpipe =new modglue::pipe("stdexc", modglue::pipe::input, 2);

	mm->add(outpipe);
   mm->add(foopipe);
   mm->add(excpipe);

	if(mm->check()) {
		PipeView mp;
		foopipe->receiver.connect(SigC::slot(&mp,&PipeView::print));
		excpipe->receiver.connect(SigC::slot(&mp,&PipeView::excprint));
		
		mp.show();
		for(unsigned int i=0; i<mm->pipes().size(); ++i) {
			connections[mm->pipes()[i]->get_fd()]=Gtk::Main::input.connect(bind(SigC::slot(&callmm),mm),
											 mm->pipes()[i]->get_fd(), 
											 (GdkInputCondition)(GDK_INPUT_READ|GDK_INPUT_EXCEPTION));
			}
		mymain.run();
		}
	}
