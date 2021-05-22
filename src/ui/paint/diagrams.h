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

		BufferSymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));
			return false;
		}

		BufferDiagram(ABuffer &a_buffer, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_buffer(a_buffer), m_point_right(point_right),  m_x(x), m_y(y)
		{
			double rotation = 0;
			if (!m_point_right) rotation = CairoDrawing::DIRECTION::LEFT;
			m_symbol = BufferSymbol(m_x, m_y, rotation, false);
		}
	};

	class InverterDiagram:  public CairoDrawing {
		Inverter &m_inverter;
		double m_rotation;

		int m_x;
		int m_y;

		BufferSymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));
			return false;
		}

		InverterDiagram(Inverter &a_inverter, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_inverter(a_inverter), m_rotation(rotation),  m_x(x), m_y(y)
		{
			m_symbol = BufferSymbol(m_x, m_y, m_rotation, true);

		}
	};

	class PinDiagram:  public CairoDrawing {
		Connection &m_pin;
		int m_x;
		int m_y;

	  public:

		virtual bool on_motion(double x, double y) {
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			PinSymbol(m_x, m_y).draw_symbol(cr, Point(m_xofs, m_yofs));
			return false;
		}

		PinDiagram(Connection &a_pin, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_pin(a_pin), m_x(x), m_y(y)
		{

		}
	};

	class ClampDiagram:  public CairoDrawing {
		Clamp &m_clamp;
		int m_x;
		int m_y;

	  public:

		virtual bool on_motion(double x, double y) {
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.2);
			black(cr);
			DiodeSymbol(0, -10, CairoDrawing::DIRECTION::UP).draw_symbol(cr, Point(m_xofs, m_yofs));
			DiodeSymbol(0,  17, CairoDrawing::DIRECTION::UP).draw_symbol(cr, Point(m_xofs, m_yofs));
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
		bool m_dual;

		SchmittSymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));
			return false;
		}

		SchmittDiagram(Schmitt &a_schmitt, double x, double y, double rotation, bool dual, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_schmitt(a_schmitt), m_x(x), m_y(y), m_rotation(rotation), m_dual(dual)
		{
			m_symbol = SchmittSymbol(m_x, m_y, m_rotation, m_dual);
		}
	};

	class TristateDiagram: public CairoDrawing {
		Tristate &m_tris;
		bool m_point_right;

		int m_x;
		int m_y;

		BufferSymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.2);
			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));

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
		{
			double rot = m_point_right?CairoDrawing::DIRECTION::RIGHT:CairoDrawing::DIRECTION::LEFT;
			m_symbol = BufferSymbol(0,0,rot,m_tris.inverted());
		}

	};

	class RelayDiagram: public CairoDrawing {
		Relay &m_relay;

		int m_x;
		int m_y;

		RelaySymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);

			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));

			cr->restore();
			return false;
		}

		RelayDiagram(Relay &a_relay, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_relay(a_relay), m_x(x), m_y(y) {
			m_symbol = RelaySymbol(0,0,0,m_relay.sw().signal());
		}

	};

	class MuxDiagram: public CairoDrawing {
		Mux &m_mux;

		int m_x;
		int m_y;
		double m_rotation;

		MuxSymbol m_symbol;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);

			m_symbol.draw_symbol(cr, Point(m_xofs, m_yofs));

			cr->restore();
			return false;
		}

		MuxDiagram(Mux &a_mux, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_mux(a_mux), m_x(x), m_y(y), m_rotation(rotation) {
			m_symbol = MuxSymbol(0, 0, m_rotation, m_mux.no_selects(), m_mux.no_inputs());
		}

	};

	class LatchDiagram: public CairoDrawing {
		Latch &m_latch;
		bool m_point_right;

		int m_x;
		int m_y;
		Point m_size;
		BlockSymbol m_basic;

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_basic.bounding_rect();
			bool selected = inside(x, y, r.x, r.y, r.w, r.h);
			if (selected != m_basic.selected()) {
				m_basic.selected() = selected;
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_basic.draw_symbol(cr, Point(m_xofs, m_yofs));

			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.5);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->set_line_width(0.2);
			if (m_point_right) {
				cr->move_to(10, 17); cr->text_path("D");
				cr->move_to(50, 17); cr->text_path("Q");

				cr->move_to(10, 59); cr->text_path("Ck");
				cr->move_to(50, 59); cr->text_path("Q");
			} else {
				cr->move_to(10, 17); cr->text_path("Q");
				cr->move_to(50, 17); cr->text_path("D");

				cr->move_to(10, 59); cr->text_path("Q");
				cr->move_to(50, 59); cr->text_path("Ck");
			}
			cr->fill_preserve(), cr->stroke();
			cr->set_line_width(0.8);
			if (m_point_right) {
				cr->move_to(50, 48); cr->line_to(60,48);
			} else {
				cr->move_to(10, 48); cr->line_to(20,48);
			}
			cr->stroke();

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
			CairoDrawing(a_area), m_latch(a_latch), m_point_right(point_right),  m_x(x), m_y(y), m_size(70, 70)
		{
			m_basic = BlockSymbol(m_x+m_size.x/2, m_y+m_size.y/2, m_size.x, m_size.y);
		}
	};

}
