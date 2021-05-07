#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "porta.h"

namespace app {

	class Machine: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		std::vector<Component *> component_tabs;

	  protected:
		PortA port_a;
	  public:
		Machine(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade),
			port_a(a_cpu, a_refGlade)
		{}
	};

}
