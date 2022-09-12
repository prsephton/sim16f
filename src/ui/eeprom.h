//______________________________________________________________________________________________________
// Simple eeprom data editing view.
#pragma once

#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include "datagrid.h"
#include "../cpu_data.h"

namespace app {

	class EEMemory: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		std::queue<CpuEvent> m_cpu_events;

	  protected:
		DataGrid *grid;

		void eeprom_changed(EEPROM *e, const std::string &name) {
			if (name == "init" || name == "clear" || name == "reset") {
				reset();
			}
		}

	  public:
		void reset() {
			grid->reset();
		}

		EEMemory(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade) {

			auto eeprom_ra = new DeviceRandomAccessAdapter(a_cpu.eeprom);
			grid = new DataGrid(*eeprom_ra, m_refGlade, "eeprom_grid", "eeprom_scroll", 2);
			DeviceEvent<EEPROM>::subscribe<EEMemory>(this, &EEMemory::eeprom_changed);
		}
	};
}               // namespace app


