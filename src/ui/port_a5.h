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

	class PortA5: public CairoDrawing {
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
			cr->text_path("Device RA5/MCLR/Vpp");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void draw_data_bus() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Data Bus"]);
			wire.add(WireDiagram::pt(0,  210, true));
			wire.add(WireDiagram::pt(70, 210));
			wire.add(WireDiagram::pt(70, 340));
			wire.add(WireDiagram::pt(120,340));
			wire.add(WireDiagram::pt(70, 270, true,true));
			wire.add(WireDiagram::pt(120,270));
			wire.add(WireDiagram::text(0, 208, "Data bus"));
		}


		void draw_pin_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Pin Wire"]);
			wire.add(WireDiagram::pt(630, 200, true));
			wire.add(WireDiagram::pt(360, 200));
			wire.add(new BlockSymbol(320, 200, 80, 30));
			wire.add(WireDiagram::text(295, 205, "HV Detect"));
			wire.add(WireDiagram::pt(280, 200, true));
			wire.add(WireDiagram::pt(120, 200));
			wire.add(new VssSymbol(120, 200, CairoDrawing::DIRECTION::DOWN));
			wire.add(WireDiagram::text(130, 185, "Program\n mode"));
			wire.add(WireDiagram::pt(490, 200, true, true));
			wire.add(WireDiagram::pt(490, 250));
			wire.add(WireDiagram::pt(530, 200, true, true));
			wire.add(WireDiagram::pt(530, 140));
			wire.add(WireDiagram::pt(440, 140));
			wire.add(WireDiagram::pt(575, 200, true, true));
			wire.add(WireDiagram::pt(575, 230));
			wire.add(new DiodeSymbol(575, 230, CairoDrawing::DIRECTION::UP));
			wire.add(new VssSymbol(575, 235, CairoDrawing::DIRECTION::RIGHT));

		}


		void draw_mclre_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["MCLRE Wire"]);
			wire.add(WireDiagram::text(24, 0, "MCLRE (configuration bit)"));
			wire.add(WireDiagram::pt(20, 0, true));
			wire.add(WireDiagram::pt(0, 0));
			wire.add(WireDiagram::pt(0, 130));
			wire.add(WireDiagram::pt(220, 130));
			wire.add(WireDiagram::pt(220, 145, false, false, true));
			wire.add(WireDiagram::pt(0, 20, true, true));
			wire.add(WireDiagram::pt(-65, 20));
		}


		void draw_and1() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["And1.out"]);
			conn.add(new AndSymbol(0, 0, ConnectionDiagram::DIRECTION::LEFT, true));
			conn.add(ConnectionDiagram::pt(-50,0, true));
			conn.add(ConnectionDiagram::pt(-65,0));
			conn.add(new VssSymbol(-65, 0, CairoDrawing::DIRECTION::DOWN));
			conn.add(ConnectionDiagram::text(-65, -20, "MCLR Circuit"));

		}

		void draw_schmitt1() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Schmitt1_Out"]);
			conn.add(ConnectionDiagram::pt( -45, 0, true, false, true));
			conn.add(ConnectionDiagram::pt( -45, 0, true));
			conn.add(ConnectionDiagram::pt( -80, 0));
			conn.add(new BlockSymbol(-120, 0, 80, 30));
			conn.add(ConnectionDiagram::text(-145, 6, "MCLR Filter"));
			conn.add(ConnectionDiagram::pt( -160, 0, true));
			conn.add(ConnectionDiagram::pt( -255, 0));
			conn.add(ConnectionDiagram::text(-20, 35, "Schmitt Trigger\nInput buffer"));
		}

		void draw_schmitt2() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Schmitt2_Out"]);
			conn.add(ConnectionDiagram::pt(   0, 45, true));
			conn.add(ConnectionDiagram::pt(   0, 74));
			conn.add(ConnectionDiagram::pt( -50, 74));
		}

		void draw_rd_trisa() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_TRISA"]);
			conn.add(ConnectionDiagram::pt(0,   40, true));
			conn.add(ConnectionDiagram::pt(140, 40));
			conn.add(ConnectionDiagram::pt(140, 10));
			conn.add(ConnectionDiagram::text(0, 38, "RD TrisA"));
			ConnectionDiagram &in = dynamic_cast<ConnectionDiagram &>(*m_components["Tristate3.in"]);
			in.add(ConnectionDiagram::pt(0,  0, true));
			in.add(ConnectionDiagram::pt(20, 0));
			in.add(ConnectionDiagram::pt(20, 10));
			in.add(new VssSymbol(20,10, CairoDrawing::DIRECTION::RIGHT));
		}

		void draw_rd_porta() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_PORTA"]);
			conn.add(ConnectionDiagram::pt(0,   25, true));
			conn.add(ConnectionDiagram::pt(140, 25));
			conn.add(ConnectionDiagram::pt(140, 10));
			conn.add(ConnectionDiagram::pt(140, 25, true, true));
			conn.add(ConnectionDiagram::pt(200, 25));
			conn.add(ConnectionDiagram::text(0, 23, "RD PortA"));
		}

		void draw_inverter1_out() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Inverter1 out"]);
			conn.add(ConnectionDiagram::pt(  0,  0, true));
			conn.add(ConnectionDiagram::pt( 150,  0));
			conn.add(ConnectionDiagram::pt( 150,-40));
			conn.add(ConnectionDiagram::pt( 100,-40));
		}

		void draw_output_q() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Output.Q"]);
			conn.add(ConnectionDiagram::pt(  0,  24, true));
			conn.add(ConnectionDiagram::pt( -50, 24));
			conn.add(ConnectionDiagram::pt( -50, 80));
			conn.add(ConnectionDiagram::pt(-110, 80));
		}


		void on_wire_change(Wire *wire, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
			m_area->queue_draw();
		}

		PortA5(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_RA5"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{

			auto &p5 = dynamic_cast<SinglePortA_MCLR_RA5 &>(*(m_cpu.porta.RA[5]));
			auto &c = p5.components();
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Wire &PinWire = dynamic_cast<Wire &> (*(c["Pin Wire"]));
			Wire &MCLREWire = dynamic_cast<Wire &> (*(c["MCLRE Wire"]));
			Schmitt &Schmitt1 = dynamic_cast<Schmitt &> (*(c["Schmitt1"]));
			Schmitt &Schmitt2 = dynamic_cast<Schmitt &> (*(c["Schmitt2"]));
			Tristate &Tristate2 = dynamic_cast<Tristate &> (*(c["Tristate2"]));
			Tristate &Tristate3 = dynamic_cast<Tristate &> (*(c["Tristate3"]));
			Latch &OutputLatch = dynamic_cast<Latch &>(*(c["SR1"]));
			Inverter &Inverter1 = dynamic_cast<Inverter &> (*(c["Inverter1"]));
			AndGate &And1 = dynamic_cast<AndGate &> (*(c["And1"]));

			DeviceEvent<Wire>::subscribe<PortA5>(this, &PortA5::on_wire_change, &DataBus);

			m_components["Data Bus"]   = new WireDiagram( DataBus,   100.0,  40.0, m_area);
			m_components["Pin Wire"]   = new WireDiagram( PinWire,   0.0,  0.0, m_area);
			m_components["MCLRE Wire"]  = new WireDiagram( MCLREWire,  250.0,  100.0, m_area);
			m_components["Pin"]  = new PinDiagram(p5.pin(), 630, 200, 0, 1, m_area);
			m_components["Schmitt1"]  = new SchmittDiagram(Schmitt1, 440, 140, CairoDrawing::DIRECTION::LEFT, false, m_area);
			m_components["Schmitt1_Out"]  = new ConnectionDiagram(Schmitt1.rd(), 440, 140, m_area);
			m_components["Schmitt2"]  = new SchmittDiagram(Schmitt2, 480, 250, CairoDrawing::DIRECTION::DOWN, true, m_area);
			m_components["Schmitt2_Out"]  = new ConnectionDiagram(Schmitt2.rd(), 480, 250, m_area);
			m_components["And1.out"]  = new ConnectionDiagram(And1.rd(), 185, 130, m_area);
			m_components["Tristate2"]  = new TristateDiagram(Tristate2, false, 250, 380, m_area);
			m_components["Tristate3"]  = new TristateDiagram(Tristate3, false, 250, 310, m_area);
			m_components["Tristate3.in"] = new ConnectionDiagram(Tristate3.input(), 250, 310, m_area);
			m_components["Inverter1"]  = new InverterDiagram(Inverter1, 300, 405, CairoDrawing::DIRECTION::RIGHT, m_area);
			m_components["Output Latch"]= new LatchDiagram(OutputLatch, false, 360.0, 310.0, m_area);
			m_components["RD_TRISA"]  = new ConnectionDiagram(Tristate3.gate(), 100, 310, m_area);
			m_components["RD_PORTA"]  = new ConnectionDiagram(Tristate2.gate(), 100, 380, m_area);
			m_components["Inverter1 out"]  = new ConnectionDiagram(Inverter1.rd(), 330, 405, m_area);
			m_components["Output.Q"]= new ConnectionDiagram(OutputLatch.Q(), 360.0, 300.0, m_area);


			draw_data_bus();
			draw_pin_wire();
			draw_mclre_wire();
			draw_and1();
			draw_schmitt1();
			draw_schmitt2();
			draw_rd_trisa();
			draw_rd_porta();
			draw_inverter1_out();
			draw_output_q();
		}

	};
}
