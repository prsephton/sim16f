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

	class PortB3: public CairoDrawing {
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

			cr->move_to(200, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Device RB3/CCP");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void draw_data_bus() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Data Bus"]);
			wire.add(WireDiagram::pt(0,  53, true));
			wire.add(WireDiagram::pt(100, 53));
			wire.add(WireDiagram::pt(70,  53, true,true));
			wire.add(WireDiagram::pt(70, 285));
			wire.add(WireDiagram::pt(210,285));
			wire.add(WireDiagram::pt(70, 144, true,true));
			wire.add(WireDiagram::pt(100,144));
			wire.add(WireDiagram::pt(70, 250, true,true));
			wire.add(WireDiagram::pt(120,250));
			wire.add(WireDiagram::text(0, 51, "Data bus"));
		}

		void draw_datalatch_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Datalatch.Q"]);
			conn.add(ConnectionDiagram::pt(70,55,true));
			conn.add(ConnectionDiagram::pt(85,55));
			conn.add(ConnectionDiagram::pt(85,45));
			conn.add(ConnectionDiagram::pt(105,45));
		}


		void draw_trislatch_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Trislatch.Q"]);

			// Connection to AndGate "Out Enable"
			conn.add(ConnectionDiagram::pt(70, 15, true));
			conn.add(ConnectionDiagram::pt(115, 15));

			// connection to RBPU_AND
			conn.add(ConnectionDiagram::pt(90, 15,true,true));
			conn.add(ConnectionDiagram::pt(90, -160));
			conn.add(ConnectionDiagram::pt(170,-160));

			// connection to Tristate3 input
			conn.add(ConnectionDiagram::pt(90, 15,true));
			conn.add(ConnectionDiagram::pt(90, 120));
			conn.add(ConnectionDiagram::pt(50, 120));

		}

		void draw_datamux() {
			ConnectionDiagram &dmux = dynamic_cast<ConnectionDiagram &>(*m_components["dMUX"]);
			dmux.add(new MuxSymbol(0,0,0,1,2));
			dmux.add(ConnectionDiagram::pt(10,0).first());
			dmux.add(ConnectionDiagram::pt(65,0));
		}

		void draw_out_enable() {
			ConnectionDiagram &out_en = dynamic_cast<ConnectionDiagram &>(*m_components["CCP.Out_en"]);
			out_en.add(new AndSymbol(2, 0, 0, 0, false));
			out_en.add(ConnectionDiagram::pt(30, 0).first());
			out_en.add(ConnectionDiagram::pt(70, 0));
			out_en.add(ConnectionDiagram::pt(70, -100));
		}

		void draw_pin_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Pin Wire"]);
			wire.add(WireDiagram::pt(400, 125, true));
			wire.add(WireDiagram::pt(530, 125));
			wire.add(WireDiagram::pt(500, 125, true, true));
			wire.add(WireDiagram::pt(500, 375));

			//  TTL Input buffer
			wire.add(WireDiagram::pt(480, 375));
			wire.add(new BufferSymbol(480, 375, M_PI));

			// Wire between input buffer and input latch
			wire.add(WireDiagram::pt(450, 375, true));
			wire.add(WireDiagram::pt(430, 375));

			// Wire from PBPU MOS to pin horizontal
			wire.add(WireDiagram::pt(480,  80, true));
			wire.add(WireDiagram::pt(480, 125, false, true));

			// Wire continuation to INT schmitte trigger
			wire.add(WireDiagram::pt(500, 375, true, true));
			wire.add(WireDiagram::pt(500, 490));
			wire.add(WireDiagram::pt(430, 490));

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


		void draw_schmitt() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SchmittOut"]);
			conn.add(ConnectionDiagram::pt(  0, 45, true));
			conn.add(ConnectionDiagram::pt(  0, 74));
			conn.add(ConnectionDiagram::pt(-60, 74));
		}

		void draw_rd_trisb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_TRISB"]);
			conn.add(ConnectionDiagram::pt(0,   40, true));
			conn.add(ConnectionDiagram::pt(140, 40));
			conn.add(ConnectionDiagram::pt(140, 30));
			conn.add(ConnectionDiagram::text(0, 38, "RD TrisB"));
		}

		void draw_rd_portb() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_PORTB"]);
			conn.add(ConnectionDiagram::pt(0,   55, true));
			conn.add(ConnectionDiagram::pt(225, 55));
			conn.add(ConnectionDiagram::pt(225,-15));
			conn.add(ConnectionDiagram::pt(225, 55, true, true));
			conn.add(ConnectionDiagram::pt(240, 55));
			conn.add(ConnectionDiagram::text(0, 53, "RD PortB"));
		}

		void draw_inverter1_out() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Inverter1 out"]);
			conn.add(ConnectionDiagram::pt(  0,  0, true));
			conn.add(ConnectionDiagram::pt( 80,  0));
			conn.add(ConnectionDiagram::pt( 80,-40));
			conn.add(ConnectionDiagram::pt( 60,-40));
		}

		void draw_output_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Output.Q"]);
			conn.add(ConnectionDiagram::pt(  0,  54, true));
			conn.add(ConnectionDiagram::pt( -20, 54));
		}

		void draw_rbpu() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RBPU"]);
			conn.add(ConnectionDiagram::pt(   0,  0, true));
			conn.add(ConnectionDiagram::pt( 268,  0, false, false, true));
			conn.add(new AndSymbol(3, 270, 10, 0, true));
			conn.add(ConnectionDiagram::text(0, -2, "RBPU").overscore());
		}

		void draw_CCP1CON() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["CCP1CON"]);
			conn.add(ConnectionDiagram::pt(   0,  0).first());
			conn.add(ConnectionDiagram::pt( 268,  0).invert());
			conn.add(ConnectionDiagram::pt( 210,  0).first().join());
			conn.add(ConnectionDiagram::pt( 210, 30));
			conn.add(ConnectionDiagram::text(0, -2, "CCP1CON"));
		}

		void draw_rbpu_and() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RBPU_AND"]);
			conn.add(ConnectionDiagram::pt( 305,  10, true));
			conn.add(ConnectionDiagram::pt( 360,  10, false, false, true));
			conn.add(new FETSymbol(360, 10, 0, false, false, true));
		}

		void draw_ccp_out() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["CCP.out"]);
			conn.add(ConnectionDiagram::pt(   0,  0).first());
			conn.add(ConnectionDiagram::pt( 205,  0));
			conn.add(ConnectionDiagram::text(0, -2, "CCP Output"));
		}

		void draw_peripheral_oe() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Peripheral.OE"]);
			conn.add(ConnectionDiagram::pt(   0,  0).first());
			conn.add(ConnectionDiagram::pt( 200,  0));
			conn.add(ConnectionDiagram::pt( 200,  -75));
			conn.add(ConnectionDiagram::pt( 215,  -75));
			conn.add(ConnectionDiagram::text(0, -2, "Peripheral OE").overscore());
		}

		void draw_ccp_rec() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["REC_WIRE"]);
			wire.add(WireDiagram::pt(295, 0, true));
			wire.add(WireDiagram::pt(0, 0));
			wire.add(WireDiagram::text(8, -2, "CCP In"));
			wire.add(new VssSymbol(0, 0, M_PI*0.5));
		}

		void on_wire_change(Wire *wire, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		void on_port_change(BasicPort *conn, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		virtual ~PortB3() {
			auto &p3 = dynamic_cast<PortB_RB3 &>(*(m_cpu.portb.RB[3]));
			auto &c = p3.components();

			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			DeviceEvent<Wire>::unsubscribe<PortB3>(this, &PortB3::on_wire_change, &DataBus);
			DeviceEvent<BasicPort>::unsubscribe<PortB3>(this, &PortB3::on_port_change, &p3);
		}



		PortB3(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_RB3"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			pix_extents(600,520);
			auto &p3 = dynamic_cast<PortB_RB3 &>(*(m_cpu.portb.RB[3]));
			auto &c = p3.components();
			Latch &DataLatch = dynamic_cast<Latch &>(*(c["Data Latch"]));
			Latch &TrisLatch = dynamic_cast<Latch &>(*(c["Tris Latch"]));
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Wire &PinWire = dynamic_cast<Wire &> (*(c["Pin Wire"]));
			Tristate &Tristate1 = dynamic_cast<Tristate &> (*(c["Tristate1"]));
			Tristate &Tristate2 = dynamic_cast<Tristate &> (*(c["Tristate2"]));
			Tristate &Tristate3 = dynamic_cast<Tristate &> (*(c["Tristate3"]));
			Latch &OutputLatch = dynamic_cast<Latch &>(*(c["SR1"]));
			Inverter &Inverter1 = dynamic_cast<Inverter &> (*(c["Inverter1"]));
			Clamp &Clamp1 = dynamic_cast<Clamp &> (*(c["PinClamp"]));
			AndGate &RBPU = dynamic_cast<AndGate &> (*(c["RBPU_NAND"]));

			Schmitt &trigger = dynamic_cast<Schmitt &> (*(c["TRIGGER"]));
			Mux &dMux = dynamic_cast<Mux &>(*c["Data MUX"]);
			AndGate &Out_en = dynamic_cast<AndGate &>(*c["Out Enable"]);

			Wire &REC_wire = dynamic_cast<Wire &> (*(c["CCP_REC_WIRE"]));

			DeviceEvent<Wire>::subscribe<PortB3>(this, &PortB3::on_wire_change, &DataBus);
			DeviceEvent<BasicPort>::subscribe<PortB3>(this, &PortB3::on_port_change, &p3);

			m_components["Data Latch"] = new LatchDiagram(DataLatch, true, 200.0, 130.0, m_area);
			m_components["Tris Latch"] = new LatchDiagram(TrisLatch, true, 200.0, 220.0, m_area);
			m_components["Data Bus"]   = new WireDiagram( DataBus,   100.0,  90.0, m_area);
			m_components["Pin Wire"]   = new WireDiagram( PinWire,   0.0,  0.0, m_area);
			m_components["Tristate1"]  = new TristateDiagram( Tristate1, true, 370.0, 125.0, m_area);
			m_components["Datalatch.Q"] = new ConnectionDiagram(DataLatch.Q(), 200, 90, m_area);
			m_components["Trislatch.Q"]  = new ConnectionDiagram(TrisLatch.Q(), 200, 220, m_area);
			m_components["Pin"]  = new PinDiagram(p3.pin(), 530, 125, 0, 1, m_area);
			m_components["WR_PORTB"]  = new ConnectionDiagram(DataLatch.Ck(), 100, 90, m_area);
			m_components["WR_TRISB"]  = new ConnectionDiagram(TrisLatch.Ck(), 100, 210, m_area);
			m_components["Tristate2"]  = new TristateDiagram(Tristate2, false, 340, 375, m_area);
			m_components["Tristate3"]  = new TristateDiagram(Tristate3, false, 250, 340, m_area);
			m_components["Inverter1"]  = new InverterDiagram(Inverter1, 340, 455, CairoDrawing::DIRECTION::RIGHT, m_area);
			m_components["Output Latch"]= new LatchDiagram(OutputLatch, false, 360.0, 360.0, m_area);
			m_components["RD_TRISB"]  = new ConnectionDiagram(Tristate3.gate(), 100, 320, m_area);
			m_components["RD_PORTB"]  = new ConnectionDiagram(Tristate2.gate(), 100, 400, m_area);
			m_components["Inverter1 out"]  = new ConnectionDiagram(Inverter1.rd(), 365, 455, m_area);
			m_components["Output.Q"]= new ConnectionDiagram(OutputLatch.Q(), 360.0, 320.0, m_area);
			m_components["Clamp"]= new ClampDiagram(Clamp1, 515.0,125.0, m_area);
			m_components["RBPU"]= new ConnectionDiagram(p3.RBPU(), 100.0, 50.0, m_area);
			m_components["CCP1CON"]= new ConnectionDiagram(p3.CCP1CON(), 100.0, 70.0, m_area);
			m_components["RBPU_AND"]= new ConnectionDiagram(RBPU.rd(), 100.0, 50.0, m_area);
			m_components["Schmitt"]  = new SchmittDiagram(trigger, 430, 490, CairoDrawing::DIRECTION::LEFT, false, m_area);
			m_components["REC_WIRE"] = new WireDiagram(REC_wire, 105, 490, m_area);
			m_components["CCP.Out_en"]= new ConnectionDiagram(Out_en.rd(), 315.0, 240.0, m_area);
			m_components["dMUX"]= new ConnectionDiagram(dMux.rd(), 305.0, 125.0, m_area);
			m_components["CCP.out"]= new ConnectionDiagram(p3.CCP_Out(), 100.0, 115.0, m_area);
			m_components["Peripheral.OE"]= new ConnectionDiagram(p3.Peripheral_OE(), 100.0, 320.0, m_area);

			draw_data_bus();
			draw_datalatch_q();
			draw_trislatch_q();
			draw_out_enable();
			draw_pin_wire();
			draw_wr_portb();
			draw_wr_trisb();
			draw_rbpu();
			draw_CCP1CON();
			draw_rbpu_and();
			draw_rd_trisb();
			draw_rd_portb();
			draw_inverter1_out();
			draw_output_q();
			draw_ccp_rec();
			draw_ccp_out();
			draw_peripheral_oe();
			draw_datamux();
		}

	};
}
