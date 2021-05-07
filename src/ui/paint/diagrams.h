#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include "cairo_drawing.h"
#include "../../devices/devices.h"

namespace app {

	class BufferDiagram:  public CairoDrawing {
		ABuffer &m_buffer;
		bool m_point_right;

		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BufferSymbol(m_x, m_y).draw(cr, m_point_right, false);
			return false;
		}

		BufferDiagram(ABuffer &a_buffer, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_buffer(a_buffer), m_point_right(point_right),  m_x(x), m_y(y)
		{}
	};

	class InverterDiagram:  public CairoDrawing {
		Inverter &m_inverter;
		double m_rotation;

		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BufferSymbol(m_x, m_y).draw(cr, true, m_rotation);
			return false;
		}

		InverterDiagram(Inverter &a_inverter, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_inverter(a_inverter), m_rotation(rotation),  m_x(x), m_y(y)
		{}
	};

	class PinDiagram:  public CairoDrawing {
		Connection &m_pin;
		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			PinSymbol(m_x, m_y).draw(cr);
			return false;
		}

		PinDiagram(Connection &a_pin, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_pin(a_pin), m_x(x), m_y(y)
		{}
	};

	class ClampDiagram:  public CairoDrawing {
		Clamp &m_clamp;
		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.2);
			black(cr);
			DiodeSymbol(0, -10).draw(cr, CairoDrawing::DIRECTION::UP);
			DiodeSymbol(0,  17).draw(cr, CairoDrawing::DIRECTION::UP);
			cr->move_to(0, -10); cr->line_to(0,10);
			cr->move_to(0, -25); cr->line_to(0,-17);
			cr->move_to(0,  25); cr->line_to(0, 17);
			cr->stroke();
			cr->arc(0, 0, 2, 0, 2*M_PI);
			cr->fill_preserve();
			cr->stroke();
			cr->move_to(-10, -25); cr->line_to(10, -25);
			cr->move_to(-10,  25); cr->line_to(10,  25);
			cr->stroke();

			cr->set_line_width(0.2);
			cr->move_to(-8, -28);
			cr->text_path("Vdd");
			cr->move_to(-8,  37);
			cr->text_path("Vss");
			cr->fill_preserve();
			cr->stroke();
			cr->restore();

			return false;
		}

		ClampDiagram(Clamp &a_clamp, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_clamp(a_clamp), m_x(x), m_y(y)
		{}
	};

	class SchmittDiagram:  public CairoDrawing {
		Schmitt &m_schmitt;
		int m_x;
		int m_y;
		double m_rotation;

	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			SchmittSymbol(m_x, m_y).draw(cr, m_rotation);
			return false;
		}

		SchmittDiagram(Schmitt &a_schmitt, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_schmitt(a_schmitt), m_x(x), m_y(y), m_rotation(rotation)
		{}
	};

	class TristateDiagram: public CairoDrawing {
		Tristate &m_tris;
		bool m_point_right;

		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.2);
			double rot = m_point_right?CairoDrawing::DIRECTION::RIGHT:CairoDrawing::DIRECTION::LEFT;
			BufferSymbol().draw(cr, m_tris.inverted(), rot);

			if (m_tris.gate_invert()) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(15, 11, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->stroke();
				cr->restore();
			}
			cr->restore();
			return false;
		}

		TristateDiagram(Tristate &a_tris, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_tris(a_tris), m_point_right(point_right),  m_x(x), m_y(y)
		{}

	};

	class LatchDiagram: public CairoDrawing {
		Latch &m_latch;
		bool m_point_right;

		int m_x;
		int m_y;
	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.5);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rectangle(0, 0, 70, 70);
			cr->set_source_rgba(0.9, 0.9, 0.95, 8.0);
			cr->fill_preserve();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->stroke();

			cr->set_line_width(0.2);
			if (m_point_right) {
				cr->move_to(10, 17); cr->text_path("D");
				cr->move_to(50, 17); cr->text_path("Q");

				cr->move_to(10, 59); cr->text_path("Ck");
				cr->move_to(50, 59); cr->text_path("Q");
				cr->move_to(50, 48); cr->line_to(60,48);
			} else {
				cr->move_to(10, 17); cr->text_path("Q");
				cr->move_to(50, 17); cr->text_path("D");

				cr->move_to(10, 59); cr->text_path("Q");
				cr->move_to(10, 48); cr->line_to(20,48);
				cr->move_to(50, 59); cr->text_path("Ck");
			}
			cr->fill_preserve(), cr->stroke();

			cr->set_source_rgba(0.5, 0.5, 0.9, 1.0);
			cr->set_line_width(0.8);

			cr->rectangle(0, 10, 5, 8);
			draw_indicator(cr, m_point_right?m_latch.D().signal():m_latch.Q().signal());
			cr->rectangle(0, 52, 5, 8);
			draw_indicator(cr, m_point_right?m_latch.Ck().signal():m_latch.Qc().signal());
			cr->rectangle(65,10, 5, 8);
			draw_indicator(cr, m_point_right?m_latch.Q().signal():m_latch.D().signal());
			cr->rectangle(65,52, 5, 8);
			draw_indicator(cr, m_point_right?m_latch.Qc().signal():m_latch.Ck().signal());

			cr->move_to(0, 82);
			cr->text_path(m_latch.name());
			cr->set_line_width(0.2);
			black(cr); cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		LatchDiagram(Latch &a_latch, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_latch(a_latch), m_point_right(point_right),  m_x(x), m_y(y)
		{}
	};

}
