#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "port_a0.h"
#include "../utils/smart_ptr.h"

namespace app {

	class PortA: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		SmartPtr<Component> a0;

	  public:

		PortA(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			a0 = new PortA0(a_cpu, a_refGlade);
		}
	};

}
