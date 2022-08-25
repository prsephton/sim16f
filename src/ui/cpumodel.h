#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "paint/cairo_drawing.h"
#include "../utils/smart_ptr.h"
#include "../utils/utility.h"
#include "paint/common.h"
#include "paint/diagrams.h"

namespace app {

	class FlashDiagram: public BlockDiagram {
		CPU_DATA &m_cpu;
		const WORD &m_execPC;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			const int LINE_HEIGHT = 12;
			const int LINES = 5;
			int pc = m_execPC - (LINES / 2);
			pc = pc < 0? 0: pc;
			for (int n = 0; n < LINES; ++n) {
				WORD d = m_cpu.flash.fetch(pc+n);
				cr->move_to(5,  35+LINE_HEIGHT*n);
				cr->text_path(int_to_hex(pc+n));
				cr->move_to(35, 35+LINE_HEIGHT*n);
				cr->text_path(int_to_hex(d));
				cr->set_line_width((pc+n == m_execPC)?0.9:0.4);
				cr->fill_preserve();
				cr->stroke();
			}
		}

	  public:
		FlashDiagram(CPU_DATA &a_cpu, const WORD &execPC, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "FLASH", a_area), m_cpu(a_cpu), m_execPC(execPC) {
		}
	};


	class PCDiagram: public BlockDiagram {
		const WORD &m_execPC;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->move_to(5, 30);
			cr->text_path(int_to_hex(m_execPC));
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		PCDiagram(const WORD &a_execPC, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "Program Counter", a_area), m_execPC(a_execPC) {
		}
	};

	class FSRDiagram: public BlockDiagram {
		const CPU_DATA &m_cpu;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->move_to(5, 30);
			cr->text_path("Value: ");
			cr->text_path(int_to_hex(m_cpu.sram.fsr()));
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		FSRDiagram(const CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "FSR", a_area), m_cpu(a_cpu) {
		}
	};


	class PortDiagram: public BlockDiagram {
		const CPU_DATA &m_cpu;
		double x, y, w, h;

		BYTE m_tris = 0xff;
		BYTE m_port = 0;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			do_draw(cr);
		}

		virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {}
		void register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
			on_register_change(r, name, data);
		}

	  public:

		void tris(BYTE t) { m_tris = t; }
		BYTE tris() const { return m_tris; }

		void port(BYTE p) { m_port = p; }
		BYTE port() const { return m_port; }

		void do_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_line_width(0.7);
			cr->rectangle(5,5,w-10,h-10);
			cr->stroke();
			cr->move_to(5 + (w-10)/3, 5);
			cr->line_to(5 + (w-10)/3, h-5);
			cr->move_to(5 + 2*(w-10)/3, 5);
			cr->line_to(5 + 2*(w-10)/3, h-5);
			cr->stroke();
		}

		virtual ~PortDiagram() {
			DeviceEvent<Register>::unsubscribe<PortDiagram>(this, &PortDiagram::register_change);
		}

		PortDiagram(CPU_DATA &a_cpu, const std::string &name, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "", a_area), m_cpu(a_cpu), x(x), y(y), w(width), h(height) {

			add(BlockDiagram::text(0, -2, name));

			DeviceEvent<Register>::subscribe<PortDiagram>(this, &PortDiagram::register_change);
		}
	};

	class PortADiagram: public PortDiagram {
		const CPU_DATA &cpu;
		int margin;
		double dh;

		std::stack<PinDiagram *> pd;
		std::vector<ConnectionDiagram *>pins;

		virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
			if (name == "TRISA") tris(data[Register::DVALUE::NEW]);
			if (name == "PORTA") port(data[Register::DVALUE::NEW]);
		}

		void on_pin_change(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
			redraw();
		}

	  public:

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			PortDiagram::do_draw(cr);
			const PINS &pins = cpu.pins;

			cr->save();
			BYTE trisA = tris();
			BYTE portA = port();
			for (int n = 0; n < 8; ++n) {
				bool trisflag = trisA & 1;
				bool portflag = portA & 1;

				cr->move_to(9, margin - 5 + (n+1) * dh);
				cr->text_path(portflag ? "1" : "0");

				cr->move_to(22, margin - 5 + (n+1) * dh);
				cr->text_path(trisflag ? "i" : "o");

				cr->move_to(35, margin - 5 + (n+1) * dh);
				int pin_no = cpu.porta.pin_numbers[n];
				cr->text_path(pins[pin_no].signal() ? "1" : "0");

				trisA = trisA >> 1;
				portA = portA >> 1;
			}
			cr->set_line_width(0.5);
			cr->fill_preserve();
			cr->stroke();
			cr->restore();
		}

		PortADiagram(CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			PortDiagram(a_cpu, "PORTA", x, y, width, height, a_area), cpu(a_cpu), margin(10), dh((height - margin*2) / 8) {

			for (int n=0; n < 8; ++n) {
				Connection &conn = a_cpu.pins[a_cpu.porta.pin_numbers[n]];
				pd.push(new PinDiagram(conn, x + width + 15, y + margin - 10 + (n+1) * dh, 0, 0.5, a_area));
				add(BlockDiagram::text(width+35, margin - 5 + (n+1) * dh, conn.name()));
				DeviceEvent<Connection>::subscribe<PortADiagram>(this, &PortADiagram::on_pin_change, &conn);
				pins.push_back(new ConnectionDiagram(conn, x+width, y + margin - 10 + (n+1) * dh, m_area));
				pins[n]->add(ConnectionDiagram::pt( 0,  0, true));
				pins[n]->add(ConnectionDiagram::pt(20,  0, false));
			}
		}

		virtual ~PortADiagram(){
			for (int n=0; n < 8; ++n) {
				Connection &conn = pins[n]->connection();
				DeviceEvent<Connection>::unsubscribe<PortADiagram>(this, &PortADiagram::on_pin_change, &conn);
			}
		}

	};

	class PortBDiagram: public PortDiagram {
		const CPU_DATA &cpu;
		int margin;
		double dh;

		std::vector<ConnectionDiagram *>pins;
		std::stack<PinDiagram *> pd;

		virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
			if (name == "TRISB") tris(data[Register::DVALUE::NEW]);
			if (name == "PORTB") port(data[Register::DVALUE::NEW]);
		}

		void on_pin_change(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
			redraw();
		}

	  public:

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			PortDiagram::do_draw(cr);
			BYTE trisB = tris();
			BYTE portB = port();
			const PINS &pins = cpu.pins;

			cr->save();
			for (int n = 0; n < 8; ++n) {
				bool trisflag = trisB & 1;
				bool portflag = portB & 1;

				cr->move_to(9, margin - 5 + (n+1) * dh);
				cr->text_path(portflag ? "1" : "0");

				cr->move_to(22, margin - 5 + (n+1) * dh);
				cr->text_path(trisflag ? "i" : "o");

				cr->move_to(35, margin - 5 + (n+1) * dh);
				int pin_no = cpu.portb.pin_numbers[n];
				cr->text_path(pins[pin_no].signal() ? "1" : "0");

				trisB = trisB >> 1;
				portB = portB >> 1;
			}
			cr->set_line_width(0.5);
			cr->fill_preserve();
			cr->stroke();
			cr->restore();
		}

		PortBDiagram(CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			PortDiagram(a_cpu, "PORTB", x, y, width, height, a_area), cpu(a_cpu), margin(10), dh((height - margin*2) / 8) {

			for (int n=0; n < 8; ++n) {
				Connection &conn = a_cpu.pins[a_cpu.portb.pin_numbers[n]];
				pd.push(new PinDiagram(conn, x + width + 15, y + margin - 10 + (n+1) * dh, 0, 0.5, a_area));
				DeviceEvent<Connection>::subscribe<PortBDiagram>(this, &PortBDiagram::on_pin_change, &conn);
				add(BlockDiagram::text(width+35, margin - 5 + (n+1) * dh, conn.name()));
				pins.push_back(new ConnectionDiagram(conn, x+width, y + margin - 10 + (n+1) * dh, m_area));
				pins[n]->add(ConnectionDiagram::pt( 0,  0).first());
				pins[n]->add(ConnectionDiagram::pt(20,  0));
			}
		}
		virtual ~PortBDiagram(){
			for (int n=0; n < 8; ++n) {
				Connection &conn = pins[n]->connection();
				DeviceEvent<Connection>::unsubscribe<PortBDiagram>(this, &PortBDiagram::on_pin_change, &conn);
			}
		}

	};

	class WRegDiagram: public BlockDiagram {
		const CPU_DATA &m_cpu;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->move_to(5, 30);
			cr->text_path("Value: ");
			cr->text_path(int_to_hex(m_cpu.W));
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		WRegDiagram(const CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "W reg", a_area), m_cpu(a_cpu) {
		}
	};

	class ClockDiagram: public BlockDiagram {
		const CPU_DATA &m_cpu;
		std::string Q;
		int osc;

		void draw_row(const Cairo::RefPtr<Cairo::Context>& cr, const std::string &t, int row) {
			const int BASE = 14;
			const int LINE_HEIGHT = 14;
			const int LEFT = 60;

			cr->save();
			Cairo::TextExtents extents;
			cr->get_text_extents(t, extents);
			cr->move_to(LEFT-2-extents.width, BASE+row*LINE_HEIGHT);
			cr->text_path(t);
			cr->move_to(LEFT, BASE+row*LINE_HEIGHT);
			cr->text_path("|");
			cr->set_line_width(0.5);
			cr->fill_preserve();
			cr->stroke();
			cr->restore();
		}


		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			const int BASE = 14;
			const int LINE_HEIGHT = 14;
			const int STEP = 12;
			const int LEFT = 60;

			cr->move_to(LEFT, BASE);
			cr->text_path("| Q1 | Q2 | Q3 | Q4 ");
			cr->set_line_width(0.5);
			cr->fill_preserve();
			cr->stroke();
			if (Q.length()) {
				cr->set_line_width(0.8);

				draw_row(cr, "OSC1", 1);
				cr->save();
				cr->translate(LEFT, LINE_HEIGHT * 0 + BASE + 6);
				cr->move_to(0, 0);
				for (int n = 0; n < osc+1; ++n) {
					int q = n + 1;
					cr->line_to(q*STEP-2,     (n % 2) * (LINE_HEIGHT-6));
					cr->line_to(q*STEP,       (q % 2) * (LINE_HEIGHT-6));
				}
				cr->stroke();
				cr->restore();

				draw_row(cr, Q, 2);
				cr->save();
				cr->translate(LEFT, LINE_HEIGHT * 1 + BASE + 6);
				cr->move_to(0, Q == "Q1" ? 0 : LINE_HEIGHT - 6);
				for (int n = 0; n < osc+1; ++n) {
					int q = n + 1;
					std::string t  = std::string("Q") + int_to_string(n / 2 + 1);
					std::string tq = std::string("Q") + int_to_string(q / 2 + 1);
					cr->line_to(q*STEP-2, (Q == t ? 0 : 1) * (LINE_HEIGHT-6));
					cr->line_to(q*STEP,   (Q == tq ? 0 : 1) * (LINE_HEIGHT-6));
				}
				cr->stroke();
				cr->restore();

				draw_row(cr, "CLKOUT", 3);
				cr->save();
				cr->translate(LEFT, LINE_HEIGHT * 2 + BASE + 6);
				cr->move_to(2, 0);
				cr->line_to(4, LINE_HEIGHT-6);
				for (int n = 0; n < osc+1; ++n) {
					int q = n + 1;
					cr->line_to(q*STEP-2, ((n / 4 + 1) % 2) * (LINE_HEIGHT-6));
					cr->line_to(q*STEP,   ((q / 4 + 1) % 2) * (LINE_HEIGHT-6));
				}
				cr->stroke();
				cr->restore();
			}
			redraw();
		}


	  public:
		void process(const std::string &name) {
			if (name == "Q1" || name == "Q2" || name == "Q3" || name == "Q4")
				Q = name;
			else if (name == "oscillator")
				osc++;
			else if (name == "cycle")
				osc = 0;
		}

		ClockDiagram(const CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "CLK", a_area), m_cpu(a_cpu), Q(""), osc(0) {
		}
	};

	class STATUSDiagram: public BlockDiagram {
		const CPU_DATA &m_cpu;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			const int BASE=30;
			const int LINE_HEIGHT=12;
			const std::string HEADERS("irp rp1 rp0  t0  pd   z   dc   c");

			Cairo::TextExtents extents;
			cr->get_text_extents(HEADERS.c_str(), extents);

			BYTE status = m_cpu.sram.status();

			cr->move_to(5, BASE);
			cr->text_path(HEADERS);

			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->move_to(5, BASE);
			cr->line_to(5 + extents.width, BASE);
			cr->stroke();

			cr->move_to(5, BASE+LINE_HEIGHT);
			cr->text_path(" ");      cr->text_path(int_to_string(status >> 7 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 6 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 5 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 4 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 5 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 4 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status >> 1 & 1));
			cr->text_path("    ");    cr->text_path(int_to_string(status & 1));
			cr->set_line_width(0.5);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		STATUSDiagram(const CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "STATUS", a_area), m_cpu(a_cpu) {
		}
	};


	class RAMDiagram: public BlockDiagram {
		CPU_DATA &m_cpu;
		const BYTE &m_idx;
		const bool &m_file;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			const int BASE = 30;
			const int LINE_HEIGHT = 14;

			if (!m_file) {
				cr->move_to(5, BASE+LINE_HEIGHT*3);
				cr->line_to(90, BASE);
				cr->set_line_width(1.0);
				cr->stroke();
			} else if (m_idx) {
				cr->move_to(5, BASE);
				cr->text_path("INDEX: ");
				cr->text_path(int_to_hex(m_idx));
				cr->move_to(5, BASE+LINE_HEIGHT);
				cr->text_path("BANK: ");
				cr->text_path(int_to_string(m_cpu.sram.bank()));
				cr->move_to(5, BASE+LINE_HEIGHT*2);
				cr->text_path("NAME: ");
				cr->text_path(m_cpu.register_name(m_idx));
				cr->move_to(5, BASE+LINE_HEIGHT*3);
				cr->text_path("CONTENT: ");
				cr->text_path(int_to_hex(m_cpu.sram.read(m_idx, false)));
			} else {
				cr->move_to(5, BASE);
				cr->text_path("INDIRECT");
				WORD fsr = m_cpu.sram.fsr();
				cr->move_to(5, BASE+LINE_HEIGHT);
				cr->text_path("FSR: ");
				cr->text_path(int_to_hex(fsr));
				cr->move_to(5, BASE+LINE_HEIGHT*2);
				cr->text_path("NAME: ");
				cr->text_path(m_cpu.register_name(fsr));
				cr->move_to(5, BASE+LINE_HEIGHT*3);
				cr->text_path("CONTENT: ");
				cr->text_path(int_to_hex(m_cpu.sram.read(m_idx, true)));
			}
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		RAMDiagram(CPU_DATA &a_cpu, const BYTE &a_idx, const bool &a_file, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "File Registers", a_area), m_cpu(a_cpu), m_idx(a_idx), m_file(a_file) {
		}
	};


	class StackDiagram: public BlockDiagram {
		CPU_DATA &m_cpu;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			const int LINE_HEIGHT = 12;
			const int STACK_SIZE = 8;
			for (int n=0; n < STACK_SIZE; ++n) {
				cr->move_to(5, 30 + n * LINE_HEIGHT);
				cr->text_path(int_to_string(8-n));
				cr->text_path(": ");
				if (n >= m_cpu.SP)
					cr->text_path(int_to_hex(m_cpu.stack[n]));
				else
					cr->text_path("------------");
			}
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		StackDiagram(CPU_DATA &a_cpu, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "8-Level Stack", a_area), m_cpu(a_cpu) {
		}
	};

	class InstructionDiagram: public BlockDiagram {
		CPU_DATA &m_cpu;
		const std::string &assembly;

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->move_to(5, 30);
			size_t start=0, end=assembly.find("\t");
			cr->text_path(assembly.substr(start, end-start));
			cr->text_path(" -- ");
			start = end = end + 1; end = assembly.find("\t", end);
			cr->text_path(assembly.substr(start, end-start));
			cr->set_line_width(0.7);
			cr->fill_preserve();
			cr->stroke();
		}

	  public:
		InstructionDiagram(CPU_DATA &a_cpu, const std::string &a_assembly, double x, double y, double width, double height, Glib::RefPtr<Gtk::DrawingArea>a_area):
			BlockDiagram(x, y, width, height, "Instruction Reg", a_area), m_cpu(a_cpu), assembly(a_assembly) {
		}
	};


	class CPUDrawing: public CairoDrawing {
		CPU_DATA &m_cpu;
		std::string m_assembly;
		WORD m_execPC;
		BYTE m_idx;
		bool m_file;

		Glib::RefPtr<Gtk::Builder> m_refGlade;

	  protected:
		std::map<std::string, SmartPtr<Component> > m_components;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
			cr->move_to(400, 40);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("PIC16f628");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void on_status_change(const CpuEvent &e) {
			// CpuEvent(data.execPC, data.SP, data.W, disassembled);
			std::string no_file("CALL;GOTO;RETURN;SLEEP;RETFIE;CLRWDT;MOVLW;RETLW;ADDLW;SUBLW;XORLW;IORLW;ANDLW");

			m_assembly = e.disassembly;
			m_execPC = e.PC;
			m_idx = e.OPCODE & 0x7f;
			m_file = (no_file.find(m_assembly.substr(0,m_assembly.find("\t"))) == std::string::npos);
			m_area->queue_draw();
		}

		void clock(const std::string &name) {
			ClockDiagram &c = dynamic_cast<ClockDiagram &>(*m_components["Clock"]);
			c.process(name);
		}


		virtual ~CPUDrawing() {
//			CpuEvent::unsubscribe((void *)this, &CPUDrawing::status_monitor);
		}

		CPUDrawing(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("cpu_model"))),
			m_cpu(a_cpu), m_execPC(0), m_idx(0), m_file(0), m_refGlade(a_refGlade)
		{
			CpuEvent::subscribe((void *)this, &CPUDrawing::status_monitor);

			BlockDiagram *cpu = new BlockDiagram(10,10, 800, 600, "BLOCK DIAGRAM", m_area);
			m_components["CPU"] = cpu;

			// Program Counter
			cpu->add(new BusSymbol(Point(130, 55, true), Point(180, 55, false), 10.0, 13));
			m_components["PC"] = new PCDiagram(m_execPC, 190.0, 50.0, 100.0, 40.0, m_area);

			// Stack
			cpu->add(new BusSymbol(Point(220, 120, true), Point(220, 85, true), 10.0, 13));
			m_components["Stack"] = new StackDiagram(m_cpu, 190.0, 135.0, 100.0, 120.0, m_area);

			// Flash Memory
			m_components["Flash"] = new FlashDiagram(m_cpu, m_execPC, 50.0, 50.0, 80.0, 90.0, m_area);

			// Program Bus
			cpu->add(new BusSymbol(Point(75, 200, true), Point(75, 130, false), 10.0, 14));
			cpu->add(BlockDiagram::text(20,150, "Program\nBus"));

			m_components["Instruction"] = new InstructionDiagram(m_cpu, m_assembly, 45.0, 215.0, 120.0, 40.0, m_area);

			// Direct Addr
			cpu->add(new BusSymbol(Point(100, 285, false, true), Point(100, 245, false), 10.0));
			cpu->add(new BusSymbol(Point(108, 275, false), Point(330, 275, false, true), 10.0, 7));
			cpu->add(new BusSymbol(Point(320, 280, false), Point(320, 250, true), 10.0));
			cpu->add(BlockDiagram::text(120,275, "Direct Addr"));

			// Instruction Bus
			cpu->add(new BusSymbol(Point(75, 400, true), Point(75, 245, false), 10.0, 8));
			cpu->add(new BusSymbol(Point(83,  370, false), Point(410, 370, false, true), 10.0, 8));
			cpu->add(new BusSymbol(Point(400, 395, true), Point(400, 375, false), 10.0));
			cpu->add(BlockDiagram::text(140, 370,"Instruction Bus"));

			// Control logic
			m_components["LOGIC"] = new BlockDiagram(55,415,70,45, "Instruction\nDecode &\nControl", m_area);

			// Addr MUX
			cpu->add(new MuxSymbol(345, 245, -M_PI/2));
			cpu->add(new BusSymbol(Point(340, 235, false), Point(340, 215, true), 10.0, 9));

			// data bus
			cpu->add(new BusSymbol(Point(290, 55, true),   Point(470, 55, false, true), 10.0, 8));
			cpu->add(new BusSymbol(Point(470, 60, false),  Point(470, 530, false, true), 10.0));
			cpu->add(new BusSymbol(Point(465, 530, false), Point(55,  530, false, true), 10.0));
			cpu->add(BlockDiagram::text(400,55,"Data Bus"));

			// RAM data
			m_components["RAM"] = new RAMDiagram(m_cpu, m_idx, m_file, 315.0, 135.0, 100.0, 85.0, m_area);
			cpu->add(new BusSymbol(Point(350, 120, true),   Point(350, 60, false), 10.0));

			// FSR data & mux
			m_components["FSR"] = new FSRDiagram(m_cpu, 345.0, 280.0, 70.0, 35.0, m_area);
			cpu->add(new BusSymbol(Point(410, 280, true),   Point(465, 280, false), 10.0));
			cpu->add(new BusSymbol(Point(365, 270, false),  Point(365, 250, true), 10.0, 8));

			// STATUS data
			cpu->add(new BusSymbol(Point(410, 330, true),   Point(465, 330, false, false), 10.0));
			m_components["STATUS"] = new STATUSDiagram(m_cpu, 270.0, 320.0, 145.0, 45.0, m_area);

			// Literal-Data MUX
			cpu->add(new MuxSymbol(405, 400, M_PI/2));
			cpu->add(new BusSymbol(Point(420, 370, false, true),  Point(465, 370, false), 10.0));
			cpu->add(new BusSymbol(Point(420, 395, true), Point(420, 375, false), 10.0));

			// ALU
			cpu->add(new ALUSymbol(395, 440, 0));
			cpu->add(new BusSymbol(Point(415, 422, true),   Point(415, 410, false), 10.0));
			cpu->add(BlockDiagram::text(385, 450, "ALU"));
			cpu->add(new BusSymbol(Point(365, 430, false, true),  Point(365, 360, true), 10.0, 3));
			cpu->add(new BusSymbol(Point(400, 450, false, true),  Point(400, 475, true), 10.0));
			cpu->add(new BusSymbol(Point(395, 460, false), Point(465, 460, false), 10.0));

			// W Register
			m_components["Wreg"] = new WRegDiagram(m_cpu, 370.0, 490.0, 70.0, 35.0, m_area);
			cpu->add(new BusSymbol(Point(360, 500, false), Point(330, 500, false, true), 10.0));
			cpu->add(new BusSymbol(Point(330, 495, false), Point(330, 400, false, true), 10.0, 8));
			cpu->add(new BusSymbol(Point(335, 400, false), Point(360, 400, false, true), 10.0));
			cpu->add(new BusSymbol(Point(360, 405, false), Point(360, 420, true), 10.0));

			// Devices
			m_components["Comparator"] = new BlockDiagram(40,  494, 65, 16, "Comparator", m_area);
			m_components["Timer0"] =     new BlockDiagram(110, 494, 65, 16, "Timer0", m_area);
			m_components["Timer1"] =     new BlockDiagram(180, 494, 65, 16, "Timer1", m_area);
			m_components["Timer2"] =     new BlockDiagram(250, 494, 65, 16, "Timer2", m_area);

			m_components["VREF"]   =     new BlockDiagram(40,  560, 65, 16, "VREF", m_area);
			m_components["CCP1"]   =     new BlockDiagram(110, 560, 65, 16, "CCP1", m_area);
			m_components["USART"]  =     new BlockDiagram(180, 560, 65, 16, "USART", m_area);
			m_components["EEPROM"] =     new BlockDiagram(250, 560, 65, 16, "EEPROM", m_area);

			cpu->add(new BusSymbol(Point(65,  505, true), Point(65,  525, false), 10.0));
			cpu->add(new BusSymbol(Point(135, 505, true), Point(135, 525, false), 10.0));
			cpu->add(new BusSymbol(Point(205, 505, true), Point(205, 525, false), 10.0));
			cpu->add(new BusSymbol(Point(275, 505, true), Point(275, 525, false), 10.0));

			cpu->add(new BusSymbol(Point(65,  525, false), Point(65,  545, true), 10.0));
			cpu->add(new BusSymbol(Point(135, 525, false), Point(135, 545, true), 10.0));
			cpu->add(new BusSymbol(Point(205, 525, false), Point(205, 545, true), 10.0));
			cpu->add(new BusSymbol(Point(275, 535, true),  Point(275, 550, false),10.0));

			// Clock
			m_components["Clock"] = new ClockDiagram(m_cpu, 160.0, 410.0, 160.0, 60.0, m_area);
			cpu->add(new BusSymbol(Point(150, 440, false),  Point(120, 440, true),10.0));

			// PORT A
			m_components["PORTA"] = new PortADiagram(m_cpu, 510, 100, 50, 140, m_area);
			cpu->add(new BusSymbol(Point(465, 160, false),  Point(495, 160, true),10.0));

			// PORT B
			m_components["PORTB"] = new PortBDiagram(m_cpu, 510, 300, 50, 140, m_area);
			cpu->add(new BusSymbol(Point(465, 360, false),  Point(495, 360, true),10.0));


		}

		static void status_monitor(void *ob, const CpuEvent &e) {
			CPUDrawing *dwg = (CPUDrawing *)(ob);
			dwg->on_status_change(e);
		}


	};

	class CPUModel: public Component {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		SmartPtr<CPUDrawing> cpu_drawing;
	  public:

		void clock_event(Clock *device, const std::string &name, const std::vector<BYTE> &data){
			cpu_drawing->clock(name);
		}

		virtual ~CPUModel() {
			DeviceEvent<Clock>::unsubscribe<CPUModel>(this, &CPUModel::clock_event);
		}

		CPUModel(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			cpu_drawing = new CPUDrawing(a_cpu, a_refGlade);
			DeviceEvent<Clock>::subscribe<CPUModel>(this, &CPUModel::clock_event);
		}
	};


}
