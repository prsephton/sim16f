
#include <iostream>
#include <sstream>
#include <map>
#include <bitset>
#include <queue>
#include "../cpu_data.h"
#include "../utils/utility.h"

namespace app {

	struct RegisterChange {
		int ofs;
		BYTE value;

		RegisterChange(int a_ofs, BYTE a_value): ofs(a_ofs), value(a_value) {};
	};

	class DisplayRegisters: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		std::map<int, Glib::RefPtr<Gtk::Label> > m_data_label;
		std::map<int, Glib::RefPtr<Gtk::Box> >m_flag;
		std::queue<RegisterChange> m_changes;

		Pango::AttrList m_normal;     //("<span font_weight='normal', background='none' />");
		Pango::AttrList m_selected;   //("<span font_weight='bold', background='#fce94f' />");
		bool m_exiting;

		virtual void exiting(){
			m_exiting = true;
		}

		void update_register(int a_ofs, BYTE a_value) {
			std::ostringstream obuf("");

			auto found_label = m_data_label.find(a_ofs);
			if (found_label != m_data_label.end()) {
				obuf.clear();
				obuf << "0b" << std::bitset<8>(a_value) << " [0x" << std::hex << std::setfill('0') << std::setw(2) << (int)a_value << "]";
				if (found_label->second)
					found_label->second->set_text(obuf.str());
				else
					std::cout << "Offset: " << std::hex << a_ofs << ": Data label not found" << std::endl;
			}
			auto found_box = m_flag.find(a_ofs);
			if (found_box != m_flag.end()) {
				if (found_box->second) {
					auto children = found_box->second->get_children();
					BYTE mask = 0b10000000;
					for (unsigned int n = 0; n < children.size(); ++n) {
						auto child = dynamic_cast<Gtk::Label *>(children[n]);
						if (child)
							child -> set_attributes((a_value & mask)?m_selected:m_normal);
						else
							std::cout << "Offset: " << std::hex << a_ofs << ": Child[" << n << "] label not found" << std::endl;
						mask >>= 1;
					}
				} else {
					std::cout << "Offset: " << std::hex << a_ofs << ": Box not found" << std::endl;
				}
			}
		}

		void on_register_changed(Register *r, const std::string &name, const std::vector<BYTE>&data) {
			m_changes.push(RegisterChange(r->index(), r->get_value()));
		}

		void update_from_sram() {
			for (auto data_label: m_data_label) {
				BYTE value = m_cpu.sram.read(data_label.first);
				update_register(data_label.first, value);
			}
		}

		bool process_queue(){
			if (m_changes.size()) {    // -jump through hoops to ensure gtk updates only in app thread.
				auto &change = m_changes.front();
				update_register(change.ofs, change.value);
				m_changes.pop();
			} else {
				sleep_for_us(100);
			}
			return !m_exiting;
		}

	  public:

		DisplayRegisters(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			m_data_label[SRAM::INDF] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_indf_data"));
			m_data_label[SRAM::INDF+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_indf_data2"));
			m_data_label[SRAM::TMR0] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_tmr0_data"));
			m_data_label[SRAM::PCL] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pcl_data"));
			m_data_label[SRAM::PCL+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pcl_data2"));
			m_data_label[SRAM::STATUS] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_status_data"));
			m_data_label[SRAM::STATUS+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_status_data2"));
			m_data_label[SRAM::FSR] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_fsr_data"));
			m_data_label[SRAM::FSR+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_fsr_data2"));
			m_data_label[SRAM::PORTA] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_porta_data"));
			m_data_label[SRAM::PORTB] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_portb_data"));
			m_data_label[SRAM::PCLATH] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pclath_data"));
			m_data_label[SRAM::PCLATH+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pclath_data2"));
			m_data_label[SRAM::INTCON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_intcon_data"));
			m_data_label[SRAM::INTCON+0x80] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_intcon_data2"));
			m_data_label[SRAM::PIR1] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pir1_data"));
			m_data_label[SRAM::TMR1L] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_tmr1l_data"));
			m_data_label[SRAM::TMR1H] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_tmr1h_data"));
			m_data_label[SRAM::T1CON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_t1con_data"));
			m_data_label[SRAM::TMR2] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_tmr2_data"));
			m_data_label[SRAM::T2CON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_t2con_data"));
			m_data_label[SRAM::CCPR1L] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_ccpr1l_data"));
			m_data_label[SRAM::CCPR1H] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_ccpr1h_data"));
			m_data_label[SRAM::CCP1CON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_ccp1con_data"));
			m_data_label[SRAM::RCSTA] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_rcsta_data"));
			m_data_label[SRAM::TXREG] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_txreg_data"));
			m_data_label[SRAM::RCREG] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_rcreg_data"));
			m_data_label[SRAM::CMCON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_cmcon_data"));
			m_data_label[SRAM::OPTION] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_option_data"));
			m_data_label[SRAM::TRISA] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_trisa_data"));
			m_data_label[SRAM::TRISB] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_trisb_data"));
			m_data_label[SRAM::PIE1] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pie1_data"));
			m_data_label[SRAM::PCON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pcon_data"));
			m_data_label[SRAM::PR2] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_pr2_data"));
			m_data_label[SRAM::TXSTA] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_txsta_data"));
			m_data_label[SRAM::SPBRG] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_spbrg_data"));
			m_data_label[SRAM::EEDATA] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_eedata_data"));
			m_data_label[SRAM::EEADR] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_eeadr_data"));
			m_data_label[SRAM::EECON1] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_eecon1_data"));
			m_data_label[SRAM::EECON2] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_eecon2_data"));
			m_data_label[SRAM::VRCON] = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("lb_vrcon_data"));

			m_flag[SRAM::STATUS] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_status_reg"));
			m_flag[SRAM::STATUS+0x80] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_status_reg2"));
			m_flag[SRAM::PORTA] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_porta_reg"));
			m_flag[SRAM::PORTB] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_portb_reg"));
			m_flag[SRAM::INTCON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_intcon_reg"));
			m_flag[SRAM::INTCON+0x80] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_intcon_reg2"));
			m_flag[SRAM::PIR1] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_pir1_reg"));
			m_flag[SRAM::T1CON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_t1con_reg"));
			m_flag[SRAM::T2CON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_t2con_reg"));
			m_flag[SRAM::CCP1CON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_ccp1con_reg"));
			m_flag[SRAM::RCSTA] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_rcsta_reg"));
			m_flag[SRAM::CMCON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_cmcon_reg"));
			m_flag[SRAM::OPTION] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_option_reg"));
			m_flag[SRAM::TRISA] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_trisa_reg"));
			m_flag[SRAM::TRISB] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_trisb_reg"));
			m_flag[SRAM::PIE1] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_pie1_reg"));
			m_flag[SRAM::PCON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_pcon_reg"));
			m_flag[SRAM::TXSTA] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_txsta_reg"));
			m_flag[SRAM::EECON1] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_eecon1_reg"));
			m_flag[SRAM::VRCON] = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("box_vrcon_reg"));


			Pango::Attribute background = Pango::Attribute::create_attr_background(0xfcfc, 0xe9e9, 0x4f4f);
			Pango::Attribute weight = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
			m_selected.insert(background);
			m_selected.insert(weight);

			update_from_sram();

			DeviceEvent<Register>::subscribe<DisplayRegisters>(this, &DisplayRegisters::on_register_changed);

			m_exiting = false;
			Glib::signal_idle().connect( sigc::mem_fun(*this, &DisplayRegisters::process_queue) );

		}

		~DisplayRegisters(){
			DeviceEvent<Register>::unsubscribe<DisplayRegisters>(this, &DisplayRegisters::on_register_changed);
		}
	};

}

