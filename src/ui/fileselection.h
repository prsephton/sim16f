#pragma once
#include <gtkmm.h>
#include <iostream>
#include "../utils/hex.h"

namespace app {

	class FileSelection: public Gtk::FileChooserDialog {
		Glib::RefPtr<Gtk::Builder> m_builder;

	protected:
		Glib::RefPtr<Gtk::Button> m_ok_button;
		Glib::RefPtr<Gtk::Button> m_cancel_button;
		Glib::RefPtr<Gtk::Entry> m_filename;
		Glib::RefPtr<Gtk::FileFilter> m_hex_filter;
		Glib::RefPtr<Gtk::FileFilter> m_asm_filter;

		void on_ok_clicked() {
			response(Gtk::RESPONSE_OK);
		}

		void on_cancel_clicked() {
			response(Gtk::RESPONSE_CANCEL);
		}

		void on_selection_changed() {
			m_filename->set_text(get_filename());
		}

		void on_response(int a_response) {
			hide();
		}

		std::string save_file(const std::string &a_filename) {
			set_filename(a_filename);
			set_title("Please select a destination file name");
 			m_filename->set_can_focus(true);
			int response = run();
			if (response == Gtk::RESPONSE_OK) return m_filename->get_text();
			return "";
		}

		std::string load_file() {
 			set_title("Please select a file to load");
 			m_filename->set_can_focus(false);
			int response = run();
			if (response == Gtk::RESPONSE_OK) return m_filename->get_text();
			return "";
		}

	  public:
		FileSelection(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder):
			FileChooserDialog(cobject), m_builder(refBuilder) {

			m_ok_button = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_builder->get_object("file_select_ok"));
			m_cancel_button = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_builder->get_object("file_select_cancel"));
			m_filename = Glib::RefPtr<Gtk::Entry>::cast_dynamic(m_builder->get_object("file_select_filename"));
			m_hex_filter = Glib::RefPtr<Gtk::FileFilter>::cast_dynamic(m_builder->get_object("hex_chooser"));
			m_asm_filter = Glib::RefPtr<Gtk::FileFilter>::cast_dynamic(m_builder->get_object("asm_chooser"));

			m_ok_button->signal_clicked().connect(sigc::mem_fun(*this, &FileSelection::on_ok_clicked));
			m_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &FileSelection::on_cancel_clicked));
			signal_selection_changed().connect(sigc::mem_fun(*this, &FileSelection::on_selection_changed));
			this->signal_response().connect(sigc::mem_fun(*this, &FileSelection::on_response));

		}

		std::string save_hex_file(const std::string &a_filename) {
			set_filter(m_hex_filter);
			return save_file(a_filename);
		}

		std::string load_hex_file() {
			set_filter(m_hex_filter);
			return load_file();
		}

		std::string save_asm_file(const std::string &a_filename) {
			set_filter(m_asm_filter);
			return save_file(a_filename);
		}

		std::string load_asm_file() {
			set_filter(m_asm_filter);
			return load_file();
		}
	};

}
