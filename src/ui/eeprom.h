//______________________________________________________________________________________________________
// Simple eeprom data editing view.
#pragma once

#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include "datagrid.h"
#include "../cpu_data.h"

namespace app {

	class EEPROM: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;

	  protected:
		DataGrid *grid;

	  public:
		EEPROM(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade) {

			auto eeprom_ra = new DeviceRandomAccessAdapter(a_cpu.eeprom);
			grid = new DataGrid(*eeprom_ra, m_refGlade, "eeprom_grid", "eeprom_scroll", 2);
		}

	};

};               // namespace app


