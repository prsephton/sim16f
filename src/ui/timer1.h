#pragma once

#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include <queue>
#include "application.h"
#include "paint/cairo_drawing.h"
#include "paint/common.h"
#include "paint/diagrams.h"
#include "../cpu_data.h"
#include "../utils/smart_ptr.h"

namespace app {

	class Timer1Diagram: public CairoDrawing  {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		SignalTrace m_trace;

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
//			show_coords(cr);
			cr->move_to(260, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Diagram of Timer1");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void draw_rb6() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_cpu.tmr1.rb6(), 40, 305, m_area);
			m_components["RB6"] = dia;
			dia->add(new PinSymbol(90, 0, DIRECTION::LEFT));
			dia->add(ConnectionDiagram::pt(90, 0).first());
			dia->add(ConnectionDiagram::pt(250, 0));
			dia->add(ConnectionDiagram::pt(155, 0).first().join());
			dia->add(ConnectionDiagram::pt(155, 15));
			dia->add(ConnectionDiagram::pt(120, 0).first().join());
			dia->add(ConnectionDiagram::pt(120, 10));
			dia->add(ConnectionDiagram::text(0, 0, "RB6/T1OSCO"));
		}

		void draw_rb7() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_cpu.tmr1.rb7(), 40, 370, m_area);
			m_components["RB7"] = dia;
			dia->add(new PinSymbol(90, 0, DIRECTION::LEFT));
			dia->add(ConnectionDiagram::pt(90, 0).first());
			dia->add(ConnectionDiagram::pt(155, 0));
			dia->add(ConnectionDiagram::pt(155, -15));
			dia->add(ConnectionDiagram::pt(120, 0).first().join());
			dia->add(ConnectionDiagram::pt(120, -10));
			dia->add(new ResistorSymbol(120, -10, DIRECTION::UP));
			dia->add(ConnectionDiagram::text(0, 0, "RB7/T1OSCI"));
		}

		void draw_t1osc() {
			TristateDiagram *ts = new TristateDiagram(m_cpu.tmr1.t1osc(), false, 195, 355, m_area);
			m_components["t1osc"] = ts;
			ts->set_rotation(DIRECTION::UP);
		}

		void draw_t1oscen() {
			ConnectionDiagram * dia = new ConnectionDiagram(m_cpu.tmr1.t1oscen(), 225, 340, m_area);
			m_components["t1oscen"] = dia;
			dia->add(ConnectionDiagram::text(0, 0, "T1OSCEN\nEnable\nOscillator"));
			dia->add(ConnectionDiagram::pt(0, 0).first());
			dia->add(ConnectionDiagram::pt(-20, 0));
		}

		void draw_trigger() {
			SchmittDiagram *trigger = new SchmittDiagram(m_cpu.tmr1.trigger(), 290, 305, DIRECTION::RIGHT, false, m_area);
			m_components["trigger"] = trigger;
			ConnectionDiagram * dia = new ConnectionDiagram(m_cpu.tmr1.trigger().rd(), 320, 305, m_area);
			m_components["trigger.out"] = dia;
			dia->add(ConnectionDiagram::pt(0, 0).first());
			dia->add(ConnectionDiagram::pt(40, 0));
		}

		void draw_t1csmux() {
			MuxDiagram *t1csmux = new MuxDiagram(m_cpu.tmr1.t1csmux(), 360, 322, DIRECTION::RIGHT, m_area);
			m_components["t1csmux"] = t1csmux;
			t1csmux->set_scale(1.5);
			t1csmux->flipped(true);
			ConnectionDiagram *fosc = new ConnectionDiagram(m_cpu.tmr1.fosc(), 360, 337, m_area);
			m_components["fosc"] = fosc;
			fosc->add(ConnectionDiagram::pt(0, 0).first());
			fosc->add(ConnectionDiagram::pt(-40, 0));
			fosc->add(ConnectionDiagram::text(-70, 0, "Fosc/4\nInternal\nClock"));
			ConnectionDiagram *tmr1cs = new ConnectionDiagram(m_cpu.tmr1.tmr1cs(), 368, 373, m_area);
			m_components["tmr1cs"] = tmr1cs;
			tmr1cs->add(ConnectionDiagram::pt(0, 0).first());
			tmr1cs->add(ConnectionDiagram::pt(0, 10));
			tmr1cs->add(ConnectionDiagram::text(-20, 20, "TMR1CS"));
			ConnectionDiagram *t1cs_out = new ConnectionDiagram(m_cpu.tmr1.t1csmux().rd(), 375, 322, m_area);
			m_components["t1cs.out"] = t1cs_out;
			t1cs_out->add(ConnectionDiagram::pt(0, 0).first());
			t1cs_out->add(ConnectionDiagram::pt(20, 0));
		}


		void draw_prescaler() {
			CounterDiagram *counter = new CounterDiagram(m_cpu.tmr1.prescaler(), m_area, 395, 310);
			m_components["Prescaler"] = counter;
			GenericDiagram *dia = new GenericDiagram(470, 320, m_area);
			m_components["Prescaler.io"] = dia;
			dia->add(new BusSymbol(Point(0, 0, false, false), Point(20, 0, true), 8.0, 4));
			dia->add(ConnectionDiagram::text(-70, -15, "Prescaler"));

			MuxDiagram *pscale = new MuxDiagram(m_cpu.tmr1.pscale(), 495, 325, DIRECTION::RIGHT, m_area);
			m_components["pscale"] = pscale;
			pscale->flipped(true);
			dia->add(ConnectionDiagram::text(-10, 70, "T1CKPS<1:0>"));

			ConnectionDiagram *pscale_out = new ConnectionDiagram(m_cpu.tmr1.pscale().rd(), 510, 325, m_area);
			m_components["pscale.out"] = pscale_out;
			pscale_out->add(ConnectionDiagram::pt(0, 0).first());
			pscale_out->add(ConnectionDiagram::pt(20, 0));
			pscale_out->add(ConnectionDiagram::pt(10, 0).first().join());
			pscale_out->add(ConnectionDiagram::pt(10, -208));
			pscale_out->add(ConnectionDiagram::pt(-20, -208));
		}

		void draw_synch() {
			CounterDiagram *synch = new CounterDiagram(m_cpu.tmr1.synch(), m_area, 530, 290);
			m_components["synch"] = synch;
			ConnectionDiagram *synch_out = new ConnectionDiagram(m_cpu.tmr1.synch().bit(0), 585, 325, m_area);
			m_components["synch_out"] = synch_out;
			synch_out->add(ConnectionDiagram::pt(0, 0).first());
			synch_out->add(ConnectionDiagram::pt(20, 0));
			synch_out->add(ConnectionDiagram::pt(20, -240));
			synch_out->add(ConnectionDiagram::pt(-90, -240));
			synch_out->add(ConnectionDiagram::text(-50, -40, "Synch"));
			synch_out->add(ConnectionDiagram::text(-50, -194, "Synchronised\n\nClock Input"));
		}

		void draw_t1syncmux() {
			MuxDiagram *syn_asyn = new MuxDiagram(m_cpu.tmr1.syn_asyn(), 495, 100, DIRECTION::LEFT, m_area);
			m_components["syn_asyn"] = syn_asyn;
			syn_asyn->set_scale(1.5);
			ConnectionDiagram *t1sync = new ConnectionDiagram(m_cpu.tmr1.t1sync(), 488, 150, m_area);
			m_components["t1sync"] = t1sync;
			t1sync->add(ConnectionDiagram::pt(0, 0).first());
			t1sync->add(ConnectionDiagram::pt(0, 16));
			t1sync->add(ConnectionDiagram::text(-20, 30, "T1SYNC").overscore());
			ConnectionDiagram *t1sync_out = new ConnectionDiagram(m_cpu.tmr1.syn_asyn().rd(), 480, 100, m_area);
			m_components["t1sync.out"] = t1sync_out;
			t1sync_out->add(ConnectionDiagram::pt(0, 0).first());
			t1sync_out->add(ConnectionDiagram::pt(-80, 0));
		}

		void draw_tmr1on() {
			AndDiagram *tmr1_en = new AndDiagram(m_cpu.tmr1.signal(), 400, 105, DIRECTION::LEFT, m_area);
			m_components["tmr1_en"] = tmr1_en;
			ConnectionDiagram *sig_out = new ConnectionDiagram(m_cpu.tmr1.signal().rd(), 370, 105, m_area);
			m_components["sig.out"] = sig_out;
			sig_out->add(ConnectionDiagram::pt(0, 0).first());
			sig_out->add(ConnectionDiagram::pt(-50, 0));
			ConnectionDiagram *tmr1on = new ConnectionDiagram(m_cpu.tmr1.tmr1on(), 420, 130, m_area);
			m_components["tmr1on"] = tmr1on;
			tmr1on->add(ConnectionDiagram::pt(0, 0).first());
			tmr1on->add(ConnectionDiagram::pt(0, -20));
			tmr1on->add(ConnectionDiagram::pt(-20, -20));
			tmr1on->add(ConnectionDiagram::text(-20, 12, "TMR1ON"));
		}

		void draw_tmr1() {
			CounterDiagram *tmr1 = new CounterDiagram(m_cpu.tmr1.tmr1(), m_area, 155, 90);
			m_components["tmr1"] = tmr1;
			GenericDiagram *tmr1_out = new GenericDiagram(155, 100, m_area);
			m_components["tmr1.out"] = tmr1_out;
			tmr1_out->add(ConnectionDiagram::pt(0, 0).first());
			tmr1_out->add(ConnectionDiagram::pt(-20, 0));
			tmr1_out->add(ConnectionDiagram::pt(-20, -40));
			tmr1_out->add(new VssSymbol(-20, -40, DIRECTION::LEFT));
			tmr1_out->add(ConnectionDiagram::text(5, -12, "TMR1"));
			tmr1_out->add(ConnectionDiagram::text(-80, -22, "Set flag bit\nTMR1IF on\nOverflow"));
		}


		void draw_trace() {
			TraceDiagram *trace = new TraceDiagram(m_trace, m_area, 100, 170);
			m_components["trace"] = trace;
		}

	  protected:

		void timer1_changed(Timer1 *t, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		void fosc_changed(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

	  public :
		void process_queue(){
			sleep_for_us(100);
		}

		Timer1Diagram(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_TMR1"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade),
			m_trace({&m_cpu.tmr1.rb6(), &m_cpu.tmr1.fosc(),
					 &m_cpu.tmr1.pscale().rd(), &m_cpu.tmr1.synch().bit(0)}) {

			pix_extents(650,450);

			DeviceEvent<Timer1>::subscribe<Timer1Diagram>(this, &Timer1Diagram::timer1_changed);
			DeviceEvent<Connection>::subscribe<Timer1Diagram>(this, &Timer1Diagram::fosc_changed, &m_cpu.tmr1.fosc());

			draw_rb6();
			draw_rb7();
			draw_t1osc();
			draw_t1oscen();
			draw_trigger();
			draw_t1csmux();
			draw_prescaler();
			draw_synch();
			draw_t1syncmux();
			draw_tmr1on();
			draw_tmr1();
			draw_trace();
		}

		~Timer1Diagram() {
			DeviceEvent<Timer1>::unsubscribe<Timer1Diagram>(this, &Timer1Diagram::timer1_changed);
			DeviceEvent<Connection>::unsubscribe<Timer1Diagram>(this, &Timer1Diagram::fosc_changed, &m_cpu.tmr1.fosc());
		}

	};

	class Timer1Component: public Component  {
		Timer1Diagram m_diagram;
		bool m_exiting;

		bool process_queue(){
			m_diagram.process_queue();
			return !m_exiting;
		}

		virtual void exiting(){
			m_exiting = true;
		}

	  public:
		Timer1Component(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade): Component(),
		  m_diagram(a_cpu, a_refGlade), m_exiting(false) {
			Glib::signal_idle().connect( sigc::mem_fun(*this, &Timer1Component::process_queue) );
		};
	};

}
