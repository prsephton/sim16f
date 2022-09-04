#pragma once

#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include <queue>
#include "application.h"
#include "paint/cairo_drawing.h"
#include "paint/common.h"
#include "paint/diagrams.h"
#include "../utils/smart_ptr.h"

namespace app {

	struct Timer0Data {
		std::string event_name;
		BYTE event_data;

		Timer0Data(const std::string &a_event_name, BYTE a_event_data) {
			event_name = a_event_name;
			event_data = a_event_data;
		}
	};

	class Timer0Diagram: public CairoDrawing  {

		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		std::map<std::string, SmartPtr<Component> > m_components;

		std::queue<Timer0Data> m_queue;
		Connection m_Fosc;      //  (Clock oscillator cycle signal)
		Connection m_T0CS;      //  (Clock select - Options register)
		Connection m_PSA;       //  (Prescaler - Options register)
		Connection m_T0CKI;     // (PIN RA4)
		Connection m_T0SE;      //  (Signal from Options Register)
		Connection m_WDT;       //  (Watchdog Timer)
		Connection m_WDT_en;    //  (Watchdog Timer Enable)
		Connection m_T0IF;      //  Timer 0 Interrupt Flag
		XOrGate m_T0SE_Gate;    // RA4 xor T0SE
		Mux m_T0CS_Mux;         // choose between Fosc and TOSE_Gate
		Mux m_PSA_Mux1;         // WDT or TMR1 Clock source
		Counter m_Prescaler;    //  (Represents the prescaler counter)
		Counter m_PS;           //  PS<2:0> from Options register
		Mux m_Prescale_Mux;     // Prescaler Mux
		Mux m_PSA_Mux2;         // WDT or Prescaler output
		Mux m_PSA_Mux3;         // Clock Source or Prescaler output
		Counter m_Sync;         //  PS<2:0> from Options register
		Counter m_tmr0;         // Our timer register

		SignalTrace m_trace;   // trace some signals

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
			show_coords(cr);
			cr->move_to(400, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Diagram of Timer0/WDT");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}


		void draw_pin() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_T0CKI, 40, 150, m_area);
			m_components["PIN"] = dia;
			dia->add(new PinSymbol(60, 0, DIRECTION::LEFT));
			dia->add(ConnectionDiagram::pt(60, 0).first());
			dia->add(ConnectionDiagram::pt(85, 0));
			dia->add(ConnectionDiagram::text(0, 0, "T0CKI\npin"));
		}


		void draw_T0SE() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_T0SE, 100, 180, m_area);
			m_components["T0SE"] = dia;
			dia->add(ConnectionDiagram::pt(10,  0).first());
			dia->add(ConnectionDiagram::pt(10, -20));
			dia->add(ConnectionDiagram::pt(25, -20));
			dia->add(ConnectionDiagram::text(0, 10, "T0SE"));
		}

		void draw_T0SE_Gate() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_T0SE_Gate.rd(), 125, 155, m_area);
			m_components["T0SE Gate"] = dia;
			dia->add(new OrSymbol(2, 0, 0, 0, false, true));
			dia->add(ConnectionDiagram::pt(45,0).first());
			dia->add(ConnectionDiagram::pt(65,0));
		}

		void draw_Fosc() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_Fosc, 80, 100, m_area);
			m_components["Fosc"] = dia;
			dia->add(ConnectionDiagram::text(0,0,"Fosc/4"));
			dia->add(ConnectionDiagram::pt(50, 0).first());
			dia->add(ConnectionDiagram::pt(320, 0));
			dia->add(ConnectionDiagram::pt(320, 30));
			dia->add(ConnectionDiagram::pt(80, 0).first().join());
			dia->add(ConnectionDiagram::pt(80, 30));
			dia->add(ConnectionDiagram::pt(110, 30));
		}

		void draw_T0CS_Mux() {
			MuxDiagram *mux = new MuxDiagram(m_T0CS_Mux, 190, 140, 0, m_area);
			m_components["T0CS_Mux"] = mux;
			mux->draw_forward(false);

			ConnectionDiagram * dia = new ConnectionDiagram(m_T0CS_Mux.rd(), 200, 140, m_area);
			m_components["T0CS_Mux.out"] = dia;
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(80,0));
			dia->add(ConnectionDiagram::pt(30,0).first().join());
			dia->add(ConnectionDiagram::pt(30,80));
			dia->add(ConnectionDiagram::pt(-80,80));
			dia->add(ConnectionDiagram::pt(-80,120));
			dia->add(ConnectionDiagram::pt(-30,120));

			ConnectionDiagram * t0cs = new ConnectionDiagram(m_T0CS, 195, 165, m_area);
			m_components["T0CS_Mux.gate"] = t0cs;

			t0cs->add(ConnectionDiagram::pt(0,0).first());
			t0cs->add(ConnectionDiagram::pt(0,20));
			t0cs->add(ConnectionDiagram::text(-15,28, "T0CS"));
		}

		void draw_PSA_Mux3() {
			MuxDiagram *mux = new MuxDiagram(m_PSA_Mux3, 280, 150, 0, m_area);
			m_components["PSA_Mux3"] = mux;
			ConnectionDiagram * dia = new ConnectionDiagram(m_PSA_Mux3.rd(), 290, 150, m_area);
			m_components["PSA_Mux3.out"] = dia;
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(80,0));

			ConnectionDiagram *psa = new ConnectionDiagram(m_PSA, 285, 175, m_area);
			m_components["PSA_Mux3.gate"] = psa;
			psa->add(ConnectionDiagram::pt(0,0).first());
			psa->add(ConnectionDiagram::pt(0,20));
			psa->add(ConnectionDiagram::text(-10,28, "PSA"));

		}

		void draw_PSA_Mux1() {
			MuxDiagram *mux = new MuxDiagram(m_PSA_Mux1, 170, 280, 0, m_area);
			m_components["PSA_Mux1"] = mux;
			mux->draw_forward(false);

			ConnectionDiagram * dia = new ConnectionDiagram(m_PSA_Mux1.rd(), 180, 280, m_area);
			m_components["PSA_Mux1.out"] = dia;
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(60,0));

			ConnectionDiagram *psa = new ConnectionDiagram(m_PSA, 175, 305, m_area);
			m_components["PSA_Mux1.gate"] = psa;
			psa->add(ConnectionDiagram::pt(0,0).first());
			psa->add(ConnectionDiagram::pt(0,20));
			psa->add(ConnectionDiagram::text(-10,28, "PSA"));
		}

		void draw_Prescaler() {
			CounterDiagram *counter = new CounterDiagram(m_Prescaler, m_area, 240, 270);
			m_components["Prescaler"] = counter;

			ConnectionDiagram * dia = new ConnectionDiagram(m_PSA_Mux1.rd(), 295, 300, m_area);
			m_components["Prescaler.out"] = dia;

			dia->add(new BusSymbol(Point(0, -1, false, false), Point(0, 40, true), 8.0, 8));
			dia->add(ConnectionDiagram::text(-50, -35, "Prescaler/Postscaler"));

		}

		struct prescale_text: public GenericDiagram::text {
			Counter &m_PS;
			std::string x;

			virtual const std::string fetch_text() {
				x = int_to_string(m_PS.get());
				return x;
			}
			prescale_text(Counter &ps, int x, int y) : GenericDiagram::text(x, y, "stuff"), m_PS(ps) {};
		};

		void draw_Prescale_Mux() {
			MuxDiagram *mux = new MuxDiagram(m_Prescale_Mux, 290, 345, DIRECTION::DOWN, m_area);
			m_components["Prescale_Mux"] = mux;
			mux->draw_forward(false);

			ConnectionDiagram * dia = new ConnectionDiagram(m_Prescale_Mux.rd(), 290, 365, m_area);
			m_components["Prescale_Mux.out"] = dia;
			dia->add(ConnectionDiagram::pt(  0,   0).first());
			dia->add(ConnectionDiagram::pt(  0,  40));
			dia->add(ConnectionDiagram::pt( 20,  40));
			dia->add(ConnectionDiagram::pt(  0,  20).first().join());
			dia->add(ConnectionDiagram::pt(200,  20));
			dia->add(ConnectionDiagram::pt(200,-140));
			dia->add(ConnectionDiagram::pt(-30,-140));
			dia->add(ConnectionDiagram::pt(-30,-200));
			dia->add(ConnectionDiagram::pt(-10,-200));

			GenericDiagram *ps = new GenericDiagram(390, 345, m_area);
			m_components["Prescale_Mux.gate"] = ps;
			ps->add(GenericDiagram::pt(0,10).first());
			ps->add(GenericDiagram::pt(-25,10));
			std::cout << "Get prescale rate: " << m_PS.get() << std::endl;
			ps->add(GenericDiagram::text(5, 10, "PS<2:0>"));
			ps->add(new prescale_text(m_PS, -15, 20));
//			ps->add(GenericDiagram::text(-15, 20, int_to_string(m_PS.get())));
		}


		void draw_WDT() {
			BlockDiagram *wdt = new BlockDiagram(50, 280, 60, 50, "Watchdog\nTimer", m_area);
			m_components["WDT"] = wdt;

			ConnectionDiagram * dia = new ConnectionDiagram(m_WDT, 110, 290, m_area);
			m_components["WDT.out"] = dia;   // TODO: Watchdog timer is unimplemented for now
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(60,0));
			dia->add(ConnectionDiagram::pt(30,0).first().join());
			dia->add(ConnectionDiagram::pt(30, 140));
			dia->add(ConnectionDiagram::pt(200,140));


			ConnectionDiagram * wdt_en = new ConnectionDiagram(m_WDT_en, 80, 330, m_area);
			m_components["WDT.en"] = wdt_en;
			wdt_en->add(ConnectionDiagram::pt(0,0).first());
			wdt_en->add(ConnectionDiagram::pt(0,20));
			wdt_en->add(ConnectionDiagram::text(-40,28, "WDT Enable Bit"));

		}

		void draw_PSA_Mux2() {
			MuxDiagram *mux = new MuxDiagram(m_PSA_Mux2, 310, 420, 0, m_area);

			m_components["PSA_Mux2"] = mux;
			ConnectionDiagram * dia = new ConnectionDiagram(m_PSA_Mux2.rd(), 320, 420, m_area);
			m_components["PSA_Mux2.out"] = dia;
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(80,0));
			dia->add(new VssSymbol(80, 0, DIRECTION::UP));
			dia->add(ConnectionDiagram::text(100, 2, "WDT\nTime-out"));

			ConnectionDiagram *psa = new ConnectionDiagram(m_PSA, 315, 445, m_area);
			m_components["PSA_Mux2.gate"] = psa;
			psa->add(ConnectionDiagram::pt(0,0).first());
			psa->add(ConnectionDiagram::pt(0,20));
			psa->add(ConnectionDiagram::text(-10,28, "PSA"));

		}


		void draw_TimerSync() {
			CounterDiagram *counter = new CounterDiagram(m_Sync, m_area, 370, 130);
			m_components["Sync"] = counter;

			ConnectionDiagram * dia = new ConnectionDiagram(m_Sync.bit(0), 427, 150, m_area);
			m_components["Sync.out"] = dia;
			dia->add(ConnectionDiagram::pt(0,0).first());
			dia->add(ConnectionDiagram::pt(35,0));

		}


		void draw_trace() {
			TraceDiagram *trace = new TraceDiagram(m_trace, m_area, 500, 230);
			m_components["trace"] = trace;
		}

		void draw_TMR0() {
			CounterDiagram *counter = new CounterDiagram(m_tmr0, m_area, 460, 130);
			m_components["TMR0"] = counter;

			GenericDiagram * dia = new GenericDiagram(460, 130, m_area);
			m_components["TMR0.out"] = dia;
			dia->add(ConnectionDiagram::text(0, 43, "TMR0 Register"));
			dia->add(new BusSymbol(Point(40, -5, true), Point(40, -40, true), 8.0, 8));
			dia->add(ConnectionDiagram::text(20, -48, "Data Bus"));

			ConnectionDiagram * t0if = new ConnectionDiagram(m_T0IF, 566, 150, m_area);
			m_components["t0if"] = t0if;
			t0if->add(ConnectionDiagram::pt(0,0).first());
			t0if->add(ConnectionDiagram::pt(140,0));
			t0if->add(new VssSymbol(140, 0, DIRECTION::UP));
			t0if->add(ConnectionDiagram::text(20, -4, "TMR0 Interrupt Flag"));
		}


	  protected:

		void timer0_changed(Timer0 *t, const std::string &name, const std::vector<BYTE> &data) {
			if (name == "Reset") {
				m_Prescaler.set_value(0);
				m_tmr0.set_value(data[0]);
			} else if (name == "INTCON") {
				bool int_flag = (data[0] & Flags::INTCON::T0IF) != 0;
				m_T0IF.set_value(int_flag * m_T0IF.Vdd, false);
			} else if (name == "Overflow") {
				m_T0IF.set_value(m_T0IF.Vdd, false);
			}
		}

		void clock_changed(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
			if (name == "Q1" || name == "Q3") {
				m_Fosc.set_value(m_Fosc.Vdd, false);
				m_queue.push(Timer0Data(name, data.size()?data[0]:0));
			} else if (name == "Q2" || name == "Q4") {
				m_Fosc.set_value(m_Fosc.Vss, false);
				m_queue.push(Timer0Data(name, data.size()?data[0]:0));
			} else if (name == "cycle") {
//				Timer0 &t = m_cpu.tmr0;
//				std::cout << "T0 value=" << std::hex << (int)t.timer() << "; TMR0 value=" << std::hex << (int)m_tmr0.get() << std::endl;
			}
		}

	  public:
		void process_queue(){
			if (m_queue.size()) {    // - updates in app thread only
//				Timer0Data &t0data = m_queue.front();
				Timer0 &t = m_cpu.tmr0;
				m_T0CKI.set_value(t.ra4_signal() * m_T0CKI.Vdd, false);
				m_T0SE.set_value(t.falling_edge() * m_T0SE.Vdd, false);
				m_T0CS.set_value(t.use_RA4() * m_T0CS.Vdd, false);
				m_WDT_en.set_value(t.WDT_en() * m_WDT_en.Vdd, false);
				m_PSA.set_value(t.assigned_to_wdt() * m_PSA.Vdd, false);
				m_PS.set_value(t.prescale_rate());
				m_queue.pop();
				m_area->queue_draw();
			} else {
				sleep_for_us(100);
			}
		}


		Timer0Diagram(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_TMR0"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade),
			m_Fosc(0, false, "Fosc"),
			m_T0IF(0, false, "T0IF"),
			m_T0SE_Gate({&m_T0CKI, &m_T0SE}),
			m_T0CS_Mux({&m_Fosc, &m_T0SE_Gate.rd()}, {&m_T0CS}),
			m_PSA_Mux1({&m_T0CS_Mux.rd(), &m_WDT}, {&m_PSA}, "Mux1"),
			m_Prescaler(m_PSA_Mux1.rd(), true, 8, 0),
			m_PS(3, 0),
			m_Prescale_Mux(m_Prescaler.databits(), m_PS.databits()),
			m_PSA_Mux2({&m_WDT, &m_Prescale_Mux.rd()}, {&m_PSA}, "Mux2"),
			m_PSA_Mux3({&m_Prescale_Mux.rd(), &m_T0CS_Mux.rd()}, {&m_PSA}, "Mux3"),
			m_Sync(m_PSA_Mux3.rd(), true, 1, 0, &m_Fosc),
			m_tmr0(m_Sync.bit(0), true, 8),
			m_trace({&m_PSA_Mux3.rd(), &m_Fosc, &m_Sync.bit(0)})
		{
			DeviceEvent<Timer0>::subscribe<Timer0Diagram>(this, &Timer0Diagram::timer0_changed);
			DeviceEvent<Clock>::subscribe<Timer0Diagram>(this, &Timer0Diagram::clock_changed);

			draw_pin();
			draw_T0SE();
			draw_T0SE_Gate();
			draw_Fosc();
			draw_T0CS_Mux();
			draw_PSA_Mux3();
			draw_PSA_Mux1();
			draw_Prescaler();
			draw_Prescale_Mux();
			draw_WDT();
			draw_PSA_Mux2();
			draw_TimerSync();
			m_Sync.name("sync");
			m_trace.duration_us(10000000);  // 10s
			draw_trace();
			draw_TMR0();
		}

		~Timer0Diagram() {
			DeviceEvent<Timer0>::unsubscribe<Timer0Diagram>(this, &Timer0Diagram::timer0_changed);
			DeviceEvent<Clock>::unsubscribe<Timer0Diagram>(this, &Timer0Diagram::clock_changed);
		}

	};


	class Timer0: public Component  {
		Timer0Diagram m_diagram;
		bool m_exiting;

		bool process_queue(){
			m_diagram.process_queue();
			return !m_exiting;
		}

		virtual void exiting(){
			m_exiting = true;
		}

	public:
		Timer0(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade): Component(),
		  m_diagram(a_cpu, a_refGlade), m_exiting(false) {
			Glib::signal_idle().connect( sigc::mem_fun(*this, &Timer0::process_queue) );
		};

	};

}				// namespace app
