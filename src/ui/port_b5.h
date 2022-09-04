#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "paint/cairo_drawing.h"
#include "../utils/smart_ptr.h"
#include "paint/common.h"
#include "paint/diagrams.h"

namespace app {

	class PortB5: public CairoDrawing {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
	  protected:
		std::map<std::string, SmartPtr<Component> > m_components;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
			show_coords(cr);
			cr->move_to(400, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Device RB5");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void draw_data_bus() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Data Bus"]);
			wire.add(WireDiagram::pt(0,  53, true));
			wire.add(WireDiagram::pt(100, 53));
			wire.add(WireDiagram::pt(70,  53, true,true));
			wire.add(WireDiagram::pt(70, 315));
			wire.add(WireDiagram::pt(120,315));
			wire.add(WireDiagram::pt(70, 144, true,true));
			wire.add(WireDiagram::pt(100,144));
			wire.add(WireDiagram::pt(70, 270, true,true));
			wire.add(WireDiagram::pt(120,270));
			wire.add(WireDiagram::text(0, 51, "Data bus"));
		}

		void draw_datalatch_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Datalatch.Q"]);
			conn.add(ConnectionDiagram::pt(70,53,true));
			conn.add(ConnectionDiagram::pt(170,53));
		}


		void draw_trislatch_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Trislatch.Q"]);

			// Connection to Tristate1 gate
			conn.add(ConnectionDiagram::pt(70, 15, true));
			conn.add(ConnectionDiagram::pt(185, 15));
			conn.add(ConnectionDiagram::pt(185,-60));

			// connection to RBPU_AND
			conn.add(ConnectionDiagram::pt(90, 15,true,true));
			conn.add(ConnectionDiagram::pt(90, -110));
			conn.add(ConnectionDiagram::pt(170,-110));

			// connection to Tristate3 input
			conn.add(ConnectionDiagram::pt(90, 15,true));
			conn.add(ConnectionDiagram::pt(90, 140));
			conn.add(ConnectionDiagram::pt(50, 140));

			// connection to RBIF And Gate
			conn.add(ConnectionDiagram::pt(90, 140).first().join());
			conn.add(ConnectionDiagram::pt(90, 250));
			conn.add(ConnectionDiagram::pt(65, 250));
		}

//		void draw_out_enable() {
//			ConnectionDiagram &out_en = dynamic_cast<ConnectionDiagram &>(*m_components["Out_en"]);
//			out_en.add(new OrSymbol(0, 0, 0, false));
//			out_en.add(ConnectionDiagram::pt(45, 0).first());
//			out_en.add(ConnectionDiagram::pt(55, 0));
//			out_en.add(ConnectionDiagram::pt(55, -85));
//		}

		void draw_pin_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Pin Wire"]);
			wire.add(WireDiagram::pt(400, 105, true));
			wire.add(WireDiagram::pt(530, 105));
			wire.add(WireDiagram::pt(500, 105, true, true));
			wire.add(WireDiagram::pt(500, 310));

			//  TTL Input buffer
			wire.add(new BufferSymbol(500, 310, DIRECTION::DOWN));

			// Wire from PBPU MOS to pin horizontal
			wire.add(WireDiagram::pt(480,  80, true));
			wire.add(WireDiagram::pt(480, 105, false, true));

			// Wire between input buffer and input latch
			wire.add(WireDiagram::pt(500, 340).first());
			wire.add(WireDiagram::pt(500, 365));
			wire.add(WireDiagram::pt(480, 365));

			// Wire continuation to second input latch
			wire.add(WireDiagram::pt(500, 365).first().join());
			wire.add(WireDiagram::pt(500, 455));
			wire.add(WireDiagram::pt(480, 455));

		}


		void draw_wr_portb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["WR_PORTB"]);
			conn.add(ConnectionDiagram::pt(0,   96, true));
			conn.add(ConnectionDiagram::pt(100, 96));
			conn.add(ConnectionDiagram::text(0,  94, "WR PortB"));
		}


		void draw_wr_trisb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["WR_TRISB"]);
			conn.add(ConnectionDiagram::pt(0,   66, true));
			conn.add(ConnectionDiagram::pt(100, 66));
			conn.add(ConnectionDiagram::text(0,  64, "WR TrisB"));
		}


//		void draw_schmitt() {
//			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SchmittOut"]);
//			conn.add(ConnectionDiagram::pt(  0, 45, true));
//			conn.add(ConnectionDiagram::pt(  0, 74));
//			conn.add(ConnectionDiagram::pt(-60, 74));
//		}

//		void draw_lvp() {
//			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["LVP"]);
//			conn.add(ConnectionDiagram::pt(0,   0, true));
//			conn.add(ConnectionDiagram::pt(205, 0));
//
//			conn.add(ConnectionDiagram::pt(205, -230));
//			conn.add(ConnectionDiagram::pt(270, -230).invert());
//
//			conn.add(ConnectionDiagram::pt(205, -95).first().join());
//			conn.add(ConnectionDiagram::pt(230, -95));
//
//			conn.add(ConnectionDiagram::pt(205, 0).first().join());
//			conn.add(ConnectionDiagram::pt(205, 140));
//			conn.add(ConnectionDiagram::pt(170, 140).invert());
//
//			conn.add(ConnectionDiagram::text(0, -2, "LVP (configuration bit)"));
//		}

		void draw_rd_trisb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_TRISB"]);
			conn.add(ConnectionDiagram::pt(0,   0, true));
			conn.add(ConnectionDiagram::pt(140, 0));
			conn.add(ConnectionDiagram::pt(140, -10));
			conn.add(ConnectionDiagram::text(0, -2, "RD TrisB"));
		}

		void draw_rd_portb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_PORTB"]);
			conn.add(ConnectionDiagram::pt(0,   0, true));
			conn.add(ConnectionDiagram::pt(140, 0));
			conn.add(ConnectionDiagram::pt(140, -10));
			conn.add(ConnectionDiagram::text(0, -2, "RD PortB"));

			//  connect to AND(Q3-RDport)
			conn.add(ConnectionDiagram::pt(70, 0).first().join());
			conn.add(ConnectionDiagram::pt(70, 145));
			conn.add(ConnectionDiagram::pt(460, 145));
			conn.add(ConnectionDiagram::pt(460, 120));
			conn.add(ConnectionDiagram::pt(440, 120));

		}

		void draw_q1() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Q1"]);
			conn.add(ConnectionDiagram::pt(0,   0, true));
			conn.add(ConnectionDiagram::pt(-80, 0));
			conn.add(ConnectionDiagram::text(2, 5, "Q1"));
		}

		void draw_q3() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Q3"]);
			conn.add(ConnectionDiagram::pt(0,   0, true));
			conn.add(ConnectionDiagram::pt(-20, 0));
			conn.add(ConnectionDiagram::text(2, 5, "Q3"));
		}

		void draw_SR2en() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SR2en"]);
			conn.add(new AndSymbol(2, 0, 0, DIRECTION::LEFT));
			conn.add(ConnectionDiagram::pt(-45,   0, true));
			conn.add(ConnectionDiagram::pt(-60, 0));
		}

		void draw_sr1_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SR1.Q"]);
			conn.add(ConnectionDiagram::pt(  0,  0, true));
			conn.add(ConnectionDiagram::pt( -20, 0));
			// Connect to XOR(pin_changed)
			conn.add(ConnectionDiagram::pt( -20, 80));
			conn.add(ConnectionDiagram::pt( -40, 80));
			// Connect to Tristate2
			conn.add(ConnectionDiagram::pt( -20, 0).first().join());
			conn.add(ConnectionDiagram::pt( -160, 0));

		}

		void draw_sr2_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SR2.Q"]);
			conn.add(ConnectionDiagram::pt(  0,  0, true));
			conn.add(ConnectionDiagram::pt( -40, 0));
		}

		void draw_pin_changed() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["PIN_Changed"]);
			conn.add(new OrSymbol(2, 0, 0, DIRECTION::LEFT, false, true));
			conn.add(ConnectionDiagram::pt(   -45,  0).first());
			conn.add(ConnectionDiagram::pt(  -105,  0));
		}

		void draw_RBIF() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RBIF"]);
			conn.add(new AndSymbol(3, 0, 0, DIRECTION::LEFT));
			conn.add(ConnectionDiagram::pt(   -45,  0).first());
			conn.add(ConnectionDiagram::pt(  -165,  0));
			conn.add(ConnectionDiagram::text(-160, -2, "Set RBIF"));
			conn.add(new VssSymbol(-165, 0, M_PI*0.5));
		}

		void draw_rbpu() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RBPU"]);
			conn.add(ConnectionDiagram::pt(   0,  0, true));
			conn.add(ConnectionDiagram::pt( 268,  0, false, false, true));
			conn.add(new AndSymbol(3, 270, 10, 0, true));
			conn.add(ConnectionDiagram::text(0, -2, "RBPU").overscore());
		}

		void draw_rbpu_and() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RBPU_AND"]);
			conn.add(ConnectionDiagram::pt( 320,  10, true));
			conn.add(ConnectionDiagram::pt( 360,  10, false, false, true));
			conn.add(new FETSymbol(360, 10, 0, false, false, true));
		}

//		void draw_pgm_rec() {
//			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["PGM"]);
//			wire.add(WireDiagram::pt(300, 0, true));
//			wire.add(WireDiagram::pt(0, 0));
//			wire.add(WireDiagram::text(8, -2, "PGM Input"));
//			wire.add(new VssSymbol(0, 0, M_PI*0.5));
//		}

		void on_wire_change(Wire *wire, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		void on_port_change(BasicPort *conn, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}


		virtual ~PortB5() {
			auto &p5 = dynamic_cast<PortB_RB5 &>(*(m_cpu.portb.RB[5]));
			auto &c = p5.components();
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));

			DeviceEvent<Wire>::unsubscribe<PortB5>(this, &PortB5::on_wire_change, &DataBus);
			DeviceEvent<BasicPort>::unsubscribe<PortB5>(this, &PortB5::on_port_change, &p5);
		}

		PortB5(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_RB5"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			auto &p5 = dynamic_cast<PortB_RB5 &>(*(m_cpu.portb.RB[5]));
			auto &c = p5.components();
			Latch &DataLatch = dynamic_cast<Latch &>(*(c["Data Latch"]));
			Latch &TrisLatch = dynamic_cast<Latch &>(*(c["Tris Latch"]));
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Wire &PinWire = dynamic_cast<Wire &> (*(c["Pin Wire"]));
			Tristate &Tristate1 = dynamic_cast<Tristate &> (*(c["Tristate1"]));
			Tristate &Tristate2 = dynamic_cast<Tristate &> (*(c["Tristate2"]));
			Tristate &Tristate3 = dynamic_cast<Tristate &> (*(c["Tristate3"]));
			Latch &SR1 = dynamic_cast<Latch &>(*(c["SR1"]));
			Latch &SR2 = dynamic_cast<Latch &>(*(c["SR2"]));

			Clamp &Clamp1 = dynamic_cast<Clamp &> (*(c["PinClamp"]));
			AndGate &RBPU = dynamic_cast<AndGate &> (*(c["RBPU_NAND"]));

			AndGate &SR2en = dynamic_cast<AndGate &> (*(c["AND(Q3,rdPort)"]));
			XOrGate &PIN_Changed = dynamic_cast<XOrGate &>(*c["XOR(SR1.Q, SR2.Q)"]);

			AndGate &RBIF = dynamic_cast<AndGate &> (*(c["AND(TrisLatch.Q, XOr1)"]));

			DeviceEvent<Wire>::subscribe<PortB5>(this, &PortB5::on_wire_change, &DataBus);
			DeviceEvent<BasicPort>::subscribe<PortB5>(this, &PortB5::on_port_change, &p5);

			m_components["Data Latch"] = new LatchDiagram(DataLatch, true, 200.0, 90.0, m_area);
			m_components["Tris Latch"] = new LatchDiagram(TrisLatch, true, 200.0, 180.0, m_area);
			m_components["Data Bus"]   = new WireDiagram( DataBus,   100.0,  50.0, m_area);
			m_components["Pin Wire"]   = new WireDiagram( PinWire,   0.0,  0.0, m_area);
			m_components["Tristate1"]  = new TristateDiagram( Tristate1, true, 370.0, 105.0, m_area);
			m_components["Datalatch.Q"] = new ConnectionDiagram(DataLatch.Q(), 200, 50, m_area);
			m_components["Trislatch.Q"]  = new ConnectionDiagram(TrisLatch.Q(), 200, 180, m_area);
			m_components["Pin"]  = new PinDiagram(p5.pin(), 530, 105, 0, 1, m_area);
			m_components["WR_PORTB"]  = new ConnectionDiagram(DataLatch.Ck(), 100, 50, m_area);
			m_components["WR_TRISB"]  = new ConnectionDiagram(TrisLatch.Ck(), 100, 170, m_area);
			m_components["Tristate2"]  = new TristateDiagram(Tristate2, false, 250, 365, m_area);
			m_components["Tristate3"]  = new TristateDiagram(Tristate3, false, 250, 320, m_area);
			m_components["SR1"]= new LatchDiagram(SR1, false, 410.0, 350.0, m_area);
			m_components["SR2"]= new LatchDiagram(SR2, false, 410.0, 440.0, m_area);
			m_components["RD_TRISB"]  = new ConnectionDiagram(Tristate3.gate(), 100, 340, m_area);
			m_components["RD_PORTB"]  = new ConnectionDiagram(Tristate2.gate(), 100, 385, m_area);
			m_components["Clamp"]= new ClampDiagram(Clamp1, 515.0,105.0, m_area);
			m_components["RBPU"]= new ConnectionDiagram(p5.RBPU(), 100.0, 50.0, m_area);
			m_components["RBPU_AND"]= new ConnectionDiagram(RBPU.rd(), 100.0, 50.0, m_area);

			m_components["Q1"]  = new ConnectionDiagram(p5.Q1(), 560, 405, m_area);
			m_components["Q3"]  = new ConnectionDiagram(p5.Q3(), 560, 485, m_area);
			m_components["SR2en"]= new ConnectionDiagram(SR2en.rd(), 540, 495, m_area);

			m_components["SR1.Q"]= new ConnectionDiagram(SR1.Q(), 410.0, 365.0, m_area);
			m_components["SR2.Q"]= new ConnectionDiagram(SR2.Q(), 410.0, 455.0, m_area);

			m_components["PIN_Changed"]= new ConnectionDiagram(PIN_Changed.rd(), 370, 450, m_area);
			m_components["RBIF"]= new ConnectionDiagram(RBIF.rd(), 265, 440, m_area);

			draw_data_bus();
			draw_datalatch_q();
			draw_trislatch_q();
			draw_pin_wire();
			draw_wr_portb();
			draw_wr_trisb();
			draw_rbpu();
			draw_rbpu_and();
			draw_rd_trisb();
			draw_rd_portb();
			draw_sr1_q();
			draw_sr2_q();
			draw_q1();
			draw_q3();
			draw_SR2en();
			draw_pin_changed();
			draw_RBIF();
		}

	};
}
