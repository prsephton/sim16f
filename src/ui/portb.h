#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "port_b0.h"
#include "port_b1.h"
#include "port_b2.h"
#include "port_b3.h"
//#include "port_b4.h"
//#include "port_b5.h"
//#include "port_b6.h"
//#include "port_b7.h"
#include "../utils/smart_ptr.h"

namespace app {

	class PortB: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		SmartPtr<Component> b0;
		SmartPtr<Component> b1;
		SmartPtr<Component> b2;
		SmartPtr<Component> b3;
		SmartPtr<Component> b4;
		SmartPtr<Component> b5;
		SmartPtr<Component> b6;
		SmartPtr<Component> b7;

	  public:

		PortB(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
				m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			b0 = new PortB0(a_cpu, a_refGlade);
			b1 = new PortB1(a_cpu, a_refGlade);
			b2 = new PortB2(a_cpu, a_refGlade);
			b3 = new PortB3(a_cpu, a_refGlade);
//			b4 = new PortB4(a_cpu, a_refGlade);
//			b5 = new PortB5(a_cpu, a_refGlade);
//			b6 = new PortB6(a_cpu, a_refGlade);
//			b7 = new PortB7(a_cpu, a_refGlade);
		}
	};

}
