/*
 * Context Dialog
 *=================
 *
 * A single dialog with various editable settings.  Depending on the context, the dialog
 * will adapt by hiding some fields and showing others, so that only relevant data is
 * requested at runtime.
 *
 * When a context operation is selected on an element such as a symbol, text item, point
 * or line, the first step is always to identify the object at that location.  The
 * WHATS_AT data will contain enough information to properly identify the selected item.
 *
 * For each type of element, and for each dialog item _xxx, we will define a method
 * needs_xxx() returning a boolean true value if the element requires the data, and a
 * set_xxx() method to allow the data to be updated.
 * The needs_xxx() method will also take a reference of the approriate type if applicable,
 * to return the current value of _xxx for the dialog to populate default.
 *
 * For example, a default symbol would define
 *     virtual bool needs_name(std::string &current_name);
 *        and
 *     virtual void set_name(const std::string &new_name);
 *
 * With this arrangement, we will build the default behaviour into each base class, and
 * specialisations will contain only the code it needs to manage configuration specific
 * to each type.
 *
 */
#pragma once

#include <gtkmm.h>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "../../utils/smart_ptr.h"

namespace app {

	//_______________________________________________________________________________________________
	// A class which should support configurable data would override at least two of these
	// methods.  One (needs_xxx) method tells the dialog that we are interested in configuring the attribute,
    // and the other (set_xxx) twin method will be called to update the value.
    // the (needs_xxx) method is passed a value by reference; that value should be updated by the
    // implementation to contain the existing value, and the dialog will show this value as a default.
	struct Configurable {
		virtual bool needs_name(std::string &a_name){ return false; }
		virtual bool needs_voltage(double &V){ return false; }
		virtual bool needs_resistance(double &R){ return false; }
		virtual bool needs_capacitance(double &C){ return false; }
		virtual bool needs_inductance(double &H){ return false; }
		virtual bool needs_inverted(bool &inverted){ return false; }
		virtual bool needs_gate_inverted(bool &inverted){ return false; }
		virtual bool needs_orientation(double &orientation){ return false; }
		virtual bool needs_forward(){ return false; }
		virtual bool needs_back(){ return false; }
		virtual bool needs_up(){ return false; }
		virtual bool needs_down(){ return false; }
		virtual bool needs_scale(double &scale){ return false; }
		virtual bool needs_trigger(bool &positive){ return false; }
		virtual bool needs_position(double &x, double &y){ return false; }
		virtual bool needs_size(double &w, double &h){ return false; }
		virtual bool needs_rows(int &rows){ return false; }
		virtual bool needs_inputs(int &inputs){ return false; }
		virtual bool needs_selectors(int &selectors){ return false; }
		virtual bool needs_ntype(bool &ntype){ return false; }
		virtual bool needs_synchronous(bool &synchronous){ return false; }
		virtual bool needs_first(bool &first){ return false; }
		virtual bool needs_join(bool &join){ return false; }
		virtual bool needs_invert(bool &invert){ return false; }
		virtual bool needs_underscore(bool &unserscore){ return false; }
		virtual bool needs_overscore(bool &overscore){ return false; }
		virtual bool needs_bold(bool &bold){ return false; }
		virtual bool needs_switch(bool &on){ return false; }
		virtual bool needs_font(std::string &font_face, float &font_size){ return false; }
		virtual bool needs_fg_colour(double &r, double &g, double &b){ return false; }
		virtual bool needs_xor(bool &a_xor){ return false; }

		virtual void set_xor(int a_xor){}
		virtual void set_name(const std::string &a_name){}
		virtual void set_voltage(double V){}
		virtual void set_resistance(double R){}
		virtual void set_capacitance(double C){}
		virtual void set_inductance(double H){}
		virtual void set_inverted(bool inverted){}
		virtual void set_gate_inverted(bool inverted){}
		virtual void set_orientation(double orientation){}
		virtual void set_scale(double scale){}
		virtual void set_trigger(bool positive){}
		virtual void set_position(double x, double y){}
		virtual void set_size(double w, double h){}
		virtual void set_rows(int rows){}
		virtual void set_inputs(int inputs){}
		virtual void set_selectors(int selectors){}
		virtual void set_ntype(bool ntype){}
		virtual void set_synchronous(bool synchronous){}
		virtual void set_first(bool first){}
		virtual void set_join(bool join){}
		virtual void set_invert(bool invert){}
		virtual void set_underscore(bool unserscore){}
		virtual void set_overscore(bool overscore){}
		virtual void set_bold(bool bold){}
		virtual void set_switch(bool on){}
		virtual void set_font(const std::string &font_face, float font_size){}
		virtual void set_fg_colour(double r, double g, double b){}

		Configurable() {}
		virtual ~Configurable() {}
	};


	//_______________________________________________________________________________________________
	// The context dialog is defined entirely in Glade, and will be initialised at startup time.
	class ContextDialog: public Gtk::Dialog {
		Glib::RefPtr<Gtk::Builder> m_builder;

	protected:
		Glib::RefPtr<Gtk::Window> m_window;
		Glib::RefPtr<Gtk::Button> m_ok_button;
		Glib::RefPtr<Gtk::Button> m_cancel_button;

		Glib::RefPtr<Gtk::Label> m_lb_name;
		Glib::RefPtr<Gtk::Label> m_lb_voltage;
		Glib::RefPtr<Gtk::Label> m_lb_resistance;
		Glib::RefPtr<Gtk::Label> m_lb_capacitance;
		Glib::RefPtr<Gtk::Label> m_lb_inductance;
		Glib::RefPtr<Gtk::Label> m_lb_inverted;
		Glib::RefPtr<Gtk::Label> m_lb_gate_invert;
		Glib::RefPtr<Gtk::Label> m_lb_orientation;
		Glib::RefPtr<Gtk::Label> m_lb_scale;
		Glib::RefPtr<Gtk::Label> m_lb_trigger;
		Glib::RefPtr<Gtk::Label> m_lb_position;
		Glib::RefPtr<Gtk::Label> m_lb_rows;
		Glib::RefPtr<Gtk::Label> m_lb_inputs;
		Glib::RefPtr<Gtk::Label> m_lb_selectors;
		Glib::RefPtr<Gtk::Label> m_lb_attributes;
		Glib::RefPtr<Gtk::Label> m_lb_switch;
		Glib::RefPtr<Gtk::Label> m_lb_font;
		Glib::RefPtr<Gtk::Label> m_lb_colour;

		Glib::RefPtr<Gtk::Entry> m_name;
		Glib::RefPtr<Gtk::Entry> m_voltage;
		Glib::RefPtr<Gtk::Entry> m_resistance;
		Glib::RefPtr<Gtk::Entry> m_capacitance;
		Glib::RefPtr<Gtk::Entry> m_inductance;
		Glib::RefPtr<Gtk::ComboBoxText> m_voltage_unit;
		Glib::RefPtr<Gtk::ComboBoxText> m_resistance_unit;
		Glib::RefPtr<Gtk::ComboBoxText> m_capacitance_unit;
		Glib::RefPtr<Gtk::ComboBoxText> m_inductance_unit;

		Glib::RefPtr<Gtk::Box> m_box_inverted;
		Glib::RefPtr<Gtk::RadioButton> m_rb_inverted;

		Glib::RefPtr<Gtk::Box> m_box_gate_inverted;
		Glib::RefPtr<Gtk::RadioButton> m_rb_gate_inverted;

		Glib::RefPtr<Gtk::Box> m_box_orientation;
		Glib::RefPtr<Gtk::RadioButton> m_rb_dir_fwd;
		Glib::RefPtr<Gtk::RadioButton> m_rb_dir_back;
		Glib::RefPtr<Gtk::RadioButton> m_rb_dir_up;
		Glib::RefPtr<Gtk::RadioButton> m_rb_dir_dn;

		Glib::RefPtr<Gtk::Entry> m_scale;

		Glib::RefPtr<Gtk::Box> m_box_trigger;
		Glib::RefPtr<Gtk::RadioButton> m_rb_trigger_pos;

		Glib::RefPtr<Gtk::Box> m_box_position;
		Glib::RefPtr<Gtk::Box> m_box_size;

		Glib::RefPtr<Gtk::Entry> m_posn_x;
		Glib::RefPtr<Gtk::Entry> m_posn_y;
		Glib::RefPtr<Gtk::Entry> m_size_w;
		Glib::RefPtr<Gtk::Entry> m_size_h;

		Glib::RefPtr<Gtk::Entry> m_entry_rows;
		Glib::RefPtr<Gtk::Entry> m_entry_inputs;
		Glib::RefPtr<Gtk::Entry> m_entry_selectors;

		Glib::RefPtr<Gtk::Box> m_box_attributes;
		Glib::RefPtr<Gtk::CheckButton> m_ntype;
		Glib::RefPtr<Gtk::CheckButton> m_synchronous;
		Glib::RefPtr<Gtk::CheckButton> m_first;
		Glib::RefPtr<Gtk::CheckButton> m_join;
		Glib::RefPtr<Gtk::CheckButton> m_invert;
		Glib::RefPtr<Gtk::CheckButton> m_underscore;
		Glib::RefPtr<Gtk::CheckButton> m_overscore;
		Glib::RefPtr<Gtk::CheckButton> m_bold;
		Glib::RefPtr<Gtk::CheckButton> m_is_xor;

		Glib::RefPtr<Gtk::Box> m_box_switch;
		Glib::RefPtr<Gtk::RadioButton> m_rb_switch_open;

		Glib::RefPtr<Gtk::FontButton> m_bn_font;
		Glib::RefPtr<Gtk::ColorButton> m_bn_colour;

		void on_ok_clicked() {
			response(Gtk::RESPONSE_OK);
		}
		void on_cancel_clicked() { response(Gtk::RESPONSE_CANCEL); }

		static std::string as_text(double a_value);
		static std::string as_text(int a_value);
		static double as_double(std::string a_text);
		static int as_int(std::string a_text);

		struct scaled_value {
			Glib::RefPtr<Gtk::Entry> entry;
			Glib::RefPtr<Gtk::ComboBoxText> cbox;
			int max_unit;

			void set_from_value(double a_value);
			double value();
			scaled_value(Glib::RefPtr<Gtk::Entry> a_entry, Glib::RefPtr<Gtk::ComboBoxText> a_cbox, int a_max_unit);
		};

	  public:

		void configure(Configurable &component);

		ContextDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder);

	};


	//_______________________________________________________________________________________________________________
	// A factory for this dialog.  We re-use the same one, and we need a single global instance
	class ContextDialogFactory {
		ContextDialog *m_ctx_dialog;   // singleton
		static SmartPtr<ContextDialogFactory> factory;

	  public:
		ContextDialog *dialog() { return m_ctx_dialog; }

		// component is a vistor to the ContextDialog
		void popup_context(Configurable &component) {
			dialog()->configure(component);
		}
		ContextDialogFactory() : m_ctx_dialog(factory->dialog()) {}

		// this constructor gets called once only at startup, and initialises factory.
		ContextDialogFactory(const Glib::RefPtr<Gtk::Builder>& a_refGlade) {
			a_refGlade->get_widget_derived("dlg_context", m_ctx_dialog);
			factory = this;
		}
	};

}


