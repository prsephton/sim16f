//______________________________________________________________________________________________________
// A reasonably generic grid based hex editor.  Requires a "RandomAccess" adapter view for a
// device.  Currently, this applies to flash and eeprom devices.

#pragma once

#include <gtkmm.h>
#include "../devices/randomaccess.h"


namespace app {

	class GridEntry: public Component {
		Gtk::Entry *m_entry;
		RandomAccess &m_ra;
		unsigned int m_offset;
		unsigned int m_pos;


		bool update_data() {
			char *p = NULL;
			Glib::ustring buf = m_entry->get_text();
			float value = strtoul(buf.c_str(), &p, 16);
			if (p && !(*p)) {     // valid conversion
				m_ra.set_data(m_offset + m_pos, value);
				return true;  // continue
			}
			return false;
		}

	  protected:

		bool on_focus(Gtk::DirectionType d) {  // losing focus
			if (d == Gtk::DIR_TAB_FORWARD || Gtk::DIR_TAB_BACKWARD) {
				return !update_data(); // don't leave the cell if failed
			}
			return false;
		}

		void on_change() {
			update_data();
		}

	  public:
		void refresh(unsigned int scroll_pos) {
			m_pos = scroll_pos;
			if (m_offset + scroll_pos >= m_ra.size()) {
				m_entry->set_sensitive(false);
				m_entry->set_editable(false);
				m_entry->set_text("");
			} else {
				m_entry->set_sensitive(true);
				m_entry->set_editable(true);
				m_entry->set_text(int_to_hex(m_ra.get_data(m_offset + scroll_pos), "", ""));
			}
		}

		void flash(bool on) {
			if (on)
				m_entry->set_state(Gtk::STATE_SELECTED);
			else
				m_entry->set_state(Gtk::STATE_NORMAL);


//			m_entry->override_background_color(Gdk::RGBA("green"), Gtk::StateFlags::STATE_FLAG_NORMAL);
		}

		GridEntry(Gtk::Entry *a_entry, RandomAccess &ra, int a_offset):
			m_entry(a_entry), m_ra(ra), m_offset(a_offset), m_pos(0) {
			refresh(0);
			m_entry->set_alignment(1);
			m_entry->signal_changed().connect(sigc::mem_fun(*this, &GridEntry::on_change));
			m_entry->signal_focus().connect(sigc::mem_fun(*this, &GridEntry::on_focus));
		}
	};


	class DataGrid: public Component {
		DeviceRandomAccessAdapter &m_ra;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
	  protected:
		Glib::RefPtr<Gtk::Grid> m_grid;
		Glib::RefPtr<Gtk::Adjustment> m_adjustment;
		std::vector<GridEntry *> m_entry;

		void on_scroll_changed() {
			int offset = (int)m_adjustment->get_value();
			offset = (offset/16)*16;
			m_adjustment->set_value(offset);

//			std::cout << "scroll value: " << offset << "\n";
			show_grid_data(offset);
			return;
		}

		void set_up_grid(int max_length) {
			auto cstyle = Gtk::CssProvider::create();
			cstyle->load_from_data(
					"entry {font: 12px \"Fixed\"; }\n" \
					"entry:selected {color: #2020ff; background-color: #afaf3f; }");

			if (m_grid) {
				for (int n=0; n<16; ++n) {
					for (int c=0; c<16; ++c) {
						int offset = n*16+c;

						auto *e = dynamic_cast<Gtk::Entry *>(m_grid->get_child_at(c+1, n+1));
						e->set_max_length(max_length);
						e->set_width_chars(4);
//						e->set_margin_left(1);
						e->set_margin_right(1);
//						e->set_margin_top(1);
						e->set_margin_bottom(1);
						auto style = e->get_style_context();
						style->add_provider(cstyle, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
						m_entry.push_back(new GridEntry(e, m_ra, offset));

					}
				}
			}
		}

		void set_up_adjustment() {
			if (m_adjustment) {
				m_adjustment->set_lower(0);
				m_adjustment->set_upper(m_ra.size());
				m_adjustment->set_page_size(8*16);
				m_adjustment->set_step_increment(16);
				m_adjustment->set_value(0);
				m_adjustment->signal_value_changed().connect( sigc::mem_fun(*this, &DataGrid::on_scroll_changed) );
			}
		}

		void show_grid_data(unsigned int a_offset) {
			if (m_grid) {
				for (int n=0; n<16; ++n) {
					Gtk::Label *l = (Gtk::Label *)m_grid->get_child_at(0, n+1);
					l->set_text(int_to_hex(n*16 + a_offset, "", "h"));
				}
				typedef std::vector<GridEntry *>::iterator each_entry;
				for (each_entry e=m_entry.begin(); e != m_entry.end(); ++e) {
					(*e)->refresh(a_offset);
				}
			}
		}


	  public:
		void reset() {
			show_grid_data(0);
		}

		void position_for(WORD pc, bool on) {
			WORD scroll_pos = m_adjustment->get_value();
			if (pc < scroll_pos or pc >= scroll_pos+256) {
				m_adjustment->set_value(pc);
				scroll_pos = m_adjustment->get_value();
			}
			int offset = pc - scroll_pos;
			if (offset >=0 and offset < 256) {
				m_entry[offset]->flash(on);
			}
		}

		DataGrid(DeviceRandomAccessAdapter &ra, const Glib::RefPtr<Gtk::Builder>& a_refGlade,
				const char *grid_name, const char *scroll_name, int max_length):
			m_ra(ra), m_refGlade(a_refGlade) {

			m_grid = Glib::RefPtr<Gtk::Grid>::cast_dynamic(m_refGlade->get_object(grid_name));
			auto scroll = Glib::RefPtr<Gtk::Scrollbar>::cast_dynamic(m_refGlade->get_object(scroll_name));

			if (scroll)
				m_adjustment = scroll->get_adjustment();

			set_up_adjustment();
			set_up_grid(max_length);
			show_grid_data(0);
		}

	};
}
