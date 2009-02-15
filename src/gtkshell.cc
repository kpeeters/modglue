/* 

	Gtk shell for modglue.
	Copyright (C) 2001-2009  Kasper Peeters <kasper.peeters@aei.mpg.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
	
*/

#include <string>
#include <sys/socket.h>
#include <gtk--/main.h>
#include <gtk--/tree.h>
#include <gtk--/window.h>
#include <gtk--/box.h>
#include <gtk--/button.h>
#include <gtk--/checkbutton.h>
#include <gtk--/entry.h>
#include <gtk--/scrolledwindow.h>
#include <gtk--/label.h>
#include <gtk--/text.h>
#include <gdk--/color.h>
#include <gdk--/font.h>
#include <iostream>
#include <vector>
#include <modglue/modglue.hh>
#include <stdexcept>
#include <unistd.h>

/* 

If we want to eg. use wget to fetch network data, we need some way to
make `blocked' data transfer possible such that we know when a certain
write has finished. The latter can be faked by requiring a null or
something like that at the end for the time being. BETTER: use sendmsg.

echo "http://www.somewhere.net" | sed -e 's/\(.*\)/\1 -q -O -/'

Also make it possible to write out the data stored in the loader object
in a file and reload it at a later stage.

 */

class ProcessView;

modglue::pipe *inforeq; 
modglue::pipe *infodump;
modglue::pipe *fddump;  

class BondItem : public Gtk::TreeItem {
	public:
		BondItem(modglue::Bond *bd, ProcessView *owner);

		void refresh_subtree(void); // call this when bond_ content changes

		modglue::Bond *bond_;
	private:
		Gtk::Tree     pipetree_;
		ProcessView  *owner_;
};

class InfoItem : public Gtk::TreeItem {
	public:
		InfoItem(modglue::pipeinfo *);

		modglue::pipeinfo *get_pipeinfo(void) const;
	private:
		modglue::pipeinfo *info_;
};

class ASpaceItem : public Gtk::TreeItem {
	public:
		ASpaceItem(const string& name, ProcessView *owner);
	private:
		Gtk::Tree               proctree_;
		std::vector<InfoItem *> proc_items_;
		modglue::process       *proc_;
		ProcessView            *owner_;
};

class ToggleItem : public Gtk::TreeItem {
	public:
		ToggleItem(const string& name);
		
		Gtk::CheckButton *button(void);
		void select_impl(void) {} ;
	private:
		Gtk::CheckButton *cb_;
};

class ProcessItem : public Gtk::TreeItem {
	public:
		ProcessItem(const string& name, ProcessView *owner);

		void                      activate_subtree(modglue::process *);
		const vector<InfoItem *>& info_items(void) const;
		void                      update_pipe_subtree(void);
		modglue::process         *get_process(void) const;

		void                      toggle_event(Gtk::CheckButton *tb);
		ToggleItem           	toggle_start_on_input_;
		ToggleItem              toggle_abort_on_failed_write_;
	private:
		Gtk::Tree               infotree_;
		std::vector<InfoItem *> info_items_;
		modglue::process       *proc_;
		ProcessView            *owner_;
		ToggleItem           	toggle_restart_after_exit_;
};

class ProcessView : public Gtk::Window {
	public:
		ProcessView(int argc, char **argv, modglue::main& mn);
		~ProcessView();
		
		void cb_select_child(Gtk::Widget& child,Gtk::Tree *root_tree, Gtk::Tree *subtree);
		void cb_unselect_child(Gtk::Widget& child,Gtk::Tree *root_tree, Gtk::Tree *subtree);
		void cb_add_proc(void);
		void cb_run_proc(void);
		void cb_kill_proc(void);
		void cb_connect(void);
		void cb_generate(void);
		void cb_hide(void);
		void cb_remove_bonds(void);
		int  delete_event_impl(GdkEventAny *);

		void io_callback(int fd, GdkInputCondition);
		void inforeq_cb(const string&);
		void popup_window(int fd, GdkInputCondition);

		bool         no_window;
	private:
		void setup_tables_(int argc, char **argv);

		Gtk::ScrolledWindow sw,bsw;
		Gtk::Tree    tr;   // for address spaces and processes
		Gtk::Tree    btr;  // for bonds
		Gtk::VBox    vb0,vb1,vb2;
		Gtk::HBox    hb0,hb1,hb2,hb3,hb4,hb_misc;
		Gtk::Label   l_newproc, l_newid, l_newaddr;
		Gtk::Entry   e_procname, e_newid, e_addrname;
		Gtk::Button  b_newproc, b_newaddr;
		Gtk::Button  b_kill, b_connect, b_remove, b_run, b_cremove, b_generate, b_hide;

		std::vector<ProcessItem *>      processes_;
		std::vector<BondItem *>         bonds_;
		std::map<int, SigC::Connection> fdhandlers_;

		modglue::main &main_;
};

ToggleItem::ToggleItem(const string& name)
	: Gtk::TreeItem(name)
	{
	remove();
 	Gtk::HBox *hb=new Gtk::HBox;
 	add(*(manage(hb)));
	cb_=new Gtk::CheckButton();
 	hb->pack_start(*(manage(cb_)),false);
 	hb->pack_start(*(manage(new Gtk::Label(name))),false);
 	show_all();
	}

Gtk::CheckButton *ToggleItem::button(void)
	{
	return cb_;
	}

int ProcessView::delete_event_impl(GdkEventAny *)
	{
	modglue::loader::instance().quit();
	Gtk::Main::quit();
	return 0;
	}

BondItem::BondItem(modglue::Bond *bd, ProcessView *ow)
	: bond_(bd), owner_(ow), Gtk::TreeItem(bd->name)
	{
	}

void BondItem::refresh_subtree(void)
	{
	set_subtree(pipetree_);
	modglue::Bond::const_iterator it=bond_->pipes.begin();
	while(it!=bond_->pipes.end()) {
		Gtk::TreeItem *tmp=new Gtk::TreeItem((*it).first->get_name()+":"+
														 (*it).first->pipes[(*it).second]->get_name());
		pipetree_.tree().push_back(*(manage(tmp)));
		tmp->show_all();
		++it;
		}
	}

InfoItem::InfoItem(modglue::pipeinfo *inf)
	: info_(inf), Gtk::TreeItem("   "+inf->get_name()+
										 ((inf->get_inout()==modglue::pipeinfo::input)?" (input)":" (output)")
										 )
	{
	}

modglue::pipeinfo *InfoItem::get_pipeinfo(void) const
	{
	return info_;
	}

ProcessItem::ProcessItem(const string& name, ProcessView *owner)
	: Gtk::TreeItem(name), owner_(owner), toggle_start_on_input_("start on input"),
	  toggle_abort_on_failed_write_("abort on failed write"),
	  toggle_restart_after_exit_("restart after exit")
	{
	}

void ProcessItem::activate_subtree(modglue::process *proc)
	{
	proc_=proc;

	set_subtree(infotree_);
	infotree_.select_child.connect(bind(SigC::slot(owner_,&ProcessView::cb_select_child), 
													&infotree_, &infotree_));
	infotree_.append(toggle_start_on_input_);
	infotree_.append(toggle_abort_on_failed_write_);
//	infotree_.append(toggle_restart_after_exit_);
	toggle_start_on_input_.show_all();
	toggle_abort_on_failed_write_.show_all();
//	toggle_restart_after_exit_.show_all();
   toggle_start_on_input_.button()->toggled.connect(SigC::bind(SigC::slot(this, &ProcessItem::toggle_event),
															toggle_start_on_input_.button()));
   toggle_abort_on_failed_write_.button()->toggled.connect(
		SigC::bind(SigC::slot(this, &ProcessItem::toggle_event),
					  toggle_abort_on_failed_write_.button()));
   toggle_restart_after_exit_.button()->toggled.connect(SigC::bind(SigC::slot(this, &ProcessItem::toggle_event), 
												  toggle_restart_after_exit_.button()));

	update_pipe_subtree();
	}

void ProcessItem::update_pipe_subtree(void)
	{
	// This only _adds_, it cannot yet handle removals (but modglue cannot either).
	for(unsigned int i=info_items_.size(); i<proc_->pipes.size(); ++i) {
		info_items_.push_back(new InfoItem(proc_->pipes[i]));
		infotree_.append(*(info_items_.back()));
		info_items_.back()->show_all();
		}
	}

const vector<InfoItem *>& ProcessItem::info_items(void) const
	{
	return info_items_;
	}

modglue::process *ProcessItem::get_process(void) const
	{
	return proc_;
	}

void ProcessItem::toggle_event(Gtk::CheckButton *tb)
	{
	bool onoff=tb->get_active();
	if(tb==toggle_start_on_input_.button())             proc_->start_on_input=onoff;
	else if(tb==toggle_abort_on_failed_write_.button()) proc_->abort_on_failed_write=onoff;
	else                                                proc_->restart_after_exit=onoff;
	}

ProcessView::ProcessView(int argc, char **argv, modglue::main& mn)
	: no_window(false), l_newproc("program:"), l_newid("unique id:"), b_newproc("new process"), 
	  l_newaddr("space name:"), b_newaddr("new address space"), 
	  b_kill("kill"), b_connect("connect"), b_remove("remove proc"), b_run("run"), b_cremove("remove bond"),
	  b_generate("generate script"), b_hide("hide"), main_(mn)
	{
	add(vb0);
	vb0.pack_start(hb2, false);
//	vb0.pack_start(hb3,false);
	vb0.add(hb0);
	hb0.pack_start(vb1,true);
	hb0.pack_start(vb2,true);

	vb1.pack_start(sw, true);
	sw.add_with_viewport(tr);
	vb2.pack_start(bsw,true);
	bsw.add_with_viewport(btr);

	tr.select_child.connect(bind(SigC::slot(this,&ProcessView::cb_select_child), &tr, &tr));
	tr.unselect_child.connect(bind(SigC::slot(this,&ProcessView::cb_unselect_child), &tr, &tr));

	vb1.pack_start(hb1, false);
	hb1.add(b_connect);
	hb1.add(b_run);
	hb1.add(b_kill);
	hb1.add(b_remove);
	vb2.pack_start(hb4, false);
	hb4.add(b_cremove);
	vb2.pack_start(hb_misc,false);
	hb_misc.pack_start(b_generate,true);
	hb_misc.pack_start(b_hide,false);

	hb2.pack_start(l_newproc,false);
	hb2.pack_start(e_procname,true);
	hb2.pack_start(l_newid,false);
	hb2.pack_start(e_newid,true);
	hb2.pack_start(b_newproc,false);
	hb3.pack_start(l_newaddr,false);
	hb3.pack_start(e_addrname,true);
	hb3.pack_start(b_newaddr,false);
	b_newproc.clicked.connect(SigC::slot(this,&ProcessView::cb_add_proc));
	e_procname.activate.connect(SigC::slot(this,&ProcessView::cb_add_proc));
	e_newid.activate.connect(SigC::slot(this,&ProcessView::cb_add_proc));
	b_connect.clicked.connect(SigC::slot(this,&ProcessView::cb_connect));
	b_generate.clicked.connect(SigC::slot(this,&ProcessView::cb_generate));
	b_run.clicked.connect(SigC::slot(this,&ProcessView::cb_run_proc));
	b_kill.clicked.connect(SigC::slot(this,&ProcessView::cb_kill_proc));
	b_hide.clicked.connect(SigC::slot(this,&ProcessView::cb_hide));
	b_cremove.clicked.connect(SigC::slot(this,&ProcessView::cb_remove_bonds));
	
	vb1.show_all();
	b_kill.set_sensitive(false);
	b_remove.set_sensitive(false);
//	b_run.set_sensitive(false);
	tr.set_selection_mode(GTK_SELECTION_MULTIPLE);

	setup_tables_(argc, argv);
	}


void ProcessView::cb_select_child(Gtk::Widget& child,Gtk::Tree *root_tree, Gtk::Tree *subtree)
	{
	Gtk::Tree::SelectionList &selection=tr.selection();
	
	if(selection.empty()) {
//		b_run.set_sensitive(false);
		b_remove.set_sensitive(false);
		b_kill.set_sensitive(false);
		}
	else {
//		b_run.set_sensitive(true);
		b_remove.set_sensitive(true);
		b_kill.set_sensitive(true);
		}
	}

void ProcessView::cb_unselect_child(Gtk::Widget& child,Gtk::Tree *root_tree, Gtk::Tree *subtree)
	{
	}

void ProcessView::cb_add_proc(void)
	{
	ProcessItem *it=new ProcessItem(e_procname.get_text(), this);

	vector<string> args;
	modglue::loader::instance().convert_to_args(e_procname.get_text(), args);
	modglue::process *proc=modglue::loader::instance().load(args);
	if(e_newid.get_text()!="" && e_newid.get_text()!=e_procname.get_text()) 
		proc->set_id(e_newid.get_text());

	for(unsigned int i=0; i<processes_.size(); ++i) {
		if(processes_[i]->get_process()->get_id()==proc->get_id() ||
			processes_[i]->get_process()->get_name()==proc->get_id() ) {
			cerr << "module with id " << proc->get_id() << " already present" << endl;
			delete it;
// FIXME:      remove proc from loader
			return;
			}
		}
	tr.append(*it);
	it->activate_subtree(proc);
	processes_.push_back(it);
	it->expand();
	it->show();
	}

void ProcessView::cb_run_proc(void)
	{
	// first start all processes
	vector<ProcessItem *> changed; // keep track of which process changed status
	for(unsigned int k=0; k<processes_.size(); ++k) {
		if(processes_[k]->get_process()->has_sockets()==false) {
			if(processes_[k]->get_process()->get_pid()==0) {
				modglue::loader::instance().start(processes_[k]->get_process());
				Gtk::Label *pl=dynamic_cast<Gtk::Label *>(processes_[k]->get_child());
				assert(pl!=0);
				if(processes_[k]->get_process()->start_on_input)
					pl->set_text(processes_[k]->get_process()->get_name()+" (standby)");
				else
					pl->set_text(processes_[k]->get_process()->get_name()+" (running)");
				changed.push_back(processes_[k]);
				}
			}
		}
	// then add to the select loop
	for(unsigned int k=0; k<changed.size(); ++k) {
		for(unsigned int i=0; i<changed[k]->get_process()->pipes.size(); ++i) {
			if(changed[k]->get_process()->pipes[i]->get_fd()!=-1) {
				if(changed[k]->get_process()->pipes[i]->get_inout()==modglue::pipeinfo::output) {
					fdhandlers_[changed[k]->get_process()->pipes[i]->get_fd()]=
						Gtk::Main::input.connect(slot(this,&ProcessView::io_callback),
														 changed[k]->get_process()->pipes[i]->get_fd(),
														 (GdkInputCondition)(GDK_INPUT_READ));
					}
//				cerr << "adding " << changed[k]->get_process()->pipes[i]->fd() << endl;
				}
			}
		}
	}

void ProcessView::cb_kill_proc(void)
	{
	Gtk::Tree::SelectionList &selection=tr.selection();
	
	Gtk::Tree::SelectionList::iterator i=selection.begin();
	vector<pair<int, int> > sels;
	while(i!=selection.end()) {
      Gtk::TreeItem *item = (*i);
      Gtk::Label *label = dynamic_cast<Gtk::Label*>(item->get_child());
		if(label!=0) { // only if this is a properstupid selections of non-pipe items...
			string name=label->get();
			for(unsigned int k=0; k<processes_.size(); ++k) {
				if(static_cast<Gtk::TreeItem *>(processes_[k])==item) {
					modglue::loader::instance().kill_process(processes_[k]->get_process());
					break;
					}
				}
			}
		++i;
		}

	}

void ProcessView::cb_hide(void)
	{
	hide_all();
	}

void ProcessView::cb_remove_bonds(void)
	{
	Gtk::Tree::SelectionList &selection=btr.selection();

	Gtk::Tree::SelectionList::iterator i=selection.begin();
	while(i!=selection.end()) {
      BondItem *bitem = dynamic_cast<BondItem *>(*i);
		if(bitem) {
			Gtk::Tree_Helpers::ItemList::iterator ili=btr.tree().begin();
			while(ili!=btr.tree().end()) {
				if((*ili)==bitem) {
					modglue::loader::instance().remove_bond(bitem->bond_);
					// FIXME: if processes are running, we have to remove the fd listening
					// too. This will require the signalling pipe in modglue, otherwise
					// we end up with races. Perhaps remove_bond() should contain that logic.
					// FIXME: also remove it from the bonds_ container in processview;
					(*ili)->hide();
					btr.remove_item(**ili);
					break;
					}
				++ili;
				}
			}
		++i;
		}	
	}

void ProcessView::io_callback(int fd, GdkInputCondition) 
	{
	vector<modglue::process *> proc;
	vector<int>                remfds, addfds;
	modglue::loader::instance().select_callback(fd, proc, remfds, addfds);

	for(unsigned int i=0; i<addfds.size(); ++i) {
		cerr << "adding " << addfds[i] << " to select loop" << endl;
		fdhandlers_[addfds[i]]=
			Gtk::Main::input.connect(slot(this,&ProcessView::io_callback),
											 addfds[i],
											 (GdkInputCondition)(GDK_INPUT_READ));
		}

	for(unsigned int i=0; i<remfds.size(); ++i) {
		cerr << "removing " << remfds[i] << " from select loop" << endl;
		if(fdhandlers_.find(remfds[i])==fdhandlers_.end())
			throw logic_error("ordered to remove fd from select loop which was not added");
		fdhandlers_[remfds[i]].disconnect();
		}

	for(unsigned int k=0; k<proc.size(); ++k) {
		string txt;
		if(proc[k]->get_pid()==0) {
			if(proc[k]->has_sockets()==false) 
				txt=proc[k]->get_name();
			else
				txt=proc[k]->get_name()+" (standby)";
			}
		else { 
			txt=proc[k]->get_name()+" (running)";
			}
		for(unsigned int i=0; i<processes_.size(); ++i) {
			if(processes_[i]->get_process()==proc[k]) {
				processes_[i]->update_pipe_subtree();
				Gtk::Label *pl=dynamic_cast<Gtk::Label *>(processes_[i]->get_child());
				assert(pl!=0);
				pl->set_text(txt);
				break;
				}
			}
		}
	}

void ProcessView::setup_tables_(int argc, char **argv)
	{
	int i=1;
	string argbit;
	ProcessItem *current_module_=0;

	while(i<argc) {
		argbit=argv[i];
		if(argbit.substr(0,5)=="--run") {
			no_window=true;
			}
		else if(argbit.substr(0,9)=="--module=") {
 			if(current_module_) {
	 			current_module_=0;
	 			}
	 		string obj=argbit.substr(9);
	 		current_module_=new ProcessItem(obj,this);
			processes_.push_back(current_module_);
			tr.append(*current_module_);
			vector<string> args;
			modglue::loader::instance().convert_to_args(obj, args);
			modglue::process *proc=modglue::loader::instance().load(args);
			current_module_->activate_subtree(proc);
//			current_module_->expand();
			current_module_->show();
			}
		else if(argbit.substr(0,5)=="--id=") {
			assert(current_module_!=0);
			current_module_->get_process()->set_id(argbit.substr(5));
			} 	
		else if(argbit.substr(0,9)=="--options") {
			if(argbit.substr(10)=="start_on_input") {
				current_module_->get_process()->start_on_input=true;
				current_module_->toggle_start_on_input_.button()->set_active(true);
				}
			else if(argbit.substr(10)=="abort_on_failed_write") {
				current_module_->get_process()->abort_on_failed_write=true;
				current_module_->toggle_abort_on_failed_write_.button()->set_active(true);
				}
			}
		else if(argbit.substr(0,7)=="--bond=") {
			unsigned int pos=0;
			string subarg=argbit.substr(6);
			vector<pair<modglue::process *, int> > pps;
			do {
				subarg=subarg.substr(pos+1);
				string argument=subarg.substr(0,subarg.find_first_of(","));
				string prog=argument.substr(0,argument.find_first_of(":"));
				string pip =argument.substr(argument.find_first_of(":")+1);
//				cerr << prog << "|" << pip << endl;
				pair<modglue::process *, int> tmp;
				tmp.second=-1;
				for(unsigned int i=0; i<processes_.size(); ++i) {
					if(processes_[i]->get_process()->get_id()==prog) {
						tmp.first=processes_[i]->get_process();
						for(unsigned int j=0; j<tmp.first->pipes.size(); ++j) {
//							cerr << tmp.first->get_name() << ":" << tmp.first->pipes[j]->get_name() << endl;
							if(tmp.first->pipes[j]->get_name()==pip) {
								tmp.second=j;
								break;
								}
							}
						break;
						}
					}
				if(tmp.first==0) {
					cerr << "module " << prog << " not declared." << endl;
					continue;
					}
				if(tmp.second==-1) {
					cerr << "no pipe " << pip << " in module " << prog << endl;
					continue;
					}
				pps.push_back(tmp);
				} while((pos=subarg.find_first_of(","))<subarg.size());

			modglue::Bond *bd=new modglue::Bond("bond");
			for(unsigned int j=0; j<pps.size(); ++j) {
				bd->pipes.insert(pps[j]);
				}
			modglue::loader::instance().add_bond(bd);
			BondItem *bi=new BondItem(bd, this);
			bonds_.push_back(bi);
			btr.tree().push_back(*bi);
			bi->refresh_subtree();
			bi->expand();
			bi->show();
			}
		else {
//			cerr << "unknown argument " << argbit << endl;
			}
		++i;
		}
	}

void ProcessView::inforeq_cb(const string& inf)
	{
	if(inf.substr(0,10)=="list-pipes") {
		string tmp=main_.build_pipe_list();
		infodump->sender(tmp);
		}
	else if(inf.substr(0,10)=="list-bonds") {
		
		}
	else if(inf.substr(0,4)=="add ") {
		infodump->sender("adding "+inf.substr(4));
		string obj=inf.substr(4);
		if(obj[obj.size()-1]=='\n')
			obj=obj.substr(0,obj.size()-1);
		ProcessItem *current_module_=new ProcessItem(obj,this);
		processes_.push_back(current_module_);
		tr.append(*current_module_);
		vector<string> args;
		modglue::loader::instance().convert_to_args(obj, args);
		modglue::process *proc=modglue::loader::instance().load(args);
		current_module_->activate_subtree(proc);
		current_module_->expand();
		current_module_->show();

		infodump->sender("creating");
		vector<int> addfds;
		modglue::loader::instance().create_forward_pipes(proc, fddump, addfds);
		for(unsigned int i=0; i<addfds.size(); ++i) {
			cerr << "adding " << addfds[i] << " to select loop" << endl;
			fdhandlers_[addfds[i]]=
				Gtk::Main::input.connect(slot(this,&ProcessView::io_callback),
												 addfds[i],
												 (GdkInputCondition)(GDK_INPUT_READ));
			}
		}
	}

void ProcessView::popup_window(int fd, GdkInputCondition)
	{
	char buffer[100];
	read(fd, buffer, 99);
	show_all();
	}

void ProcessView::cb_generate(void)
	{
	cerr << modglue::loader::instance().generate_script() << endl;
	}

void ProcessView::cb_connect(void)
	{
	Gtk::Tree::SelectionList &selection=tr.selection();
//	cerr << selection.size() << " items selected " << endl;
	
	Gtk::Tree::SelectionList::iterator i=selection.begin();
	vector<pair<int, int> > sels;
	while(i!=selection.end()) {
      Gtk::TreeItem *item = (*i);
      Gtk::Label *label = dynamic_cast<Gtk::Label*>(item->get_child());
		if(label!=0) { // stupid selections of non-pipe items...
			string name=label->get();
			for(unsigned int k=0; k<processes_.size(); ++k) {
				if(static_cast<Gtk::TreeItem *>(processes_[k])==item) {
//				cerr << "main: " << processes_[k] << " " << name << endl;
					break;
					}
				for(unsigned m=0; m<processes_[k]->info_items().size(); ++m) {
					if(processes_[k]->info_items()[m]==item) {
						sels.push_back(pair<int,int>(k,m));
//						cerr << "pipe: " << processes_[k]->info_items()[m]->get_pipeinfo()->get_name()
//							  << " of process " << processes_[k]->get_process()->get_name() << endl;
						}
					}
				}
			}
		++i;
		}

	modglue::Bond *bd=new modglue::Bond("bond");
	for(unsigned int j=0; j<sels.size(); ++j) {
		InfoItem *inf=processes_[sels[j].first]->info_items()[sels[j].second];
		Gtk::Label *pl=dynamic_cast<Gtk::Label *>(inf->get_child());
		assert(pl!=0);
		bd->pipes.insert(pair<modglue::process *, int>(processes_[sels[j].first]->get_process(), 
																	  sels[j].second));
//		pl->set_text("C "+inf->get_pipeinfo()->get_name()+
//						 ((inf->get_pipeinfo()->get_inout()==modglue::pipeinfo::input)?" (input)":" (output)"));
		}
	modglue::loader::instance().add_bond(bd);
	BondItem *bi=new BondItem(bd, this);
	bonds_.push_back(bi);
	btr.tree().push_back(*bi);
	bi->refresh_subtree();
	bi->expand();
	bi->show();

/*	for(unsigned int i=0; i<tr.tree().size(); ++i) {
		tr.unselect_item(i);
		} */

/*	i=selection.begin();
	while(i!=selection.end()) {
		(*i)->deselect();
		++i;
		} */
	}

ProcessView::~ProcessView()
	{
	for(unsigned int i=0; i<processes_.size(); ++i) {
		delete processes_[i];
		}
	for(unsigned int i=0; i<bonds_.size(); ++i) {
		delete bonds_[i];
		}
	}

int main(int argc, char **argv)
	{
   modglue::main *mm=modglue::loader::instance().create_main(argc, argv);
	Gtk::Main mymain(&argc, &argv);

	inforeq =new modglue::pipe("inforeq", modglue::pipe::input);
	infodump=new modglue::pipe("infodump", modglue::pipe::output);
	fddump  =new modglue::pipe("fddump", modglue::pipe::output);  

	mm->add(inforeq);
	mm->add(infodump);
	mm->add(fddump);

	if(mm->check()) {
		ProcessView pv(argc, argv, *mm);
		pv.set_usize(400,500);
		if(pv.no_window==false) {
			pv.show_all();
			}
		
		inforeq->receiver.connect(SigC::slot(pv,&ProcessView::inforeq_cb));
		
		Gtk::Main::input.connect(slot(pv,&ProcessView::io_callback),
										 modglue::loader::instance().sig_chld_pipe_[0],
										 (GdkInputCondition)(GDK_INPUT_READ|GDK_INPUT_EXCEPTION));
		if(inforeq->get_fd()!=-1) {
			Gtk::Main::input.connect(slot(pv,&ProcessView::io_callback),
											 inforeq->get_fd(),
											 (GdkInputCondition)(GDK_INPUT_READ|GDK_INPUT_EXCEPTION));
			}
		
		// quick trick to allow us to pop up a window by sending something to stdin; should
		// be replaced with a handler that listens to SIGHUP or something like that.
		Gtk::Main::input.connect(slot(pv,&ProcessView::popup_window),
										 0,
										 (GdkInputCondition)(GDK_INPUT_READ|GDK_INPUT_EXCEPTION));
		
		try {
			if(pv.no_window)
				pv.cb_run_proc();
			mymain.run();
			}
		catch(exception &ex) { 
			cerr << "[exception]: " << ex.what() << endl;
			}
		}
	}
