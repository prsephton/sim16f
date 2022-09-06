#include <iostream>
#include "dlg_context.h"

namespace app {
	SmartPtr<ContextDialogFactory> ContextDialogFactory::factory;

	std::string ContextDialog::as_text(double a_value) {
		std::string s;
		std::stringstream(s) << std::setprecision(4) << a_value;
		return s;
	}

	std::string ContextDialog::as_text(int a_value) {
		std::string s;
		std::stringstream(s) << a_value;
		return s;
	}

	double ContextDialog::as_double(std::string a_text) {
		double d;
		std::stringstream(a_text) >> d;
		return d;
	}

	int ContextDialog::as_int(std::string a_text) {
		int i;
		std::stringstream(a_text) >> i;
		return i;
	}


	//_____________________________________________________________________________________________________________
	// A convenience class to deal with values that have a unit.
	double ContextDialog::scaled_value::value_and_unit(double a_value, int &a_unit) {
		int mag = round(log10(a_value) / 3.0);  // round away from zero
		double divisor = pow(10, mag*3);
		return a_value / divisor;
	}

	void ContextDialog::scaled_value::set_from_value(double a_value) {
		int l_unit;
		double val = value_and_unit(a_value, l_unit);
		// units returned are from -4 -> +3, where
		//   -4 = pico
		//   -3 = nano
		//   -2 = micro
		//   -1 = milli
		//   0 = unit
		//   ... etc.
		// The ComboBoxText id starts at pico=0, ranging up to max_unit.

		max_unit -= 4;   // now using equivalent ComboBoxText units.
		if (l_unit > max_unit) {
			val *= pow(10, (max_unit-l_unit)*3);
			l_unit = max_unit;
		} else if (l_unit < -4) {
			val *= pow(10, (l_unit+4)*3);
			l_unit = -4;
		} else {
			val *= pow(10, l_unit*3);
		}
		cbox->set_active(l_unit+4);
		entry->set_text(ContextDialog::as_text(val));
	}

	double ContextDialog::scaled_value::value() {
		int l_unit = ContextDialog::as_int(cbox->get_active_id()) - 4;
		return ContextDialog::as_double(entry->get_text()) * pow(10, l_unit*3);
	}

	ContextDialog::scaled_value::scaled_value(
			Glib::RefPtr<Gtk::Entry> a_entry, Glib::RefPtr<Gtk::ComboBoxText> a_cbox, int a_max_unit):
				entry(a_entry), cbox(a_cbox), max_unit(a_max_unit){
	}


	//_____________________________________________________________________________________________________________
	// Set up the dialog, run it, update component with data from the dialog
	void ContextDialog::configure(Configurable &component) {

		// define data values potentially provided by, and to the component

		std::string l_name="";
		double l_voltage=0, l_capacitance=0, l_resistance=0, l_inductance=0;
		bool l_inverted=false, l_gate_invert=false;
		double l_orientation=0, l_scale=1;
		bool l_trigger=false;
		double l_xpos=0, l_ypos=0, l_width=100, l_height=100;
		int l_rows=1, l_inputs=1, l_selectors=1;
		bool l_attr_ntype=false, l_attr_synchronous=false, l_attr_first=false, l_attr_join=false;
		bool l_attr_invert=false, l_attr_underscore=false, l_attr_overscore=false, l_attr_bold=false;
		bool l_switch_open=false;
		std::string l_font_face="Sans";
		float l_font_size=10;
		double l_red=0, l_green=0, l_blue=0;

		// query component & configure dialog

		bool need_name = component.needs_name(l_name);
		m_name->set_visible(need_name);
		m_lb_name->set_visible(need_name);
		if (need_name) m_name->set_text(l_name);

		bool need_voltage = component.needs_voltage(l_voltage);
		m_voltage->get_parent()->set_visible(need_voltage);
		m_lb_voltage->set_visible(need_voltage);
		if (need_voltage) {
			scaled_value(m_voltage, m_voltage_unit, 0).set_from_value(l_voltage);
		}

		bool need_resistance = component.needs_resistance(l_resistance);
		m_resistance->get_parent()->set_visible(need_resistance);
		m_lb_resistance->set_visible(need_resistance);
		if (need_resistance) {
			scaled_value(m_resistance, m_resistance_unit, 2).set_from_value(l_resistance);
		}

		bool need_capacitance = component.needs_capacitance(l_capacitance);
		m_capacitance->get_parent()->set_visible(need_capacitance);
		m_lb_resistance->set_visible(need_capacitance);
		if (need_capacitance) {
			scaled_value(m_capacitance, m_capacitance_unit, 0).set_from_value(l_capacitance);
		}

		bool need_inductance = component.needs_inductance(l_inductance);
		m_inductance->get_parent()->set_visible(need_inductance);
		m_lb_inductance->set_visible(need_inductance);
		if (need_inductance) {
			scaled_value(m_inductance, m_inductance_unit, 0).set_from_value(l_inductance);
		}

		bool need_inverted = component.needs_inverted(l_inverted);
		m_rb_inverted->get_parent()->set_visible(need_inverted);
		m_lb_inverted->set_visible(need_inverted);
		if (need_inverted)
			m_rb_inverted->set_active(l_inverted);

		bool need_gate_inverted = component.needs_gate_inverted(l_gate_invert);
		m_rb_gate_inverted->get_parent()->set_visible(need_gate_inverted);
		m_lb_gate_invert->set_visible(need_gate_inverted);
		if (need_gate_inverted)
			m_rb_gate_inverted->set_active(l_gate_invert);

		std::cout << "Gate invert [app]=" << (l_gate_invert?"true":"false") << std::endl;

		bool need_orientation = component.needs_orientation(l_orientation);
		m_lb_orientation->set_visible(need_orientation);
		m_box_orientation->set_visible(need_orientation);
		if (need_orientation) {
			int compass = (int)(2 * l_orientation / M_PI);
			switch (compass) {
			  case 0:
				  m_rb_dir_fwd->set_active(true);
				break;
			  case 1:
				  m_rb_dir_dn->set_active(true);
				break;
			  case 2:
				  m_rb_dir_back->set_active(true);
				break;
			  case 3:
				  m_rb_dir_up->set_active(true);
				break;
			}
		}

		bool need_scale = component.needs_scale(l_scale);
		m_lb_scale->set_visible(need_scale);
		m_scale->set_visible(need_scale);
		if (need_scale)
			m_scale->set_text(as_text(l_scale));

		bool need_trigger = component.needs_trigger(l_trigger);
		m_lb_trigger->set_visible(need_trigger);
		m_box_trigger->set_visible(need_trigger);
		if (need_trigger)
			m_rb_trigger_pos->set_active(l_trigger);

		bool need_position = component.needs_position(l_xpos, l_ypos);
		m_box_position->set_visible(need_position);
		if (need_position) {
			m_posn_x->set_text(as_text(l_xpos));
			m_posn_y->set_text(as_text(l_ypos));
		}
		bool need_size = component.needs_size(l_width, l_height);
		m_box_size->set_visible(need_size);
		if (need_size) {
			m_size_w->set_text(as_text(l_width));
			m_size_h->set_text(as_text(l_height));
		}
		m_lb_position->set_visible(need_position || need_size);

		bool need_rows = component.needs_rows(l_rows);
		m_lb_rows->set_visible(need_rows);
		m_entry_rows->set_visible(need_rows);
		if (need_rows)
			m_entry_rows->set_text(as_text(l_rows));


		bool need_inputs = component.needs_inputs(l_inputs);
		m_lb_inputs->set_visible(need_inputs);
		m_entry_inputs->set_visible(need_inputs);
		if (need_inputs)
			m_entry_inputs->set_text(as_text(l_inputs));

		bool need_selectors = component.needs_selectors(l_selectors);
		m_lb_selectors->set_visible(need_selectors);
		m_entry_selectors->set_visible(need_selectors);
		if (need_selectors)
			m_entry_selectors->set_text(as_text(l_selectors));

		bool need_ntype = component.needs_ntype(l_attr_ntype);
		bool need_synchronous = component.needs_synchronous(l_attr_synchronous);
		bool need_first = component.needs_first(l_attr_first);
		bool need_join = component.needs_join(l_attr_join);
		bool need_invert = component.needs_invert(l_attr_invert);
		bool need_underscore = component.needs_underscore(l_attr_underscore);
		bool need_overscore = component.needs_overscore(l_attr_overscore);
		bool need_bold = component.needs_bold(l_attr_bold);
		m_ntype->set_visible(need_ntype);
		m_synchronous->set_visible(need_synchronous);
		m_first->set_visible(need_first);
		m_join->set_visible(need_join);
		m_invert->set_visible(need_invert);
		m_underscore->set_visible(need_underscore);
		m_overscore->set_visible(need_overscore);
		m_bold->set_visible(need_bold);
		if (need_ntype) m_ntype->set_active(l_attr_ntype);
		if (need_synchronous) m_synchronous->set_active(l_attr_synchronous);
		if (need_first) m_first->set_active(l_attr_first);
		if (need_join) m_join->set_active(l_attr_join);
		if (need_invert) m_invert->set_active(l_attr_invert);
		if (need_underscore) m_underscore->set_active(l_attr_underscore);
		if (need_overscore) m_overscore->set_active(l_attr_overscore);
		if (need_bold) m_bold->set_active(l_attr_bold);

		bool need_attributes = (
				need_ntype || need_synchronous || need_first ||
				need_join || need_invert || need_underscore ||
				need_overscore || need_bold);

		m_lb_attributes->set_visible(need_attributes);
		m_box_attributes->set_visible(need_attributes);

		bool need_switch = component.needs_switch(l_switch_open);
		m_lb_switch->set_visible(need_switch);
		m_box_switch->set_visible(need_switch);

		if (need_switch) m_rb_switch_open->set_active(l_switch_open);

		bool need_font = component.needs_font(l_font_face, l_font_size);
		m_lb_font->set_visible(need_font);
		m_bn_font->set_visible(need_font);
		if (need_font) {
			m_bn_font->set_font_name(l_font_face + " " + as_text(l_font_size));
		}

		bool need_colour = component.needs_fg_colour(l_red, l_green, l_blue);
		m_lb_colour->set_visible(need_colour);
		m_bn_colour->set_visible(need_colour);
		if (need_colour) {
			Gdk::RGBA colour;
			colour.set_rgba(l_red, l_green, l_blue);
			m_bn_colour->set_rgba(colour);
		}

		// poll the dialog
		int response = run();

		// query dialog and update component

		if (response == Gtk::RESPONSE_OK) {

			if (need_name) component.set_name(m_name->get_text());
			if (need_voltage) component.set_voltage(scaled_value(m_voltage, m_voltage_unit, 0).value());
			if (need_resistance) component.set_resistance(scaled_value(m_resistance, m_resistance_unit, 2).value());
			if (need_capacitance) component.set_capacitance(scaled_value(m_capacitance, m_capacitance_unit, 0).value());
			if (need_inductance) component.set_inductance(scaled_value(m_inductance, m_inductance_unit, 0).value());
			if (need_inverted) component.set_inverted(m_rb_inverted->get_active());
			if (need_gate_inverted) component.set_gate_inverted(m_rb_gate_inverted->get_active());
			std::cout << "Gate invert [dlg]=" << (m_rb_gate_inverted->get_active()?"true":"false") << std::endl;


			if (need_orientation) {
				if (m_rb_dir_fwd->get_active())
					component.set_orientation(0);
				else if (m_rb_dir_dn->get_active())
					component.set_orientation(M_PI/2);
				else if (m_rb_dir_back->get_active())
					component.set_orientation(M_PI);
				else if (m_rb_dir_up->get_active())
					component.set_orientation(M_PI*3/2);
			}
			if (need_scale) component.set_scale(as_double(m_scale->get_text()));
			if (need_trigger) component.set_trigger(m_rb_trigger_pos->get_active());
			if (need_position) component.set_position(
					as_double(m_posn_x->get_text()),
					as_double(m_posn_y->get_text()));

			if (need_size) component.set_size(
					as_double(m_size_w->get_text()),
					as_double(m_size_h->get_text()));

			if (need_rows) component.set_rows(as_int(m_entry_rows->get_text()));
			if (need_inputs) component.set_inputs(as_int(m_entry_inputs->get_text()));
			if (need_selectors) component.set_selectors(as_int(m_entry_selectors->get_text()));

			if (need_ntype) component.set_ntype(m_ntype->get_active());
			if (need_synchronous) component.set_synchronous(m_synchronous->get_active());
			if (need_first) component.set_first(m_first->get_active());
			if (need_join) component.set_join(m_join->get_active());
			if (need_invert) component.set_invert(m_invert->get_active());
			if (need_underscore) component.set_underscore(m_underscore->get_active());
			if (need_overscore) component.set_overscore(m_overscore->get_active());
			if (need_bold) component.set_bold(m_bold->get_active());

			if (need_switch) component.set_switch(m_rb_switch_open->get_active());
			if (need_font) {
				std::string l_fontname = m_bn_font->get_font_name();
				size_t n = l_fontname.find_last_of(" ");
				if (n == std::string::npos) {
					l_font_face = l_fontname;
					l_font_size = 0;
				} else {
					l_font_face = l_fontname.substr(0, n);
					l_font_size = as_int(l_fontname.substr(n+1));
				}
				component.set_font(l_font_face, l_font_size);
			}
			if (need_colour) {
				Gdk::RGBA c(m_bn_colour->get_rgba());
				component.set_fg_colour(c.get_red(), c.get_green(), c.get_blue());
			}
		}
		close();
	}

	// build a dialog from the Glade builder file.
	ContextDialog::ContextDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder):
		Gtk::Dialog(cobject), m_builder(refBuilder)
	{
		m_ok_button = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_builder->get_object("ctx_bn_ok"));
		m_cancel_button = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_builder->get_object("ctx_bn_cancel"));

		m_lb_name = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_name"));
		m_lb_voltage = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_voltage"));
		m_lb_resistance = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_resistance"));
		m_lb_capacitance = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_capacitance"));
		m_lb_inductance = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_inductance"));
		m_lb_inverted = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_invert"));
		m_lb_gate_invert = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_gate_invert"));
		m_lb_orientation = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_orientation"));
		m_lb_scale = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_scale"));
		m_lb_trigger = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_trigger"));
		m_lb_position = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_position"));
		m_lb_rows = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_rows"));
		m_lb_inputs = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_inputs"));
		m_lb_selectors = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_selectors"));
		m_lb_attributes = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_attributes"));
		m_lb_switch = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_switch"));
		m_lb_font = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_font"));
		m_lb_colour = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_builder->get_object("ctx_lb_colour"));

		m_name = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_name"));
		m_voltage = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_volt"));
		m_resistance = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_ohm"));
		m_capacitance = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_farad"));
		m_inductance = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_henry"));
		m_voltage_unit = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_builder->get_object("ctx_p_unit"));
		m_resistance_unit = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_builder->get_object("ctx_r_unit"));
		m_capacitance_unit = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_builder->get_object("ctx_f_unit"));
		m_inductance_unit = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_builder->get_object("ctx_h_unit"));

		m_box_inverted = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_inverted"));
		m_rb_inverted = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_inverted"));

		m_box_gate_inverted = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_gate_invert"));
		m_rb_gate_inverted = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_gate_invert"));

		m_box_orientation = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_direction"));
		m_rb_dir_fwd = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_direction_right"));
		m_rb_dir_back = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_direction_left"));
		m_rb_dir_up = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_direction_up"));
		m_rb_dir_dn = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_direction_down"));

		m_scale = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_scale"));

		m_box_trigger = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_positive_trigger"));
		m_rb_trigger_pos = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_trigger_positive"));

		m_box_position = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_pos"));
		m_box_size = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_size"));

		m_posn_x = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_xpos"));
		m_posn_y = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_ypos"));
		m_size_w = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_width"));
		m_size_h = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_height"));

		m_entry_rows = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_rows"));
		m_entry_inputs = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_inputs"));
		m_entry_selectors = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("ctx_selectors"));

		m_box_attributes = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_attributes"));
		m_ntype = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_ntype"));
		m_synchronous = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_synchronous"));
		m_first = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_first"));
		m_join = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_join"));
		m_invert = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_invert"));
		m_underscore = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_underscore"));
		m_overscore = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_overscore"));
		m_bold = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(m_builder->get_object("ctx_bold"));

		m_box_switch = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_builder->get_object("ctx_box_switch"));
		m_rb_switch_open = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(m_builder->get_object("ctx_switch_open"));

		m_bn_font = Glib::RefPtr<Gtk::FontButton>::cast_dynamic(m_builder->get_object("ctx_font"));
		m_bn_colour = Glib::RefPtr<Gtk::ColorButton>::cast_dynamic(m_builder->get_object("ctx_colour"));

		m_ok_button->signal_clicked().connect(sigc::mem_fun(*this, &ContextDialog::on_ok_clicked));
		m_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &ContextDialog::on_cancel_clicked));

		m_window = Glib::RefPtr<Gtk::Window>::cast_dynamic(m_builder->get_object("sim16f_main"));
		Window *w = m_window.operator->();
		this->set_transient_for(*w);
	}

}
