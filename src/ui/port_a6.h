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

	class PortA6: public CairoDrawing {
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
			cr->set_line_width(0.1);
			show_coords(cr);
			cr->move_to(400, 50);
			cr->scale(2.0, 2.0);
			cr->text_path("Device RA6/OSC2/CLKOUT");
			cr->fill_preserve(); cr->stroke();

			cr->restore();
			return false;
		}

		void draw_data_bus() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Data Bus"]);
			wire.add(WireDiagram::pt(0,   23, true));
			wire.add(WireDiagram::pt(100, 23));                  // Port D
			wire.add(WireDiagram::pt(70,  23, true,true));
			wire.add(WireDiagram::pt(70, 305));                  // Bottom Corner
			wire.add(WireDiagram::pt(120,305));                  // Rd Port
			wire.add(WireDiagram::pt(70, 145, true,true));
			wire.add(WireDiagram::pt(100,145));                  // Tris D
			wire.add(WireDiagram::pt(70, 240, true,true));
			wire.add(WireDiagram::pt(120,240));                  // Rd Tris
			wire.add(WireDiagram::text(0, 21, "Data bus"));
		}

		void draw_dataq_output() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["DataLatch.Q"]);
			conn.add(ConnectionDiagram::pt( 70, 23, true));
			conn.add(ConnectionDiagram::pt( 180, 23));

		}

		void draw_pin_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Pin Wire"]);
			wire.add(WireDiagram::pt(480, 150, true));
			wire.add(WireDiagram::pt(630, 150));
			wire.add(WireDiagram::pt(595, 150, true, true));
			wire.add(WireDiagram::pt(595, 380));
			wire.add(WireDiagram::pt(550, 150, true, true));
			wire.add(WireDiagram::pt(550, 100));
			wire.add(WireDiagram::pt(535, 100));
		}


		void draw_wr_porta() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["WR_PORTA"]);
			conn.add(ConnectionDiagram::pt(0,   66, true));
			conn.add(ConnectionDiagram::pt(100, 66));
			conn.add(ConnectionDiagram::text(0,  64, "WR PortA"));
		}


		void draw_wr_trisa() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["WR_TRISA"]);
			conn.add(ConnectionDiagram::pt(0,   66, true));
			conn.add(ConnectionDiagram::pt(100, 66));
			conn.add(ConnectionDiagram::text(0,  64, "WR TrisA"));
		}


		void draw_schmitt() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SchmittOut"]);
			conn.add(ConnectionDiagram::pt(   0, 45, true));
			conn.add(ConnectionDiagram::pt(   0, 64));
			conn.add(ConnectionDiagram::pt(-160, 64));
		}

		void draw_trislatch_qc() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["TrisLatch Qc"]);
			conn.add(ConnectionDiagram::pt(  0,  55, true));
			conn.add(ConnectionDiagram::pt( 50,  55));
			conn.add(ConnectionDiagram::pt( 10,  55, false, true));
			conn.add(ConnectionDiagram::pt( 10, 110));
			conn.add(ConnectionDiagram::pt(-20, 110));
		}

		void draw_rd_trisa() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_TRISA"]);
			conn.add(ConnectionDiagram::pt(0,   20, true));
			conn.add(ConnectionDiagram::pt(140, 20));
			conn.add(ConnectionDiagram::pt(140, 10));
			conn.add(ConnectionDiagram::text(0, 18, "RD TrisA"));
		}

		void draw_rd_porta() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_PORTA"]);
			conn.add(ConnectionDiagram::pt(0,   75, true));
			conn.add(ConnectionDiagram::pt(140, 75));
			conn.add(ConnectionDiagram::pt(140, 10));
			conn.add(ConnectionDiagram::pt(140, 75, true, true));
			conn.add(ConnectionDiagram::pt(200, 75));
			conn.add(ConnectionDiagram::text(0, 73, "RD PortA"));
		}

		void draw_inverter1_out() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Inverter1 out"]);
			conn.add(ConnectionDiagram::pt(  0,  0, true));
			conn.add(ConnectionDiagram::pt( 150,  0));
			conn.add(ConnectionDiagram::pt( 150,-35));
			conn.add(ConnectionDiagram::pt( 100,-35));
		}

		void draw_output_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Output.Q"]);
			conn.add(ConnectionDiagram::pt(  0,  14, true));
			conn.add(ConnectionDiagram::pt(-110, 14));
		}

		void draw_mux_output() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Mux.out"]);
			conn.add(ConnectionDiagram::pt(  10,  0, true));
			conn.add(ConnectionDiagram::pt(  70,  0));
		}

		void draw_mux_inputs() {
			ConnectionDiagram &in0 = dynamic_cast<ConnectionDiagram &>(*m_components["Mux.in1"]);
			in0.add(ConnectionDiagram::pt(  0,   -10, true));
			in0.add(ConnectionDiagram::pt( -180,  -10));
			in0.add(ConnectionDiagram::text(-180,  -12, "CLKOUT(Fosc/4)"));

			ConnectionDiagram &s0 = dynamic_cast<ConnectionDiagram &>(*m_components["Mux.s0"]);
			s0.add(ConnectionDiagram::pt(     5,  28, true));
			s0.add(ConnectionDiagram::pt(     5, 100));
			s0.add(ConnectionDiagram::pt(  -280, 100));
			s0.add(ConnectionDiagram::text(-280,  98, "Fosc=101,111"));
			s0.add(ConnectionDiagram::pt(  5, 100, true, true));
			s0.add(ConnectionDiagram::pt(  5, 160));
			s0.add(ConnectionDiagram::pt( 20, 160));
		}

		void draw_and1() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["And1"]);
			conn.add(new AndSymbol());
			conn.add(ConnectionDiagram::pt(  45,  0, true));
			conn.add(ConnectionDiagram::pt(  80,  0));
		}

		void draw_nor1() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["NOR1"]);
			conn.add(new OrSymbol(0,0,0,true));
			conn.add(ConnectionDiagram::pt(  45,   0, true));
			conn.add(ConnectionDiagram::pt(  65,   0));
			conn.add(ConnectionDiagram::pt(  65,-160));
		}

		void draw_osc() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["OSC"]);
			conn.add(ConnectionDiagram::pt(  0,    0, true));
			conn.add(ConnectionDiagram::pt(  365,  0));
			conn.add(ConnectionDiagram::text(0, -2, "From OSC1"));
			conn.add(new BlockSymbol(400, 0, 70, 20));
			conn.add(ConnectionDiagram::text(370, 5, "OSC Circuit"));
		}

		void draw_fosc2() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Fosc2"]);
			conn.add(ConnectionDiagram::pt(  0,    0, true));
			conn.add(ConnectionDiagram::pt( 420,   0));
			conn.add(ConnectionDiagram::pt( 420, -80));
			conn.add(ConnectionDiagram::pt( 485, -80));
			conn.add(ConnectionDiagram::pt( 485, -40));
			conn.add(ConnectionDiagram::text(0, -2, "Fosc=011,100,110"));
			conn.add(ConnectionDiagram::pt(200,   0, true, true));
			conn.add(ConnectionDiagram::pt(200, -85));
			conn.add(ConnectionDiagram::pt(220, -85));
		}


		void on_wire_change(Wire *wire, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		virtual ~PortA6() {
			auto &p6 = dynamic_cast<SinglePortA_RA6_CLKOUT &>(*(m_cpu.porta.RA[6]));
			auto &c = p6.components();
			Latch &DataLatch = dynamic_cast<Latch &>(*(c["Data Latch"]));
			Latch &TrisLatch = dynamic_cast<Latch &>(*(c["Tris Latch"]));
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Mux &Mux1 = dynamic_cast<Mux &> (*(c["Mux"]));
			DeviceEvent<Wire>::unsubscribe<PortA6>(this, &PortA6::on_wire_change, &DataBus);
			DeviceEvent<Connection>::unsubscribe<PortA6>(this, &PortA6::on_connection_change, &DataLatch.Q());
			DeviceEvent<Connection>::unsubscribe<PortA6>(this, &PortA6::on_connection_change, &TrisLatch.Q());
			DeviceEvent<Connection>::unsubscribe<PortA6>(this, &PortA6::on_connection_change, &Mux1.in(1));
		}


		PortA6(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_RA6"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			auto &p6 = dynamic_cast<SinglePortA_RA6_CLKOUT &>(*(m_cpu.porta.RA[6]));
			auto &c = p6.components();
			Latch &DataLatch = dynamic_cast<Latch &>(*(c["Data Latch"]));
			Latch &TrisLatch = dynamic_cast<Latch &>(*(c["Tris Latch"]));
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Wire &PinWire = dynamic_cast<Wire &> (*(c["Pin Wire"]));
			Schmitt &SchmittTrigger = dynamic_cast<Schmitt &> (*(c["Schmitt Trigger"]));
			Tristate &Tristate1 = dynamic_cast<Tristate &> (*(c["Tristate1"]));
			Tristate &Tristate2 = dynamic_cast<Tristate &> (*(c["Tristate2"]));
			Tristate &Tristate3 = dynamic_cast<Tristate &> (*(c["Tristate3"]));
			Latch &OutputLatch = dynamic_cast<Latch &>(*(c["SR1"]));
			Inverter &Inverter1 = dynamic_cast<Inverter &> (*(c["Inverter1"]));
			Mux &Mux1 = dynamic_cast<Mux &> (*(c["Mux"]));
			OrGate &Nor1 = dynamic_cast<OrGate &> (*(c["Nor1"]));
			AndGate &And1 = dynamic_cast<AndGate &> (*(c["And1"]));
			Clamp &Clamp1 = dynamic_cast<Clamp &> (*(c["PinClamp"]));

			DeviceEvent<Wire>::subscribe<PortA6>(this, &PortA6::on_wire_change, &DataBus);
			DeviceEvent<Connection>::subscribe<PortA6>(this, &PortA6::on_connection_change, &DataLatch.Q());
			DeviceEvent<Connection>::subscribe<PortA6>(this, &PortA6::on_connection_change, &TrisLatch.Q());
			DeviceEvent<Connection>::subscribe<PortA6>(this, &PortA6::on_connection_change, &Mux1.in(1));

			m_components["Data Latch"] = new LatchDiagram(DataLatch, true, 200.0,  150.0, m_area);
			m_components["DataLatch.Q"] = new ConnectionDiagram(DataLatch.Q(), 200, 140, m_area);
			m_components["Tris Latch"] = new LatchDiagram(TrisLatch, true, 200.0, 270.0, m_area);
			m_components["TrisLatch Qc"]  = new ConnectionDiagram(TrisLatch.Qc(), 270, 270, m_area);
			m_components["Data Bus"]   = new WireDiagram( DataBus,   100.0,  140.0, m_area);
			m_components["Pin Wire"]   = new WireDiagram( PinWire,   0.0,  0.0, m_area);
			m_components["PinClamp"]  = new ClampDiagram(Clamp1, 615, 150, m_area);
			m_components["Pin"]  = new PinDiagram(p6.pin(), 630, 150, 0, 1, m_area);
			m_components["Tristate1"]  = new TristateDiagram(Tristate1, true, 450, 150, m_area);
			m_components["Schmitt"]  = new SchmittDiagram(SchmittTrigger, 590, 380, CairoDrawing::DIRECTION::DOWN, true, m_area);
			m_components["SchmittOut"]  = new ConnectionDiagram(SchmittTrigger.rd(), 590, 380, m_area);
			m_components["WR_PORTA"]  = new ConnectionDiagram(DataLatch.Ck(), 100, 140, m_area);
			m_components["WR_TRISA"]  = new ConnectionDiagram(TrisLatch.Ck(), 100, 260, m_area);
			m_components["Tristate2"]  = new TristateDiagram(Tristate2, false, 250, 445, m_area);
			m_components["RD_PORTA"]  = new ConnectionDiagram(Tristate2.gate(), 100, 445, m_area);
			m_components["Tristate3"]  = new TristateDiagram(Tristate3, false, 250, 380, m_area);
			m_components["RD_TRISA"]  = new ConnectionDiagram(Tristate3.gate(), 100, 380, m_area);
			m_components["Inverter1"]  = new InverterDiagram(Inverter1, 300, 520, CairoDrawing::DIRECTION::RIGHT, m_area);
			m_components["Output Latch"]= new LatchDiagram(OutputLatch, false, 360.0, 430.0, m_area);
			m_components["Output.Q"]= new ConnectionDiagram(OutputLatch.Q(), 360.0, 430.0, m_area);
			m_components["Inverter1 out"]  = new ConnectionDiagram(Inverter1.rd(), 330, 520, m_area);
			m_components["Mux"]= new MuxDiagram(Mux1, 380, 150, 0, m_area);
			m_components["Mux.out"]= new ConnectionDiagram(Mux1.rd(), 380, 150, m_area);
			m_components["Mux.s0"]= new ConnectionDiagram(Mux1.select(0), 380, 150, m_area);
			m_components["Mux.in1"]= new ConnectionDiagram(Mux1.in(1), 380, 150, m_area);
			m_components["And1"]  = new ConnectionDiagram(And1.rd(), 320, 330, m_area);
			m_components["NOR1"]  = new ConnectionDiagram(Nor1.rd(), 400, 320, m_area);
			m_components["Fosc2"]  = new ConnectionDiagram(p6.fosc2(), 100, 420, m_area);
			m_components["OSC"]  = new ConnectionDiagram(p6.osc(), 100, 100, m_area);


			draw_data_bus();
			draw_pin_wire();
			draw_wr_porta();
			draw_wr_trisa();
			draw_schmitt();
			draw_trislatch_qc();
			draw_rd_trisa();
			draw_rd_porta();
			draw_inverter1_out();
			draw_output_q();
			draw_mux_output();
			draw_dataq_output();
			draw_mux_inputs();
			draw_and1();
			draw_nor1();
			draw_fosc2();
			draw_osc();
		}

	};
}
