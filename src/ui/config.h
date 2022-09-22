//______________________________________________________________________________________________________
// Configuration word editor, and also a load/save interface for .hex files.

#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "../devices/flags.h"
#include "../cpu_data.h"
#include "../utils/utility.h"
#include "../utils/file_config.h"
#include "fileselection.h"

namespace app {

	class Config: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		FileConfig cfg;

		class BitConfig: public Gtk::CheckButton {
		  private:
			WORD *m_config;
			WORD  m_mask;

			void on_signal_toggle() {
				WORD &c = *m_config;
				if (this->get_active())
					c = c | m_mask ;
				else
					c = c & (~m_mask) ;
			}

		  public:
			BitConfig(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder):
				CheckButton(cobject),
				m_config(0), m_mask(0) {
				this->signal_toggled().connect(sigc::mem_fun(*this, &Config::BitConfig::on_signal_toggle));
			}

			inline void refresh(WORD &a_config) {
				this->set_active(a_config & m_mask);
			}

			void set_up(WORD &a_config, WORD a_mask) {
				m_config = &a_config;
				m_mask = a_mask;
				refresh(*m_config);
			}
		};

	  protected:
		std::map<std::string, BitConfig *>m_bits;
		Glib::RefPtr<Gtk::ComboBoxText> m_fosc;
		Glib::RefPtr<Gtk::ComboBoxText> m_hz;
		Glib::RefPtr<Gtk::Button> m_save_hex;
		Glib::RefPtr<Gtk::Button> m_load_hex;
		Glib::RefPtr<Gtk::Button> m_load_assembler;
		Glib::RefPtr<Gtk::Button> m_save_assembler;
		FileSelection *m_file_chooser;
		std::string m_filename;

		BitConfig *make_BitConfig(const std::string &a_name, WORD a_mask) {
			BitConfig *bc;
			m_refGlade->get_widget_derived(a_name, bc);  // This is a terrible pattern.  Why does builder not expose the underlying
			if (bc) bc->set_up(m_cpu.Config, a_mask);    // GtkWidget?  Instead builder makes get_cwidget(name) a protected method.
			return bc;
		}

		void configure_bits() {
			m_bits["cp"]    = make_BitConfig("config_cp",    Flags::CONFIG::CP);
			m_bits["cpd"]   = make_BitConfig("config_cpd",   Flags::CONFIG::CPD);
			m_bits["boren"] = make_BitConfig("config_boren", Flags::CONFIG::BOREN);
			m_bits["lvp"]   = make_BitConfig("config_lvp",   Flags::CONFIG::LVP);
			m_bits["mclre"] = make_BitConfig("config_mclre", Flags::CONFIG::MCLRE);
			m_bits["pwrte"] = make_BitConfig("config_pwrte", Flags::CONFIG::PWRTE);
			m_bits["wdte"]  = make_BitConfig("config_wdte",  Flags::CONFIG::WDTE);
		}

		void set_title(const std::string &a_filename) {
			std::string l_title="PIC16fxxx Simulator";
			Glib::RefPtr<Gtk::Window> l_window = Glib::RefPtr<Gtk::Window>::cast_dynamic(m_refGlade->get_object("sim16f_main"));
			l_window->set_title(l_title + "  -   [" + a_filename + "]" );
		}

		const std::string base_name(const std::string &a_filename) {
			if (a_filename.rfind("/") != std::string::npos)
				return a_filename.substr(a_filename.rfind("/")+1);
			else if (a_filename.rfind("\\") != std::string::npos)
				return a_filename.substr(a_filename.rfind("\\")+1);
			return a_filename;
		}

		const std::string drop_ext(const std::string &a_filename) {
			return a_filename.substr(0, a_filename.find("."));
		}

		void on_fosc_changed() {
			WORD fosc_code = 7 - m_fosc->get_active_row_number();
			if (fosc_code & 0b100) {
				fosc_code = (1 << 4) | (fosc_code & 0b11);
			}
			m_cpu.Config = (m_cpu.Config & (~0b10011)) | fosc_code;
		}

		void on_hz_changed() {
			cfg.set_text("hz_choice", m_hz->get_active_id()); cfg.flush();
			WORD hz = as_int(m_hz->get_active_id());
			m_cpu.clock_delay_us = 1000000 / (2 * hz);
			cfg.set_float("frequency", hz); cfg.flush();
		}

		void on_save_hex_clicked() {

			std::string l_filename = m_file_chooser->save_hex_file(drop_ext(m_filename)+".hex");
			if (l_filename.length() and dump_hex(l_filename, m_cpu)) {
				std::cout << "Hex file " << l_filename << " successfully saved" << std::endl;
				set_title(base_name(l_filename));
				m_filename = l_filename;
				cfg.set_text("filename", l_filename); cfg.flush();
			}
		}

		void on_load_hex_clicked() {
			std::string l_filename = m_file_chooser->load_hex_file();
			try {
				if (l_filename.length() and load_hex(l_filename, m_cpu)) {
					std::cout << "Hex file " << l_filename << " successfully loaded" << std::endl;
					m_cpu.control.push(ControlEvent("reset"));
					set_title(base_name(l_filename));
					m_filename = l_filename;
					cfg.set_text("filename", l_filename); cfg.flush();
					configure_bits();
				}
			} catch(const std::string &err) {
				std::cout << "An error occurred while loading " << l_filename << ": " << err << std::endl;
			}
		}

		void on_save_assembler_clicked() {
			std::string l_filename = m_file_chooser->save_asm_file(drop_ext(m_filename)+".a");
			InstructionSet instructions;
			if (l_filename.length() and disassemble(l_filename, m_cpu, instructions)) {
				std::cout << "Assembler file " << l_filename << " successfully saved" << std::endl;
				set_title(base_name(l_filename));
				cfg.set_text("filename", l_filename); cfg.flush();
				m_filename = l_filename;
			}
		}

		void on_load_assembler_clicked() {
			std::string l_filename = m_file_chooser->load_asm_file();
			InstructionSet instructions;
			try {
				if (l_filename.length() and assemble(l_filename, m_cpu, instructions)) {
					std::cout << "Assembler file " << l_filename << " successfully loaded" << std::endl;
					m_cpu.control.push(ControlEvent("reset"));
					set_title(base_name(l_filename));
					m_filename = l_filename;
					cfg.set_text("filename", l_filename); cfg.flush();
					configure_bits();
				}
			} catch(const std::string &err) {
				std::cout << "An error occurred while assembling " << l_filename << ": " << err << std::endl;
			}
		}

		void apply_config() {
			if (not cfg.exists("cpu_model")) cfg.set_text("cpu_model", "pic16f628");
			if (not cfg.exists("filename"))  cfg.set_text("filename", "test.hex");
			if (not cfg.exists("frequency")) cfg.set_float("frequency", 4);
			if (not cfg.exists("hz_choice")) cfg.set_text("hz_choice", "4");

			m_filename = cfg.get_text("filename");
			set_title(base_name(m_filename));
			if (m_filename.find(".hex") != std::string::npos) {
				try {
					if (load_hex(m_filename, m_cpu))
						std::cout << "Hex file " << m_filename << " successfully loaded" << std::endl;
					m_cpu.control.push(ControlEvent("reset"));
				} catch (std::string &e) {
					std::cout << "Error loading hex file [" << e << "]" << std::endl;
				}
			} else if (m_filename.find(".a") != std::string::npos) {
				InstructionSet instructions;
				try {
					if (assemble(m_filename, m_cpu, instructions))
						std::cout << "File " << m_filename << " successfully assembled" << std::endl;
					m_cpu.control.push(ControlEvent("reset"));
				} catch (std::string &e) {
					std::cout << "Error loading assembly file [" << e << "]" << std::endl;
				}
			}
			m_hz->set_active_id(cfg.get_text("hz_choice"));
			WORD hz = cfg.get_float("frequency");
			m_cpu.clock_delay_us = 500000.0 / hz;
		};

	  public:
		void refresh() {
			for (auto i=m_bits.begin(); i != m_bits.end(); ++i) {
				i->second->refresh(m_cpu.Config);
			}
			WORD fosc_code = 7 - (((m_cpu.Config & 0b10000) >> 2) | (m_cpu.Config & 0b011));
			m_fosc->set_active(fosc_code);
		}

		Config(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade), cfg("sim16f.cfg") {
			configure_bits();

			m_fosc = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_refGlade->get_object("config_fosc"));
			m_fosc->signal_changed().connect(sigc::mem_fun(*this, &Config::on_fosc_changed));
			WORD fosc_code = 7 - (((m_cpu.Config & 0b10000) >> 2) | (m_cpu.Config & 0b011));
			m_fosc->set_active(fosc_code);

			m_hz = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(m_refGlade->get_object("config_hz"));
			m_hz->signal_changed().connect(sigc::mem_fun(*this, &Config::on_hz_changed));

			m_save_hex = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("save_hex"));
			m_load_hex = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("load_hex"));
			m_load_assembler = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("load_assembler"));
			m_save_assembler = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("save_assembler"));

			m_save_hex->signal_clicked().connect(sigc::mem_fun(*this, &Config::on_save_hex_clicked));
			m_load_hex->signal_clicked().connect(sigc::mem_fun(*this, &Config::on_load_hex_clicked));
			m_load_assembler->signal_clicked().connect(sigc::mem_fun(*this, &Config::on_load_assembler_clicked));
			m_save_assembler->signal_clicked().connect(sigc::mem_fun(*this, &Config::on_save_assembler_clicked));

			m_refGlade->get_widget_derived("file_chooser", m_file_chooser);
			apply_config();
		}
	};

};               // namespace app


