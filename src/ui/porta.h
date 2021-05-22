#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "port_a0.h"
#include "port_a1.h"
#include "port_a2.h"
#include "port_a3.h"
#include "port_a4.h"
#include "port_a6.h"
#include "../utils/smart_ptr.h"

namespace app {

	class PortA: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		SmartPtr<Component> a0;
		SmartPtr<Component> a1;
		SmartPtr<Component> a2;
		SmartPtr<Component> a3;
		SmartPtr<Component> a4;
		SmartPtr<Component> a6;

	  public:

		PortA(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			a0 = new PortA0(a_cpu, a_refGlade);
			a1 = new PortA1(a_cpu, a_refGlade);
			a2 = new PortA2(a_cpu, a_refGlade);
			a3 = new PortA3(a_cpu, a_refGlade);
			a4 = new PortA4(a_cpu, a_refGlade);
			a6 = new PortA6(a_cpu, a_refGlade);
		}
	};

}
