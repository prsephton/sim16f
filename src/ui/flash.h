#pragma once

#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include "datagrid.h"
#include "../instructions.h"
#include "../utils/assembler.h"

namespace app {


	class Flash: public Component  {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		Glib::RefPtr<Gtk::TextView> m_assembly;
		bool m_exiting;

		virtual void exiting(){
			m_exiting = true;
		}

	  protected:
		std::vector<Glib::RefPtr<Gtk::TextMark>> m_marks;
		Glib::RefPtr<Gtk::TextBuffer::TagTable> m_tags;
		Glib::RefPtr<Gtk::TextBuffer> m_listing;
		Glib::RefPtr<Gtk::Button> m_play;
		Glib::RefPtr<Gtk::Button> m_pause;
		Glib::RefPtr<Gtk::Button> m_next;
		Glib::RefPtr<Gtk::Button> m_back;
		Glib::RefPtr<Gtk::Label> m_w;
		Glib::RefPtr<Gtk::Label> m_pc;
		Glib::RefPtr<Gtk::Label> m_sp;
		Glib::RefPtr<Gtk::Label> m_carry;
		Glib::RefPtr<Gtk::Label> m_zero;
		Glib::RefPtr<Gtk::Label> m_digit_carry;
		Glib::RefPtr<Gtk::Label> m_bank_1;
		Glib::RefPtr<Gtk::Label> m_bank_2;
		Glib::RefPtr<Gtk::Label> m_bank_3;
		Glib::RefPtr<Gtk::Label> m_bank_4;

		DataGrid *grid;
		std::queue<CpuEvent> m_cpu_events;
		int m_active_pc;


		void on_flash_play() {
			m_cpu.control.push(ControlEvent("play"));
		}
		void on_flash_pause() {
			m_cpu.control.push(ControlEvent("pause"));
		}
		void on_flash_next() {
			m_cpu.control.push(ControlEvent("next"));
		}
		void on_flash_back() {
			m_cpu.control.push(ControlEvent("back"));
		}

		void set_toolbar_style() {
			auto cstyle = Gtk::CssProvider::create();
			cstyle->load_from_data(".label:selected { color: #2020ff; background: #afaf3f; }");
			auto toolbox = Glib::RefPtr<Gtk::Box>::cast_dynamic(m_refGlade->get_object("flash_toolbox"));
			auto style = toolbox->get_style_context();
			style->add_provider(cstyle, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}

		void set_up_toolbar() {
			m_pc = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_pc"));
			m_sp = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_sp"));
			m_w = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_w"));
			m_play = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("flash_play"));
			m_pause = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("flash_pause"));
			m_next = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("flash_next"));
			m_back = Glib::RefPtr<Gtk::Button>::cast_dynamic(m_refGlade->get_object("flash_back"));

			m_play->signal_clicked().connect(sigc::mem_fun(*this, &Flash::on_flash_play));
			m_pause->signal_clicked().connect(sigc::mem_fun(*this, &Flash::on_flash_pause));
			m_next->signal_clicked().connect(sigc::mem_fun(*this, &Flash::on_flash_next));
			m_back->signal_clicked().connect(sigc::mem_fun(*this, &Flash::on_flash_back));

			m_carry = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_status_carry"));
			m_digit_carry = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_status_digit_carry"));
			m_zero = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_status_zero"));
			m_bank_1 = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_bank_1"));
			m_bank_2 = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_bank_2"));
			m_bank_3 = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_bank_3"));
			m_bank_4 = Glib::RefPtr<Gtk::Label>::cast_dynamic(m_refGlade->get_object("flash_bank_4"));

			set_toolbar_style();
		}

		void set_up_assembly() {
			m_assembly = Glib::RefPtr<Gtk::TextView>::cast_dynamic(m_refGlade->get_object("flash_assembly"));

			m_tags = Gtk::TextBuffer::TagTable::create();
			auto left20 = Gtk::TextTag::create("left20");
			left20->property_left_margin() = 20;
			m_tags->add(left20);

			auto highlight = Gtk::TextTag::create("highlight");
			highlight->property_background_rgba() = Gdk::RGBA("rgba(200,200,255,255)");
			m_tags->add(highlight);

			auto normal = Gtk::TextTag::create("normal");
			normal->property_background_rgba() = Gdk::RGBA("rgba(0,0,0,0)");
			m_tags->add(normal);

			auto bold = Gtk::TextTag::create("bold");
			bold->property_weight() = Pango::WEIGHT_BOLD;
			m_tags->add(bold);
			auto italic = Gtk::TextTag::create("italic");
			italic->property_style() = Pango::STYLE_ITALIC;
			m_tags->add(italic);
			auto l_tabs = Pango::TabArray(5, true);
			l_tabs.set_tab(0, Pango::TAB_LEFT, 80);
			l_tabs.set_tab(1, Pango::TAB_LEFT, 150);
			l_tabs.set_tab(2, Pango::TAB_LEFT, 240);
			l_tabs.set_tab(3, Pango::TAB_LEFT, 400);
			l_tabs.set_tab(4, Pango::TAB_LEFT, 600);
			m_assembly->set_tabs(l_tabs);
			m_listing = Gtk::TextBuffer::create(m_tags);
			m_assembly->set_buffer(m_listing);
		}

	  public:
		Flash(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade), m_exiting(false) {

			auto flash_ra = new DeviceRandomAccessAdapter(a_cpu.flash);
			grid = new DataGrid(*flash_ra, m_refGlade, "flash_grid", "flash_scroll", 4);

			set_up_assembly();
			set_up_toolbar();

			Glib::signal_idle().connect( sigc::mem_fun(*this, &Flash::process_queue) );
			CpuEvent::subscribe((void *)this, &Flash::pc_monitor);

			reset();
		}
		~Flash() {
			delete grid;
		}


		void reset() {
			InstructionSet instructions;
			std::vector<Disasm> d;
			disassemble(m_cpu, instructions, d);
			typedef std::vector<Disasm>::iterator each_instruction;
			std::stringbuf buffer;
			std::ostream os(&buffer);
			m_listing->set_text("");
			m_marks.clear();
			m_active_pc = -1;
			for (each_instruction i = d.begin(); i != d.end(); ++i) {
				Glib::RefPtr<Gtk::TextMark> start = m_listing->create_mark(m_listing->end());
				m_marks.push_back(start);
				os << std::hex << std::setfill('0') << std::setw(4) << i->PC << ":\t";
				os.flush();
				m_listing->insert(m_listing->end(), buffer.str());
				buffer = std::stringbuf(); os.rdbuf(&buffer);

				m_listing->apply_tag(m_tags->lookup("bold"), start->get_iter(), m_listing->end());
				auto mnemonic = m_listing->create_mark(m_listing->end());
				os << i->astext << "\t";
				os.flush();
				m_listing->insert(m_listing->end(), buffer.str());
				buffer = std::stringbuf(); os.rdbuf(&buffer);

				auto comment = m_listing->create_mark(m_listing->end());
				os << "opcode: " << std::hex << std::setfill('0') << std::setw(4)  << i->opcode << "\n";
				os.flush();
				m_listing->insert(m_listing->end(), buffer.str());
				m_listing->apply_tag(m_tags->lookup("italic"), comment->get_iter(), m_listing->end());
				m_listing->apply_tag(m_tags->lookup("left20"), start->get_iter(), m_listing->end());
				buffer = std::stringbuf(); os.rdbuf(&buffer);
			}
		}

		void apply_highlight(const CpuEvent &e, WORD pc, bool apply) {
			if (pc < m_marks.size()) {
				auto mark = m_marks[pc];
				m_assembly->scroll_to(mark, 0.10);
//				std::cout << "iter: " << mark->get_iter().get_text(eol) << "\t" << (apply?"highlight\n":"remove\n");
				if (apply) {
					auto mne = mark->get_iter(); mne.forward_chars(6);
					auto comment = mne; comment.forward_chars(15);

					m_listing->erase(mne, comment);
					mne = mark->get_iter(); mne.forward_chars(6);
					m_listing->insert(mne, e.disassembly.substr(0, 15));

					auto stx = mark->get_iter();
					auto eol = stx; eol.forward_line();
					m_listing->apply_tag(m_tags->lookup("highlight"), stx, eol);
				} else {
					auto stx = mark->get_iter();
					auto eol = stx; eol.forward_line();
					m_listing->remove_tag(m_tags->lookup("highlight"), stx, eol);
				}
			}
		}

		bool process_queue(){
			if (!m_cpu_events.empty()) {
				CpuEvent e = m_cpu_events.front();
				m_cpu_events.pop();
				if (m_active_pc >= 0) {
					apply_highlight(e, m_active_pc, false);
					grid->position_for(m_active_pc, false);
				}
				apply_highlight(e, e.PC, true);
				m_active_pc = e.PC;
				grid->position_for(e.PC, true);
				m_pc->set_text(int_to_hex(e.PC, "", "h"));
				m_sp->set_text(int_to_hex(e.SP, "", "h"));
				m_w->set_text(int_to_hex(e.W, "", "h"));

				BYTE status = m_cpu.sram.status();

				m_carry->set_state((status & 1)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);
				m_digit_carry->set_state((status & 2)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);
				m_zero->set_state((status & 4)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);

				BYTE bank = m_cpu.sram.bank();
				m_bank_1->set_state((bank==0)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);
				m_bank_2->set_state((bank==1)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);
				m_bank_3->set_state((bank==2)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);
				m_bank_4->set_state((bank==3)?Gtk::STATE_SELECTED:Gtk::STATE_NORMAL);

			}
			return !m_exiting;
		}

		void cpu_event(const CpuEvent &e) {
			m_cpu_events.push(e);  // Callback does not happen in application thread.
		}

		static void pc_monitor(void *ob, const CpuEvent &e) {
			Flash *flash = (Flash *)(ob);
			flash->cpu_event(e);
		}
	};
}				// namespace app
