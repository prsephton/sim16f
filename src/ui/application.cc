#include <iostream>
#include <sstream>
#include <queue>
#include "../cpu.h"
#include "../utils/utility.h"

#include "application.h"
#include "datagrid.h"
#include "flash.h"
#include "eeprom.h"
#include "config.h"
#include "machine.h"



Sim16F::Sim16F(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade)
	: Gtk::Window(cobject), m_refGlade(refGlade), m_cpu(0) {}


void Sim16F::init_cpu(CPU_DATA &cpu) {
	m_cpu = &cpu;
	m_parts["EEPROM"] = new app::EEPROM(cpu, m_refGlade);
	m_parts["Flash"] = new app::Flash(cpu, m_refGlade);
	m_parts["Config"] = new app::Config(cpu, m_refGlade);
	m_parts["Machine"] = new app::Machine(cpu, m_refGlade);
	this->signal_delete_event().connect(sigc::mem_fun(*this, &Sim16F::delete_event));
}


Sim16F::~Sim16F() {
	for(each_part p = m_parts.begin(); p != m_parts.end(); ++p)
		delete p->second;
}


void run_application(CPU_DATA &cpu) {
	bool show_icon = false;
	Sim16F* pWindow = nullptr;

	Glib::init();
	auto builder = Gtk::Builder::create();

	auto app = Gtk::Application::create("org.another.sim16fcc.base");

	try {
		builder->add_from_file("src/resource/layout.glade");
	} catch(const Glib::FileError& ex)	{
	    std::cerr << "FileError: " << ex.what() << std::endl;
	    return;
	} catch(const Glib::MarkupError& ex) {
	    std::cerr << "MarkupError: " << ex.what() << std::endl;
	    return;
	} catch(const Gtk::BuilderError& ex) {
	    std::cerr << "BuilderError: " << ex.what() << std::endl;
	    return;
	}

	if (show_icon) {
		// This call to get_widget_derived() requires gtkmm 3.19.7 or higher.
		//	bool is_glad=true;
		// builder->get_widget_derived("Window", pWindow, is_glad);
	} else
		builder->get_widget_derived("sim16f_main", pWindow);
	if(pWindow) {
	    //Start:
		try {
			pWindow->init_cpu(cpu);
			app->run(*pWindow);
		} catch (std::string &error) {
			std::cout << "Program terminated with exception: " << error << "\n";
		}
	}
	delete pWindow;

}
