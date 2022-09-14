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

	class ComparatorsDiagram: public CairoDrawing  {

		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		OpAmpSymbol *c1, *c2;
		Connection vin[4];
		Connection vref;
		Connection &vout0;
		Connection &vout1;
		BYTE mode=0;
		bool CIS=false;
		std::string cm;
		std::vector<std::string> describe;

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
			cr->move_to(260, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Comparators Diagram");
			cr->fill_preserve(); cr->stroke();
			cr->restore();

			cr->move_to(50, 50);
			cr->save();
			cr->scale(1.2, 1.2);
			cr->text_path(describe[mode]);
			cr->restore();

			cr->move_to(50, 75);
			cr->text_path("CM<2:0> = " + cm);

			c1->draw_symbol(cr, m_dev_origin);
			c2->draw_symbol(cr, m_dev_origin);

			return false;
		}

	  protected:

		void draw_c1_input(const Point &at) {
			int w = 80;
			int y1 = -w/2 + w / 4;
			int y2 = -w/2 + w * 3/4;

			SinglePortA_Analog *RA0 = dynamic_cast<SinglePortA_Analog *>(m_cpu.porta.RA[0].operator ->());
			SinglePortA_Analog_RA3 *RA3 = dynamic_cast<SinglePortA_Analog_RA3 *>(m_cpu.porta.RA[3].operator ->());
			const Point p0(at.x, at.y+y1);
			const Point p1(at.x, at.y+y2);
			const Point p3(at.x+80, at.y);

			ConnectionDiagram *c1Out = new ConnectionDiagram(vout0, p3.x,p3.y, m_area);
			m_components["c1Out"] = c1Out;

			c1Out->add(ConnectionDiagram::pt(0, 0).first());
			c1Out->add(ConnectionDiagram::pt(40, 0));
			c1Out->add(ConnectionDiagram::text(-40, 4, "C1"));
			c1Out->add(ConnectionDiagram::text(44, 4, (mode==0||mode==7||mode==5)?"Off (Read as '0')":"C1::Vout"));

			ConnectionDiagram *cd0 = new ConnectionDiagram(vin[0], p0.x,p0.y, m_area);
			ConnectionDiagram *cd1 = new ConnectionDiagram(vin[1], p1.x,p1.y, m_area);
			m_components["c1_in0"] = cd0;
			m_components["c1_in1"] = cd1;
			cd0->add(ConnectionDiagram::pt(0, 0).first());
			cd0->add(ConnectionDiagram::pt(-80, 0));
			if (mode==7) {
				cd0->add(ConnectionDiagram::pt(-80, 40).join());
				cd0->add(ConnectionDiagram::pt(-80, 120).join());
				cd0->add(ConnectionDiagram::pt(-80, 160).join());
				cd0->add(ConnectionDiagram::pt(-80, 200));
				cd0->add(new VssSymbol(-80, 200, 0));
			} else if (mode==5) {
				cd0->add(ConnectionDiagram::pt(-80, 40).join());
				cd0->add(ConnectionDiagram::pt(-80, 70));
				cd0->add(new VssSymbol(-80, 70, 0));
			}
			cd0->add(ConnectionDiagram::text(-40, -2, "Vin-"));
			cd1->add(ConnectionDiagram::pt(0, 0).first());
			cd1->add(ConnectionDiagram::pt(mode==2?-60:-80, 0));
			if (mode==1) {
				cd1->add(ConnectionDiagram::pt(-80, 120).join());
			} else if (mode==2) {
				cd1->add(ConnectionDiagram::pt(-60, 120).join());
				cd1->add(ConnectionDiagram::pt(-60, 160));
				cd1->add(ConnectionDiagram::pt( 20, 160));
				cd1->add(ConnectionDiagram::text( 24, 160, "From VREF Module"));
			} else if (mode==3) {
				cd1->add(ConnectionDiagram::pt(-80, 120).join());
			} else if (mode==6) {
				cd1->add(ConnectionDiagram::pt(-80, 120).join());
			}

			cd1->add(ConnectionDiagram::text(-40, -2, "Vin+"));

			double p0_y = p0.y;
			double p1_y = p1.y;
			if (mode == 1 || mode == 2) {
				p0_y -= 20;
				p1_y -= 20;
			}

			ConnectionDiagram *ra0 = new ConnectionDiagram(RA0->comparator(), p0.x-240, p0_y, m_area);
			ConnectionDiagram *ra3 = new ConnectionDiagram(RA3->comparator(), p1.x-240, p1_y, m_area);
			m_components["ra0"] = ra0;
			m_components["ra3"] = ra3;
			ra0->add(ConnectionDiagram::text(0, -2, "RA0/AN0"));
			ra0->add(ConnectionDiagram::pt(80, 0).first());

			if (mode==7||mode==5)
				ra0->add(ConnectionDiagram::pt(120, 0));
			else if (mode==2 || mode == 1) {
				ra0->add(ConnectionDiagram::pt(120, 0).invert());
				if (!CIS)  {
					ra0->add(ConnectionDiagram::text(100, 24, "CIS = 0"));
					ra0->add(ConnectionDiagram::pt(120, 2.5).first());
					ra0->add(ConnectionDiagram::pt(160, 20).join());
				}
			} else
				ra0->add(ConnectionDiagram::pt(160, 0));

			ra3->add(ConnectionDiagram::text(0, -2, "RA3/AN3/CMP1"));
			ra3->add(ConnectionDiagram::pt(80, 0).first());
			if (mode==7||mode==3||mode==5)
				ra3->add(ConnectionDiagram::pt(120, 0));
			else if (mode==2 || mode == 1) {
				ra3->add(ConnectionDiagram::pt(120, 0).invert());
				if (CIS) {
					ra3->add(ConnectionDiagram::text(100, -16, "CIS = 1"));
					ra3->add(ConnectionDiagram::pt(120, -2.5).first());
					ra3->add(ConnectionDiagram::pt(160, -20).join());
				}
			} else if (mode==6) {
				ra3->add(ConnectionDiagram::pt(120, 0));
				ra3->add(ConnectionDiagram::pt(120, 40));
				ra3->add(ConnectionDiagram::pt(350, 40));
				ra3->add(ConnectionDiagram::pt(350, -20).join());
			} else
				ra3->add(ConnectionDiagram::pt(160, 0));
		}

		void draw_c2_input(const Point &at) {
			int w = 80;
			int y1 = -w/2 + w / 4;
			int y2 = -w/2 + w * 3/4;

			SinglePortA_Analog *RA1 = dynamic_cast<SinglePortA_Analog *>(m_cpu.porta.RA[1].operator ->());
			SinglePortA_Analog_RA2 *RA2 = dynamic_cast<SinglePortA_Analog_RA2 *>(m_cpu.porta.RA[2].operator ->());
			SinglePortA_Analog_RA4 *RA4 = dynamic_cast<SinglePortA_Analog_RA4 *>(m_cpu.porta.RA[4].operator ->());

			const Point p0(at.x, at.y+y1);
			const Point p1(at.x, at.y+y2);
			const Point p3(at.x+80, at.y);

			ConnectionDiagram *c2Out = new ConnectionDiagram(vout1, p3.x,p3.y, m_area);
			m_components["c2Out"] = c2Out;

			c2Out->add(ConnectionDiagram::pt(0, 0).first());
			c2Out->add(ConnectionDiagram::pt(40, 0));
			c2Out->add(ConnectionDiagram::text(-40, 4, "C2"));
			c2Out->add(ConnectionDiagram::text(44, 4, (mode==0||mode==7)?"Off (Read as '0')":"C2::Vout"));

			ConnectionDiagram *cd0 = new ConnectionDiagram(vin[0], p0.x,p0.y, m_area);
			ConnectionDiagram *cd1 = new ConnectionDiagram(vin[1], p1.x,p1.y, m_area);
			m_components["c2_in0"] = cd0;
			m_components["c2_in1"] = cd1;

			cd0->add(ConnectionDiagram::pt(0, 0).first());
			cd0->add(ConnectionDiagram::pt(-80, 0));
			cd0->add(ConnectionDiagram::text(-40, -2, "Vin-"));

			cd1->add(ConnectionDiagram::pt(0, 0).first());
			cd1->add(ConnectionDiagram::pt(mode==2?-60:-80, 0));
			cd1->add(ConnectionDiagram::text(-40, -2, "Vin+"));

			double p0_y = p0.y;
			double p1_y = p1.y;
			if (mode == 2) {
				p0_y -= 20;
				p1_y -= 20;
			}

			ConnectionDiagram *ra1 = new ConnectionDiagram(RA1->comparator(), p0.x-240, p0_y, m_area);
			ConnectionDiagram *ra2 = new ConnectionDiagram(RA2->comparator(), p1.x-240, p1_y, m_area);
			ConnectionDiagram *ra4 = new ConnectionDiagram(RA4->Comparator_Out(), p1.x-240, p1_y+40, m_area);
			m_components["ra1"] = ra1;
			m_components["ra2"] = ra2;
			m_components["ra4"] = ra4;
			ra1->add(ConnectionDiagram::text(0, -2, "RA1/AN1"));
			ra1->add(ConnectionDiagram::pt(80, 0).first());
			if (mode==2) {
				ra1->add(ConnectionDiagram::pt(120, 0).invert());
				if (!CIS)  {
					ra1->add(ConnectionDiagram::text(100, 24, "CIS = 0"));
					ra1->add(ConnectionDiagram::pt(120, 2.5).first());
					ra1->add(ConnectionDiagram::pt(160, 20).join());
				}
			} else if (mode==7) {
				ra1->add(ConnectionDiagram::pt(120, 0));
			} else
				ra1->add(ConnectionDiagram::pt(160, 0));

			ra2->add(ConnectionDiagram::text(0, -2, "RA2/AN2/VREF"));
			ra2->add(ConnectionDiagram::pt(80, 0).first());
			if (mode==2) {
				ra2->add(ConnectionDiagram::pt(120, 0).invert());
				if (CIS)  {
					ra2->add(ConnectionDiagram::text(100, -16, "CIS = 1"));
					ra2->add(ConnectionDiagram::pt(120, -2.5).first());
					ra2->add(ConnectionDiagram::pt(160, -20).join());
				}
			} else if (mode==7) {
				ra2->add(ConnectionDiagram::pt(120, 0));
			} else
				ra2->add(ConnectionDiagram::pt(160, 0));

			if (mode==6) {
				ra4->add(ConnectionDiagram::text(0, 4, "RA4/TOCKI/CMP2"));
				ra4->add(ConnectionDiagram::text(105, -2, "Open Drain"));
				ra4->add(ConnectionDiagram::pt(90, 0).first());
				ra4->add(ConnectionDiagram::pt(350, 0));
				ra4->add(ConnectionDiagram::pt(350, -60).join());
			}
		}



	  public:
		void process_queue(){
			sleep_for_us(100);
		}

		void on_comparator_change(Comparator *c, const std::string &name, const std::vector<BYTE>&data) {
			vin[0].set_value(c->AN0(), false);
			vin[1].set_value(c->AN1(), false);
			vin[2].set_value(c->AN2(), false);
			vin[3].set_value(c->AN3(), false);
			vref.set_value(c->VREF(), false);
			mode = c->mode();
			CIS = c->CIS();
			cm = (mode & Flags::CMCON::CM2)?"1 ":"0 ";
			cm += (mode & Flags::CMCON::CM1)?"1 ":"0 ";
			cm += (mode & Flags::CMCON::CM0)?"1 ":"0 ";

			draw_c1_input(Point(300,150));
			draw_c2_input(Point(300,270));
			m_area->queue_draw();
		}

		ComparatorsDiagram(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_Comparators"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade), vout0(a_cpu.cmp0.rd(0)), vout1(a_cpu.cmp0.rd(1)),
			describe({
				"Comparators Reset (POR Default Value)",
				"Three Inputs Multiplexed to Two Comparators",
				"Four Inputs Multiplexed to Two Comparators",
				"Two Common Reference Comparators",
				"Two Independent Comparators",
				"One Independent Comparator",
				"Two Common Reference Comparators with Outputs",
				"Comparators Off"
			})
		{
//			pix_extents(740.0, 500.0);
			DeviceEvent<Comparator>::subscribe<ComparatorsDiagram>(this, &ComparatorsDiagram::on_comparator_change);
			c1 = new OpAmpSymbol(300,150);
			c2 = new OpAmpSymbol(300,270);
			mode = 0;
			cm = "0 0 0";

			draw_c1_input(Point(300,150));
			draw_c2_input(Point(300,270));
		}

		~ComparatorsDiagram() {
		}

	};


	class Comparators: public Component  {
		ComparatorsDiagram m_diagram;
		bool m_exiting;

		bool process_queue(){
			m_diagram.process_queue();
			return !m_exiting;
		}

		virtual void exiting(){
			m_exiting = true;
		}

	public:
		Comparators(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade): Component(),
		  m_diagram(a_cpu, a_refGlade), m_exiting(false) {
			Glib::signal_idle().connect( sigc::mem_fun(*this, &Comparators::process_queue) );
		};

	};

}				// namespace app
