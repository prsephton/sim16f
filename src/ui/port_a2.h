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

	class PortA2: public CairoDrawing {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);

			cr->move_to(360, 50);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Device RA2/AN2/Vref");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		void draw_data_bus() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Data Bus"]);
			wire.add(WireDiagram::pt(0,  23, true));
			wire.add(WireDiagram::pt(100, 23));
			wire.add(WireDiagram::pt(70,  23, true,true));
			wire.add(WireDiagram::pt(70, 340));
			wire.add(WireDiagram::pt(120,340));
			wire.add(WireDiagram::pt(70, 144, true,true));
			wire.add(WireDiagram::pt(100,144));
			wire.add(WireDiagram::pt(70, 270, true,true));
			wire.add(WireDiagram::pt(120,270));
			wire.add(WireDiagram::text(0, 21, "Data bus"));
		}

		void draw_tristate1_input() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Tristate1 input"]);
			conn.add(ConnectionDiagram::pt(70,23,true));
			conn.add(ConnectionDiagram::pt(120,23));
			conn.add(ConnectionDiagram::pt(120,110));
			conn.add(ConnectionDiagram::pt(200,110));

		}

		void draw_tristate1_gate() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["Tristate1 gate"]);
			conn.add(ConnectionDiagram::pt(70,145, true));
			conn.add(ConnectionDiagram::pt(100,145));
			conn.add(ConnectionDiagram::pt(100,155));
			conn.add(ConnectionDiagram::pt(215,155));
			conn.add(ConnectionDiagram::pt(215,120));
		}

		void draw_pin_wire() {
			WireDiagram &wire = dynamic_cast<WireDiagram &>(*m_components["Pin Wire"]);
			wire.add(WireDiagram::pt(430, 150, true));
			wire.add(WireDiagram::pt(530, 150));
			wire.add(WireDiagram::pt(495, 150, true, true));
			wire.add(WireDiagram::pt(495, 250));
			wire.add(WireDiagram::pt(495, 200, true, true));
			wire.add(WireDiagram::pt(540, 200));
			wire.add(WireDiagram::pt(540, 450));
			wire.add(WireDiagram::pt(100, 450));
			wire.add(WireDiagram::text(100,  448, "To Comparator 2"));
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


		void draw_cmcon() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["CMCON"]);
			conn.add(ConnectionDiagram::text(0, 0, "Analog\nInput Mode\n(CMCON Reg.)"));
			conn.add(ConnectionDiagram::pt( 34, 2, true));
			conn.add(ConnectionDiagram::pt(155, 2));
			conn.add(ConnectionDiagram::pt(155,25, false, false, true));
		}

		void draw_schmitt() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["SchmittOut"]);
			conn.add(ConnectionDiagram::pt(  0, 30, true));
			conn.add(ConnectionDiagram::pt(  0, 74));
			conn.add(ConnectionDiagram::pt(-60, 74));
		}

		void draw_trislatch_qc() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["TrisLatch Qc"]);
			conn.add(ConnectionDiagram::pt( 0,  0, true));
			conn.add(ConnectionDiagram::pt(50,  0));
			conn.add(ConnectionDiagram::pt(50,-84));
			conn.add(ConnectionDiagram::pt(20,-84));
		}

		void draw_rd_trisa() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["RD_TRISA"]);
			conn.add(ConnectionDiagram::pt(0,   40, true));
			conn.add(ConnectionDiagram::pt(140, 40));
			conn.add(ConnectionDiagram::pt(140, 10));
			conn.add(ConnectionDiagram::text(0, 38, "RD TrisA"));
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

		void draw_vref() {
			ConnectionDiagram &conn = dynamic_cast<ConnectionDiagram &>(*m_components["VRef.out"]);
			conn.add(ConnectionDiagram::pt(  260,  -40, true, true));
			conn.add(ConnectionDiagram::pt(  260,   0));
			conn.add(ConnectionDiagram::pt(  80,    0));
			conn.add(ConnectionDiagram::text(-30, 2, "VRef"));
			conn.add(ConnectionDiagram::text(-36, -18, "VROE"));
		}


		void on_wire_change(Wire *wire, const std::string &name, const std::vector<BYTE> &data) {
			Dispatcher().dispatcher(this, "refresh").emit();
		}

		void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
			Dispatcher().dispatcher(this, "refresh").emit();
		}


		virtual ~PortA2() {
			auto &p2 = dynamic_cast<SinglePortA_Analog &>(*(m_cpu.porta.RA[2]));
			auto &c = p2.components();
			Latch &DataLatch = dynamic_cast<Latch &>(*(c["Data Latch"]));
			Latch &TrisLatch = dynamic_cast<Latch &>(*(c["Tris Latch"]));
			Wire &DataBus = dynamic_cast<Wire &> (*(c["Data Bus"]));
			Tristate &Tristate1 = dynamic_cast<Tristate &> (*(c["Tristate1"]));
			DeviceEvent<Wire>::unsubscribe<PortA2>(this, &PortA2::on_wire_change, &DataBus);
			DeviceEvent<Connection>::unsubscribe<PortA2>(this, &PortA2::on_connection_change, &DataLatch.Q());
			DeviceEvent<Connection>::unsubscribe<PortA2>(this, &PortA2::on_connection_change, &TrisLatch.Q());
			DeviceEvent<Connection>::unsubscribe<PortA2>(this, &PortA2::on_connection_change, &Tristate1.rd());
		}

		PortA2(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_RA2"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade)
		{
			pix_extents(600, 520);

			auto &p2 = dynamic_cast<SinglePortA_Analog_RA2 &>(*(m_cpu.porta.RA[2]));
			auto &c = p2.components();
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
			Clamp &Clamp1 = dynamic_cast<Clamp &> (*(c["PinClamp"]));


			DeviceEvent<Wire>::subscribe<PortA2>(this, &PortA2::on_wire_change, &DataBus);
			DeviceEvent<Connection>::subscribe<PortA2>(this, &PortA2::on_connection_change, &DataLatch.Q());
			DeviceEvent<Connection>::subscribe<PortA2>(this, &PortA2::on_connection_change, &TrisLatch.Q());
			DeviceEvent<Connection>::subscribe<PortA2>(this, &PortA2::on_connection_change, &Tristate1.rd());

			m_components["Data Latch"] = new LatchDiagram(DataLatch, true, 200.0,  50.0, m_area);
			m_components["Tris Latch"] = new LatchDiagram(TrisLatch, true, 200.0, 170.0, m_area);
			m_components["Data Bus"]   = new WireDiagram( DataBus,   100.0,  40.0, m_area);
			m_components["Pin Wire"]   = new WireDiagram( PinWire,   0.0,  0.0, m_area);
			m_components["Tristate1"]  = new TristateDiagram( Tristate1, true, 400.0, 150.0, m_area);
			m_components["Tristate1 input"] = new ConnectionDiagram(DataLatch.Q(), 200, 40, m_area);
			m_components["Tristate1 gate"]  = new ConnectionDiagram(TrisLatch.Q(), 200, 40, m_area);
			m_components["Pin"]  = new PinDiagram(p2.pin(), 530, 150, 0, 1, m_area);
			m_components["Schmitt"]  = new SchmittDiagram(SchmittTrigger, 490, 250, CairoDrawing::DIRECTION::DOWN, true, m_area);
			m_components["WR_PORTA"]  = new ConnectionDiagram(DataLatch.Ck(), 100, 40, m_area);
			m_components["WR_TRISA"]  = new ConnectionDiagram(TrisLatch.Ck(), 100, 160, m_area);
			m_components["CMCON"]  = new ConnectionDiagram(SchmittTrigger.en(), 330, 220, m_area);
			m_components["SchmittOut"]  = new ConnectionDiagram(SchmittTrigger.rd(), 490, 250, m_area);
			m_components["Tristate2"]  = new TristateDiagram(Tristate2, false, 250, 380, m_area);
			m_components["Tristate3"]  = new TristateDiagram(Tristate3, false, 250, 310, m_area);
			m_components["Inverter1"]  = new InverterDiagram(Inverter1, 300, 405, CairoDrawing::DIRECTION::RIGHT, m_area);
			m_components["Output Latch"]= new LatchDiagram(OutputLatch, false, 360.0, 310.0, m_area);
			m_components["TrisLatch Qc"]  = new ConnectionDiagram(TrisLatch.Qc(), 250, 310, m_area);
			m_components["RD_TRISA"]  = new ConnectionDiagram(Tristate3.gate(), 100, 310, m_area);
			m_components["RD_PORTA"]  = new ConnectionDiagram(Tristate2.gate(), 100, 380, m_area);
			m_components["Inverter1 out"]  = new ConnectionDiagram(Inverter1.rd(), 330, 405, m_area);
			m_components["Output.Q"]= new ConnectionDiagram(OutputLatch.Q(), 360.0, 300.0, m_area);
			m_components["Clamp"]= new ClampDiagram(Clamp1, 515.0,150.0, m_area);
			m_components["VRef"]= new RelayDiagram(p2.VRef(), 280.0, 490.0, m_area);
			m_components["VRef.out"]= new ConnectionDiagram(p2.VRef().rd(), 280.0, 490.0, m_area);

			draw_data_bus();
			draw_tristate1_input();
			draw_tristate1_gate();
			draw_pin_wire();
			draw_wr_porta();
			draw_wr_trisa();
			draw_cmcon();
			draw_schmitt();
			draw_trislatch_qc();
			draw_rd_trisa();
			draw_rd_porta();
			draw_inverter1_out();
			draw_output_q();
			draw_vref();
		}

	};
}
