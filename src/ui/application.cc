#include <iostream>
#include <sstream>
#include <queue>
#include "../cpu.h"
#include "../utils/utility.h"

#include "application.h"
#include "datagrid.h"
#include "cpumodel.h"
#include "flash.h"
#include "eeprom.h"
#include "config.h"
#include "machine.h"
#include "comparator_diagram.h"
#include "display_registers.h"
#include "timer0.h"
#include "paint/dlg_context.h"


Sim16F::Sim16F(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade)
	: Gtk::Window(cobject), m_refGlade(refGlade), m_cpu(0) {}


void Sim16F::init_cpu(CPU_DATA &cpu) {
	m_cpu = &cpu;
	m_parts["CPU"] = new app::CPUModel(cpu, m_refGlade);
	m_parts["EEPROM"] = new app::EEMemory(cpu, m_refGlade);
	m_parts["Flash"] = new app::FlashMemory(cpu, m_refGlade);
	m_parts["Config"] = new app::Config(cpu, m_refGlade);
	m_parts["Machine"] = new app::Machine(cpu, m_refGlade);
	m_parts["Registers"] = new app::DisplayRegisters(cpu, m_refGlade);
	m_parts["Timer0"] = new app::Timer0(cpu, m_refGlade);
	m_parts["Comparators"] = new app::Comparators(cpu, m_refGlade);

	new app::ContextDialogFactory(m_refGlade);  // initialise factory.

	this->signal_delete_event().connect(sigc::mem_fun(*this, &Sim16F::delete_event));
}


Sim16F::~Sim16F() {
	for(each_part p = m_parts.begin(); p != m_parts.end(); ++p)
		delete p->second;
}

Application::Application() {
	Glib::init();
	app = Gtk::Application::create("org.another.sim16fcc.base");
}

bool Application::create_window() {
	bool show_icon = false;

	auto builder = Gtk::Builder::create();

	try {
		builder->add_from_file("src/resource/layout.glade");
	} catch(const Glib::FileError& ex)	{
		std::cerr << "FileError: " << ex.what() << std::endl;
		return false;
	} catch(const Glib::MarkupError& ex) {
		std::cerr << "MarkupError: " << ex.what() << std::endl;
		return false;
	} catch(const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		return false;
	}

	if (show_icon) {
		// This call to get_widget_derived() requires gtkmm 3.19.7 or higher.
		//	bool is_glad=true;
		// builder->get_widget_derived("Window", pWindow, is_glad);
	} else
		builder->get_widget_derived("sim16f_main", pWindow);
	Dispatcher("recalculate");   // create the 'recalculate' dispatcher
	return true;
}

void Application::run(CPU_DATA &cpu) {
	if(pWindow) {
		//Start:
		try {
			pWindow->init_cpu(cpu);
			app->run(*pWindow);
		} catch (std::string &error) {
			std::cout << "Program terminated with exception: " << error << "\n";
		}
		LockUI lock;
		delete pWindow;
	}
}
