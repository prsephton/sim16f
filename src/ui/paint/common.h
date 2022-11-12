#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <cmath>
#include "cairo_drawing.h"
#include "properties.h"
#include "../../devices/devices.h"

namespace app {

	class Symbol : virtual public Configurable, public prop::Rotation, public prop::Scale {

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;

	  protected:
		std::string m_name;
		double m_x;
		double m_y;
		bool   m_selected;
		Rect   m_rect;
		Point  m_ofs;
		std::vector<Point> m_hotspots;
		bool   m_show_name = false;

	  public:

		virtual void outline(const Cairo::RefPtr<Cairo::Context>& cr) {
			Rect r = bounding_rect();
			cr->save();
			cr->set_identity_matrix();
			cr->translate(m_ofs.x, m_ofs.y);
			cr->set_line_width(2.0);
			cr->set_source_rgba(0.5, 0.6, 0.8, 0.10);
			cr->set_operator(Cairo::OPERATOR_XOR);
			cr->rectangle(r.x,r.y,r.w,r.h);
			cr->fill_preserve();
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.85);
//			cr->set_operator(Cairo::OPERATOR_OVER);
			cr->set_dash(std::vector<double>{2,2}, 1);
			cr->stroke();
			cr->restore();
		}

		void rotate(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->rotate(get_rotation());
		}
		void scale(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->scale(get_scale(), get_scale());
		}

		void draw_label(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, const std::string &text) {
			if (text.length()) {

				cr->save();
				cr->translate(m_x, m_y);
				scale(cr);
				cr->save();
				rotate(cr);
				Cairo::TextExtents extents;
				cr->get_text_extents(text, extents);

				switch (((int)round(2*(get_rotation() + 2*M_PI)/M_PI)) % 4) {
				  case WHATS_AT::EAST:
					  cr->move_to(x, y);
					  break;
				  case WHATS_AT::SOUTH:
					  cr->move_to(x + extents.height, y);
					  break;
				  case WHATS_AT::WEST:
					  cr->move_to(x+extents.width, y-extents.height);
					  break;
				  case WHATS_AT::NORTH:
					  cr->move_to(x, y-extents.width);
					  break;
				}
				cr->restore();
				CairoDrawingBase::black(cr);
				cr->show_text(text);
				cr->restore();
			}
		}

		void show_name(bool show) { m_show_name = show; }
		bool showing_name() { return m_show_name; }

		void draw_value(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, const std::string &text) {
			if (m_show_name)
				draw_label(cr, x, y, text);
		}

		void draw_label(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y) {
			if (m_show_name)
				draw_label(cr, x, y, name());
		}

		void draw_symbol(const Cairo::RefPtr<Cairo::Context>& cr, Point a_ofs) {
			m_ofs = a_ofs;
			cr->save();
			draw(cr);
			cr->restore();

			if (selected()) outline(cr);
		}

		const Rect bounding_rect() const { return m_rect; }
		void bounding_rect(const Cairo::RefPtr<Cairo::Context>& cr, Rect r) {
			cr->user_to_device(r.x, r.y);
			cr->user_to_device_distance(r.w, r.h);
			if (r.w < 0) { r.x += r.w; r.w = fabs(r.w); }
			if (r.h < 0) { r.y += r.h; r.h = fabs(r.h); }
			m_rect = Rect(r.x - m_ofs.x, r.y-m_ofs.y, r.w, r.h);
		}

		const Point &hotspot(size_t id) const {
			if (id >= m_hotspots.size())
				return m_hotspots[0];
			return m_hotspots[id];
		}
		void hotspot(const Cairo::RefPtr<Cairo::Context>& cr, size_t id, Point p) {
			if (m_show_name) {
				cr->save();
				cr->begin_new_sub_path();
				cr->set_line_width(0.4);
				cr->arc(p.x, p.y, 2, 0, 2*M_PI);
				CairoDrawing::brightred(cr);
				cr->fill_preserve();
				CairoDrawing::black(cr);
				cr->stroke();
				cr->restore();
			}

			cr->user_to_device(p.x, p.y);
			p.x -= m_ofs.x; p.y -= m_ofs.y;
			if (id >= m_hotspots.size())
				m_hotspots.resize((id/10+1) * 10);
			m_hotspots[id] = p;
		}
		virtual const Point *hotspot_at(const WHATS_AT &what) const {return NULL;}


		bool inside(const Rect r, Point p) {
			return r.inside(p);
		}

		bool inside(Point p) {
			return m_rect.inside(p);
		}
		void diagram_pos(const Point &a_pos) {
			m_ofs = a_pos;
		}
		Point origin() const { return Point(m_x, m_y); }
		void move(const Point &a_destination) {
			m_x = a_destination.x;
			m_y = a_destination.y;
		}

		virtual WHATS_AT location(Point p) {
			if (inside(p))
				return WHATS_AT(this, WHATS_AT::SYMBOL, 0);
			return WHATS_AT(this, WHATS_AT::NOTHING, 0);
		}

		const bool selected() const { return m_selected; }
		bool &selected() { return m_selected; }

		const std::string &name() const { return m_name; }
		void name(const std::string &name) { m_name = name; }

		// context menu integration
		virtual bool needs_name(std::string &a_name){ a_name = name(); return true; }
		virtual void set_name(const std::string &a_name){ name(a_name); }

		Symbol(double x=0, double y=0, double rotation=0, double scale=1.0):
			m_name(""), m_x(x), m_y(y), m_selected(false),
			m_rect(0,0,0,0), m_ofs(0,0) {
			set_rotation(rotation); set_scale(scale);
			m_hotspots.resize(10);
		}
		virtual ~Symbol() {}
	};

	class BusSymbol: public Symbol {
		Point p1, p2;
		double width, length;
		double rotation;
		int bits;

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double lw = width / 6.0;

			cr->save();
			cr->translate(p1.x, p1.y);
			cr->rotate(rotation);

//			bounding_rect(cr, Rect(0, 0, length, width));

			cr->move_to(lw, lw);
			cr->line_to(length-lw, lw);

			cr->move_to(lw, width-lw);
			cr->line_to(length-lw, width-lw);
			cr->set_line_width(lw);
			if (p1.arrow) {
				cr->move_to(lw, -lw);
				cr->line_to(-lw*3, lw*3);
				cr->line_to(lw, width+lw);
			}
			if (p2.arrow) {
				cr->move_to(length-lw, -lw);
				cr->line_to(length+lw*3, lw*3);
				cr->line_to(length-lw, width+lw);
			}
			cr->stroke();

			cr->save();
			cr->set_line_width(lw*3);
			cr->set_source_rgba(1,1,1,1);
			cr->move_to(0,lw*3); cr->line_to(length, lw*3);
			cr->stroke();
			cr->restore();

			if (p1.term)  {
				cr->move_to(lw, lw);
				cr->line_to(lw, width-lw);
				cr->stroke();
			}
			if (p2.term)  {
				cr->move_to(length-lw, lw);
				cr->line_to(length-lw, width-lw);
				cr->stroke();
			}

			if (bits) {
				cr->set_line_width(0.7);
				cr->move_to(length/2-5, -5);
				cr->line_to(length/2+5, width+5);
				cr->stroke();
				cr->move_to(length/2, -10);
				cr->rotate(-rotation);
				cr->set_font_size(8);
				cr->text_path(int_to_string(bits));
				cr->fill();
				cr->stroke();
			}
			cr->restore();
		}

	  public:

		BusSymbol(Point a_p1, Point a_p2, double w=5.0, int a_bits=0): p1(a_p1), p2(a_p2), width(w), bits(a_bits) {
			double dy = a_p2.y-p1.y;
			double dx = a_p2.x-p1.x;
			rotation = atan2(dy, dx);
//			printf("Rotation is %.2f\n", rotation);
			length = sqrt(dy*dy + dx*dx);
			name("Bus");
		}
	};

	class PinSymbol: public Symbol {
		bool m_indeterminate = false;
		bool m_signal = false;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 0, WHATS_AT::WEST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::IN_OUT, 0))
				return &hotspot(0);
			return NULL;
		}

		void signal( bool a_signal) { m_signal = a_signal; }
		void indeterminate( bool a_indeterminate) { m_indeterminate = a_indeterminate; }

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 25, 0);
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -10, 20, 20));
			hotspot(cr, 0, Point(0, 0));
			if (m_indeterminate)
				CairoDrawing::indeterminate(cr);
			else if (m_signal)
				CairoDrawing::green(cr);
			else CairoDrawing::gray(cr);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rectangle(0,-10,20,20);
			cr->fill_preserve();
			CairoDrawing::black(cr);
			cr->stroke();
			cr->move_to( 0,-10);
			cr->line_to(20, 10);
			cr->move_to(20,-10);
			cr->line_to( 0, 10);
			cr->stroke();
			cr->restore();
		}

		PinSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("P");
		}
	};

	class VddSymbol: public Symbol, public prop::Voltage {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::SOUTH);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, -8, -24);
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);

			bounding_rect(cr, Rect(-10, -20, 20, 20));
			hotspot(cr, 0, Point(0, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to( 0, -20);
			cr->line_to( 0,   0);
			cr->move_to(-10,-20);
			cr->line_to( 10,-20);
			cr->stroke();
			cr->restore();
		}

		VddSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("V");
		}
	};


	class VssSymbol: public Symbol {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::NORTH);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, what.id))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(-5, 0, 10, 10));
			hotspot(cr, 0, Point(0, 0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_BUTT);
			cr->move_to(-5, 0); cr->line_to(0, 10); cr->line_to(5, 0);
			cr->close_path();
			cr->fill_preserve(); cr->stroke();
			cr->restore();
		}
		VssSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("Ground");
		}
	};


	class ConnectionSymbol: public Symbol {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 1, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::IN_OUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::IN_OUT, 1))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 3, -5);
			cr->save();
			cr->translate(m_x, m_y);

			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -5, 30, 10));
			hotspot(cr, 0, Point(0, 0));
			hotspot(cr, 1, Point(30, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to( 0, 0);
			cr->line_to(30, 0);
			cr->move_to( 5,-5);
			cr->line_to( 0, 0);
			cr->line_to( 5, 5);
			cr->move_to(25,-5);
			cr->line_to(30, 0);
			cr->line_to(25, 5);
			cr->stroke();

			cr->restore();
		}

		ConnectionSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("IO");
		}
	};


	class InputSymbol: public Symbol {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::WEST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::OUTPUT, what.id))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 3, -5);
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -5, 30, 10));
			hotspot(cr, 0, Point(0, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to( 0, 0);
			cr->line_to(25, 0);
			cr->move_to(30,-5);
			cr->line_to(25, 0);
			cr->line_to(30, 5);
			cr->stroke();

			cr->restore();
		}

		InputSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("In");
		}
	};


	class OutputSymbol: public Symbol {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, what.id))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 3, -5);
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -5, 30, 10));
			hotspot(cr, 0, Point(0, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to( 0, 0);
			cr->line_to(30, 0);
			cr->move_to(25,-5);
			cr->line_to(30, 0);
			cr->line_to(25, 5);
			cr->stroke();

			cr->restore();
		}

		OutputSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("Out");
		}
	};

	class ResistorSymbol: public Symbol, public prop::Resistance {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 1, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 1))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 0, -8);
			draw_value(cr, 35, -8, unit_text(resistance(), "Ω"));
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);

			bounding_rect(cr, Rect(0, -5, 45, 10));
			hotspot(cr, 0, Point( 0, 0));
			hotspot(cr, 1, Point(45, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_BUTT);
			cr->move_to( 0, 0);
			cr->line_to( 5, 0);
			cr->line_to(10,-5);
			cr->line_to(15, 5);
			cr->line_to(20,-5);
			cr->line_to(25, 5);
			cr->line_to(30,-5);
			cr->line_to(35, 5);
			cr->line_to(40, 0);
			cr->line_to(45, 0);

			cr->stroke();
			cr->restore();
		}
		ResistorSymbol(double x=0, double y=0, double rotation=0, double scale=1): Symbol(x, y, rotation, scale) {
			name("R");
		}
	};

	class CapacitorSymbol: public Symbol, public prop::Capacitance {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 0, WHATS_AT::NORTH);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 1, WHATS_AT::SOUTH);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::IN_OUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::IN_OUT, 1))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 10, 4);
			draw_value(cr, 10, 44, unit_text(capacitance(), "F"));
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);

			bounding_rect(cr, Rect(-10, 0, 20, 30));
			hotspot(cr, 0, Point(0, 0));
			hotspot(cr, 1, Point(0,30));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_BUTT);
			cr->move_to( 0, 0);
			cr->line_to( 0, 13);
			cr->move_to( 0, 17);
			cr->line_to( 0, 30);
			cr->stroke();
			cr->set_line_width(2.4);
			cr->move_to(-10, 13);
			cr->line_to( 10, 13);
			cr->move_to(-10, 17);
			cr->line_to( 10, 17);
			cr->stroke();
			cr->restore();
		}
		CapacitorSymbol(double x=0, double y=0, double rotation=0, double scale=1): Symbol(x, y, rotation, scale) {
			name("C");
		}
	};


	class InductorSymbol: public Symbol, public prop::Inductance {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 1, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::IN_OUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::IN_OUT, 1))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 0, -8);
			draw_value(cr, 35, -8, unit_text(inductance(), "H"));
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);

			bounding_rect(cr, Rect(0, -5, 45, 10));
			hotspot(cr, 0, Point(0, 0));
			hotspot(cr, 1, Point(45,0));

			cr->rectangle(0, -6, 45, 9.4);
			cr->clip();

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_SQUARE);
			cr->move_to( 0, 0);
			cr->line_to(10, 0);

			cr->arc(15, 0, 5, M_PI, M_PI/4);
			cr->arc(22.5, 0, 5, M_PI*3/4, M_PI/4);
			cr->arc(30, 0, 5, M_PI*3/4, 0);

			cr->move_to(35, 0);
			cr->line_to(45, 0);
			cr->stroke();
			cr->restore();
		}
		InductorSymbol(double x=0, double y=0, double rotation=0, double scale=1): Symbol(x, y, rotation, scale) {
			name("L");
		}
	};


	class PullUpSymbol: public Symbol {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::SOUTH);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(-10, -20, 20, 40));
			hotspot(cr, 0, Point(0, 20));
			VddSymbol l_vdd(0,0); l_vdd.name(name());
			l_vdd.draw(cr);
			ResistorSymbol(0,0, M_PI/2, 0.5).draw(cr);

			cr->restore();
		}

		PullUpSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {
			name("V");
		}
	};


	class DiodeSymbol: public Symbol  {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 0, -10);
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			bounding_rect(cr, Rect(0, -7, 10, 14));
			hotspot(cr, 0, Point(0, -7));
			hotspot(cr, 1, Point(0, 7));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -6); cr->line_to(0,  6); cr->line_to(6,  0); cr->close_path(); cr->stroke();
			cr->move_to(7, -7); cr->line_to(7,  7); cr->stroke();
			cr->move_to(0, 0); cr->line_to(-5, 0); cr->stroke();
			cr->move_to(7, 0); cr->line_to(10, 0); cr->stroke();
			cr->restore();
		}
		DiodeSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {
			name("D");
		}
	};


	class FETSymbol: public Symbol {
		bool m_nType;
		bool m_with_vss;
		bool m_with_vdd;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::NORTH);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::SOUTH);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::WEST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::GATE, 0))
				return &hotspot(2);
			return NULL;
		}


		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 0, -8);
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			bounding_rect(cr, Rect(0, -20, 20, 40));
			hotspot(cr, 0, Point(20, -20));
			hotspot(cr, 1, Point(20,  20));
			hotspot(cr, 2, Point(0,  0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, 0); cr->line_to(5,0); cr->stroke();
			cr->move_to(5,-8); cr->line_to(5,8); cr->stroke();
			if (m_with_vdd) {
				cr->save();
				cr->move_to(10,-20);
				cr->line_to(30,-20);
				cr->move_to(10,-22);
				cr->text_path("Vdd");
				cr->set_line_width(0.7);
				cr->fill_preserve(); cr->stroke();
				cr->restore();
			}

			cr->move_to(20,-20); cr->line_to(20,-8); cr->line_to(9,-8);
			cr->line_to(9,8); cr->line_to(20,8); cr->line_to(20,20);
			cr->stroke();
			cr->move_to(12, 4);
			cr->text_path(m_nType?"N":"P");
			cr->save();
			cr->scale(0.8, 0.8);
			cr->fill_preserve(); cr->stroke();
			cr->stroke();
			cr->restore();
			if (m_with_vss) VssSymbol(20,20).draw(cr);
			cr->restore();
		}
		FETSymbol(double x=0, double y=0, double rotation=0, bool nType=true, bool with_vss=false, bool with_vdd=false):
			Symbol(x, y, rotation), m_nType(nType), m_with_vss(with_vss), m_with_vdd(with_vdd) {
			name("T");
		}
	};

	class BufferSymbol: public Symbol, public prop::Inverted  {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 15, -15);
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			bounding_rect(cr, Rect(0, -15, 30, 30));
			hotspot(cr, 0, Point(0, 0));
			hotspot(cr, 1, Point(30, 0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -15); cr->line_to(0, 15);
			cr->line_to(30, 0); cr->close_path();
			if (inverted()) {
				cr->stroke();
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(30, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}
			cr->stroke();
			cr->restore();
		}

		BufferSymbol(double x=0, double y=0, double rotation=0, bool a_inverted=false):
			Symbol(x, y, rotation) {
			inverted(a_inverted);
			name("U");
		}
	};


	class OpAmpSymbol: public Symbol {
		bool m_plus_on_top;

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			int w = 80;
			int y1 = -w/2 + w / 4;
			int y2 = -w/2 + w * 3/4;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			bounding_rect(cr, Rect(0, -w/2, w, w));

			hotspot(cr, 0, Point(0, y1));
			hotspot(cr, 1, Point(0, y2));
			hotspot(cr, 2, Point(w, 0));

			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -w/2); cr->line_to(0, w/2);
			cr->line_to(w, 0); cr->close_path();
			cr->stroke();
			cr->select_font_face("monospace", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
			cr->move_to(4, y1 + 4);
			cr->text_path(m_plus_on_top?"+":"-");
			cr->move_to(4, y2 + 4);
			cr->text_path(m_plus_on_top?"-":"+");

			cr->fill_preserve(); cr->stroke();

			cr->move_to(-3, y1);
			cr->line_to(0, y1);
			cr->move_to(-3, y2);
			cr->line_to(0, y2);
			cr->stroke();

			cr->restore();
		}

	public:
		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 1, WHATS_AT::WEST);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::INPUT, 1))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(2);
			return NULL;
		}

		OpAmpSymbol(double x=0, double y=0, double rotation=0, bool plus_on_top=false):
			Symbol(x, y, rotation), m_plus_on_top(plus_on_top){
			name("U");
		}

	};

	class AndSymbol: public Symbol, public prop::Inverted, public prop::Inputs {
	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<inputs(); ++n)
				if (hotspot(n+1).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n, WHATS_AT::WEST);
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<inputs(); ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(n+1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 8, -17);

			double h = 30;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			cr->scale(get_scale(), get_scale());
			cr->set_line_width(1.2);
			bounding_rect(cr, Rect(0, -h/2, h, h));
			for (int n=1; n<=inputs(); ++n) {
				double y = -h/2+n*h/(inputs()+1);
				hotspot(cr, n, Point(0, y));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, 0, Point(h, 0));  // output is hotspot with id=0

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->move_to(h/2,-h/2); cr->line_to(0,-h/2); cr->line_to(0,h/2); cr->line_to(h/2,h/2);
			cr->stroke();
			cr->arc(h/2,0, h/2, -M_PI/2, M_PI/2);
			cr->stroke();


			if (inverted()) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}

			cr->restore();
		}
		AndSymbol(int a_inputs=2, double x=0, double y=0, double rotation=0, bool a_inverted=false):
			Symbol(x, y, rotation) {
			inverted(a_inverted); inputs(a_inputs);
			name("U");
		}
	};

	class OrSymbol: public Symbol, public prop::Inverted, public prop::Inputs, public prop::Xor  {
	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<inputs(); ++n)
				if (hotspot(n+1).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n, WHATS_AT::WEST);
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<inputs(); ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(n+1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 8, -17);
			double h   = 30.0;
			double ofs = h/8;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			cr->scale(get_scale(), get_scale());
			cr->set_line_width(1.2);
			bounding_rect(cr, Rect(0, -h/2, h, h));
			for (int n=1; n<=inputs(); ++n) {
				double y = -h/2+n*h/(inputs()+1);
				hotspot(cr, n, Point(0, -h/2+n*h/(inputs()+1)));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, 0, Point(h, 0));  // output is hotspot with id=0

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->save();
			cr->rectangle(0, -h/2, h-ofs, h);
			cr->clip();
			cr->scale(1/4.0, 1.0);
			cr->arc(-ofs, 0, h/2, -M_PI/2, M_PI/2);
			cr->restore();
			cr->stroke();

			if (is_xor()) {
				cr->save();
				cr->scale(1/4.0, 1.0);
				cr->arc(ofs, 0, h/2, -M_PI/2, M_PI/2);
				cr->restore();
				cr->stroke();
			}

			cr->move_to(0,-h/2); cr->line_to(h-ofs*6,-h/2);
			cr->move_to(0, h/2); cr->line_to(h-ofs*6, h/2);
			cr->stroke();

			cr->save();
			cr->scale(1.5, 1);
			cr->arc(h/5, 0.0, h/2, -M_PI/2, M_PI/2);
			cr->restore();
			cr->stroke();

			if (inverted()) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}
			cr->restore();
		}
		OrSymbol(int a_inputs=2, double x=0, double y=0, double rotation=0, bool a_inverted=false, bool a_is_xor=false):
			Symbol(x, y, rotation){
			inverted(a_inverted); set_xor(a_is_xor); inputs(a_inputs);
			name("U");
		}
	};


	class TristateSymbol: public BufferSymbol, public prop::Gate_Inverted {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::SOUTH);
			if (hotspot(3).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 1, WHATS_AT::NORTH);

			return BufferSymbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
					return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
					return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::GATE, 0))
					return &hotspot(2);
			if (what.match((void *)this, WHATS_AT::GATE, 1))
					return &hotspot(3);
			return BufferSymbol::hotspot_at(what);
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BufferSymbol::draw(cr);
			cr->save();

			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());

			hotspot(cr, 2, Point(15,  7.5));
			hotspot(cr, 3, Point(15, -7.5));

			cr->set_line_width(1.2);
			if (gate_inverted()) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(15, 11, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->stroke();
				cr->restore();
			}

			cr->restore();
		}

		TristateSymbol(double x=0, double y=0, double rotation=0, bool inverted=false,  bool a_gate_inverted=false):
			BufferSymbol(x, y, rotation, inverted) {
			gate_inverted(a_gate_inverted);
			name("U");
		}

	};

	class LatchSymbol: public Symbol {
		bool m_point_right;
		bool D, Ck, Q;
		bool m_clocked;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			else if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::WEST);
			else if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			else if (hotspot(3).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 1, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			else if (what.match((void *)this, WHATS_AT::GATE, 0))
				return &hotspot(1);
			else if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(2);
			else if (what.match((void *)this, WHATS_AT::OUTPUT, 1))
				return &hotspot(3);
			return NULL;
		}

		void signals(bool a_D, bool a_Ck, bool a_Q) {
			D = a_D; Ck = a_Ck; Q = a_Q;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {

			bounding_rect(cr, Rect(0,0,70,70));
			if (m_point_right) {
				hotspot(cr, 0, Point(0, 14));
				hotspot(cr, 1, Point(0, 56));
				hotspot(cr, 2, Point(70, 14));
				hotspot(cr, 3, Point(70, 56));
			} else {
				hotspot(cr, 0, Point(70, 14));
				hotspot(cr, 1, Point(70, 56));
				hotspot(cr, 2, Point(0, 14));
				hotspot(cr, 3, Point(0, 56));
			}

			cr->set_line_width(1.5);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->set_line_width(0.2);
			if (m_point_right) {
				cr->move_to(10, 17); cr->text_path(m_clocked?"D":"S");
				cr->move_to(50, 17); cr->text_path("Q");

				cr->move_to(10, 59); cr->text_path(m_clocked?"Ck":"R");
				cr->move_to(50, 59); cr->text_path("Q");
			} else {
				cr->move_to(10, 17); cr->text_path("Q");
				cr->move_to(50, 17); cr->text_path(m_clocked?"D":"S");

				cr->move_to(10, 59); cr->text_path("Q");
				cr->move_to(50, 59); cr->text_path(m_clocked?"Ck":"R");
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
			CairoDrawing::draw_indicator(cr, m_point_right?D:Q);
			if (not m_point_right or not m_clocked) {
				cr->rectangle(0, 52, 5, 8);
			} else {
				cr->move_to(0, 50); cr->line_to(7,56); cr->line_to(0, 62); cr->close_path();
			}
			CairoDrawing::draw_indicator(cr, m_point_right?Ck:(!Q));
			cr->rectangle(65, 10, 5, 8);
			CairoDrawing::draw_indicator(cr, m_point_right?Q:D);
			if (m_point_right or not m_clocked) {
				cr->rectangle(65,52, 5, 8);
			} else {
				cr->move_to(70, 50); cr->line_to(63,56); cr->line_to(70, 62); cr->close_path();
			}
			CairoDrawing::draw_indicator(cr, m_point_right?(!Q):Ck);
		}

		void clocked(bool a_clocked) { m_clocked = a_clocked; }  // SR vs D flip-flop
		LatchSymbol(double x=0, double y=0, double rotation=0, bool backward=false, bool clocked=true):
			Symbol(x, y, rotation), m_point_right(!backward), D(false), Ck(false), Q(false), m_clocked(clocked) {
			name("U");
		}
	};


	class MuxSymbol: public Symbol, public prop::Inputs  {
		int m_gates;
		int m_ninputs;
		bool m_forward;
		int m_flipped = 1;
		double log2 = log(2);

	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<m_gates; ++n)
				if (hotspot(n).close_to(p))
					return WHATS_AT(this, WHATS_AT::GATE, n, WHATS_AT::SOUTH);
			for (int n=0; n<m_ninputs; ++n)
				if (hotspot(m_gates+n).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n, WHATS_AT::WEST);
			if (hotspot(m_gates+m_ninputs).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<m_gates; ++n)
				if (what.match((void *)this, WHATS_AT::GATE, n))
					return &hotspot(n);
			for (int n=0; n<m_ninputs; ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(m_gates+n);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(m_ninputs+m_gates);
			return NULL;
		}

		int gate_count() {   // log_2 (inputs)
			m_gates = (int)rint(log(m_ninputs) / log2);
			m_ninputs = (int)(pow(2, m_gates));
			return m_gates;
		}

		virtual void set_inputs(int a_inputs){
			inputs(a_inputs);
			m_ninputs = a_inputs;
			gate_count();    // recalculate counts for valid numbers
			inputs(m_ninputs);
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			cr->scale(get_scale(), get_scale());
			cr->set_line_width(1.2/get_scale());

			int cw = 5, ch = 14;
			int width = cw * (m_gates+1);
			int height0 = ch * (m_ninputs+1) + width*2;
			int height1 = ch * (m_ninputs+1);
			bounding_rect(cr, Rect(0, -height0/2, width, height0));

			for (int n=1; n<=m_gates; ++n) {
				double x = width*n/(m_gates+1);
				double y = m_flipped * (x - height0/2);
				hotspot(cr, n-1, Point(x,  y));
				cr->move_to(x, y);
				cr->line_to(x, y - m_flipped * (x + 3.5));
				cr->stroke();
			}
			for (int n=1; n<=m_ninputs; ++n) {
				double y=-height0/2+height0*n/(m_ninputs+1);
				hotspot(cr, m_gates+n-1, Point(0, y));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, m_gates+m_ninputs, Point(width, 0));

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -height0/2); cr->line_to(0, height0/2);
			cr->line_to(width,  height1/2);
			cr->line_to(width, -height1/2);
			cr->close_path();
			cr->stroke();

			cr->set_line_width(0.2);
			double h = (height0) / (m_ninputs+1);
			cr->save();
			cr->rotate(-get_rotation());
			cr->scale(1/get_scale(), 1/get_scale());
			cr->set_font_size(8);
			for (int r=0; r < m_ninputs; ++r) {
				std::string txt = int_to_hex(m_forward?r:(m_ninputs-r-1), "", "");
				draw_label(cr, width/2-3, height0/2.0 - (r+1) * h, txt);
			}
			cr->restore();
			cr->restore();
		}
		void flipped(bool a_flipped) { m_flipped = a_flipped?-1:1; }
		void draw_forward(bool a_forward){ m_forward = a_forward; }
		MuxSymbol(double x=0, double y=0, double rotation=0, int gates=1, int a_inputs=2):
			Symbol(x, y, rotation), m_gates(gates), m_ninputs(a_inputs), m_forward(true) {
			inputs(m_ninputs);
			name("U");
		}
	};

	class ALUSymbol: public Symbol  {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 8, -15);
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			int cw = 10, ch = 12;
			int width = cw * 9;
			int height = ch * 2;

			bounding_rect(cr, Rect(-width/2, -height/2, width, height));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->translate(-width/2, -height/2);

			cr->move_to(0, 0);
			cr->line_to(cw*3, 0);
			cr->line_to(cw*4, ch);
			cr->line_to(cw*5, ch);
			cr->line_to(cw*6, 0);
			cr->line_to(cw*9, 0);
			cr->line_to(cw*7, ch*2);
			cr->line_to(cw*2, ch*2);

			cr->close_path();
			cr->stroke();

			cr->restore();
		}
		ALUSymbol(double x=0, double y=0, double rotation=0, int gates=1, int inputs=2):
			Symbol(x, y, rotation) {
			name("U");
		}
	};

	class SchmittSymbol: public Symbol, public prop::Inverted, public prop::Gate_Inverted  {
		bool m_dual;

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (m_dual && hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::WEST);
			if (hotspot(m_dual?2:1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (m_dual && what.match((void *)this, WHATS_AT::GATE, 0))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(m_dual?2:1);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			draw_label(cr, 8, -17);
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			int x1 = 4, x2 = 10, x3 = 15, x4 = 20, x5 = 26;
			int y1 = 6, y2 = -6;

			if (m_dual) {
				AndSymbol s = AndSymbol(2, 0, 0, 0, inverted());
				s.draw(cr);
				bounding_rect(cr, Rect(0, -15, 30, 30));

				int h = 30;
				for (int n=1; n<=2; ++n) {
					double y = -h/2+n*h/(2+1);
					hotspot(cr, n-1, Point(0, y));
					cr->move_to(-3.5, y);
					cr->line_to(0, y);
					cr->stroke();
				}
				hotspot(cr, 2, Point(h, 0));
			} else {
				BufferSymbol s = BufferSymbol(0,0,0,inverted());
				s.draw(cr);
				bounding_rect(cr, Rect(0, -15, 30, 30));
				hotspot(cr, 0, Point(0, 0));
				hotspot(cr, 1, Point(30, 0));
				x1 = 2; x2 = 7; x3 = 11; x4 = 15; x5 = 20;
				y1 = 6; y2 = -4;
			}
			cr->set_line_width(0.8);
			cr->move_to(x1,y1); cr->line_to(x2,y1); cr->line_to(x3,y2); cr->line_to(x5,y2);
			cr->move_to(x2,y1); cr->line_to(x3,y1); cr->line_to(x4,y2);
			cr->stroke();
			cr->restore();
		}

		SchmittSymbol(double x=0, double y=0, double rotation=0, bool dual=true):
			Symbol(x, y, rotation), m_dual(dual){
			inverted(false); gate_inverted(false);
			name("U");
		}
	};

	class RelaySymbol: public Symbol  {
		bool m_flipped;
		bool m_closed;
	  public:
		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::WEST);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::GATE, 0))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(2);
			return NULL;
		}

		void closed(bool a_closed) { m_closed = a_closed; }
		void flipped(bool a_flipped) { m_flipped = a_flipped; }

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double sz = 20.0;

			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			bounding_rect(cr, Rect(0, 0, sz*4, -sz));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			hotspot(cr, 0, Point(0, 0));     // Input
			hotspot(cr, 1, Point(0, -sz));   // Switch
			hotspot(cr, 2, Point(sz*4, 0));  // Output

			cr->move_to(   0,  0); cr->line_to(  sz, 0);
			cr->move_to(sz*3,  0); cr->line_to(sz*4, 0);
			cr->stroke();

			cr->move_to( 0,-sz); cr->line_to(sz*2,-sz);
			cr->stroke();

			cr->save();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			if (m_closed) {
				cr->set_line_width(0.8);
				cr->move_to(sz, 0); cr->line_to(sz*3, 0);
			} else {
				cr->arc(sz, 0, sz*2, 0, M_PI*2);
				cr->clip();
				cr->move_to(sz,0); cr->line_to(sz*3,-sz);
			}
			cr->stroke();
			cr->restore();

			cr->save();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->set_line_width(0.8);
			cr->set_dash(std::vector<double>{2, 2}, 0);
			if (m_closed) {
				cr->move_to(sz*2,-sz); cr->line_to(sz*2, 0);
			} else {
				cr->move_to(sz*2,-sz); cr->line_to(sz*2, -sz/2);
			}
			cr->stroke();
			cr->restore();

			cr->set_line_width(0.8);
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->arc(sz,   0, 2.5, 0, 2*M_PI);
			cr->fill_preserve(); cr->stroke();
			cr->arc(sz*3, 0, 2.5, 0, 2*M_PI);
			cr->fill_preserve(); cr->stroke();
			cr->restore();
		}
		RelaySymbol(double x=0, double y=0, double rotation=0, bool closed=false, bool flipped=false):
			Symbol(x, y, rotation), m_flipped(flipped), m_closed(closed) {
			name("S");
		}
	};

	class ToggleSwitchSymbol: public Symbol  {
		bool m_flipped;
		bool m_closed;
	  public:
		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::CLICK, 0, WHATS_AT::WEST);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0, WHATS_AT::EAST);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::CLICK, 0))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(2);
			return NULL;
		}

		void closed(bool a_closed) { m_closed = a_closed; }
		void flipped(bool a_flipped) { m_flipped = a_flipped; }

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double sz = 20.0;

			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(get_rotation());
			if (m_flipped) {
				cairo_matrix_t x_reflection_matrix;
				cairo_matrix_init_identity(&x_reflection_matrix);
				x_reflection_matrix.yy = -1.0;
				cr->set_matrix(x_reflection_matrix);
//				cr->translate(0, ?);
			}
 			bounding_rect(cr, Rect(0, 0, sz*4, -sz));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			hotspot(cr, 0, Point(0, 0));     // Input
			hotspot(cr, 1, Point(sz*2, sz/2));  // Switch
			hotspot(cr, 2, Point(sz*4, 0));  // Output

			cr->move_to(   0,  0); cr->line_to(  sz, 0);
			cr->move_to(sz*3,  0); cr->line_to(sz*4, 0);
			cr->stroke();

			cr->save();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			if (m_closed) {
				cr->set_line_width(0.8);
				cr->move_to(sz, 0); cr->line_to(sz*3, 0);
			} else {
				cr->arc(sz, 0, sz*2, 0, M_PI*2);
				cr->clip();
				cr->move_to(sz,0); cr->line_to(sz*3,-sz);
			}
			cr->stroke();
			cr->restore();

			cr->set_line_width(0.8);
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->arc(sz,   0, 2.5, 0, 2*M_PI);
			cr->fill_preserve(); cr->stroke();
			cr->arc(sz*3, 0, 2.5, 0, 2*M_PI);
			cr->fill_preserve(); cr->stroke();
			cr->restore();
		}

		ToggleSwitchSymbol(double x=0, double y=0, double rotation=0, bool closed=false, bool flipped=false):
			Symbol(x, y, rotation), m_flipped(flipped), m_closed(closed) {
			name("S");
		}
	};


	class BlockSymbol: public Symbol, public prop::Dimensions  {

	  public:
		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			bounding_rect(cr, Rect(-width()/2, -height()/2, width(), height()));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rectangle(-width()/2, -height()/2, width(), height());
			cr->stroke();
			cr->restore();
		}
		BlockSymbol(double x=0, double y=0, double w=100, double h=100):
			Symbol(x, y, 0) { width(w), height(h); }
	};


	class TraceSymbol: public BlockSymbol {
		double m_rowHeight;

	  public:
		struct DataPoint {
			double xratio;
			double dratio;
			DataPoint(double a_xRatio, double a_dRatio): xratio(a_xRatio), dratio(a_dRatio) {}
		};
		struct DataRow {
			std::string name;
			std::vector<DataPoint> data;
			DataRow(const std::string &a_name) : name(a_name) {}
			void add(double xRatio, double dRatio) {
				data.push_back(DataPoint(xRatio, dRatio));
			}
		};

		virtual WHATS_AT location(Point p) {
			for (size_t row = 0; row <= m_rows.size(); ++row)
				if (hotspot(row).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, row, WHATS_AT::WEST);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (size_t row = 0; row <= m_rows.size(); ++row)
				if (what.match((void *)this, WHATS_AT::INPUT, row))
					return &hotspot(row);
			return NULL;
		}

	  protected:
		std::vector<DataRow> m_rows;



		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BlockSymbol::draw(cr);
			cr->save();
			cr->translate(2+m_x-width()/2, m_y-height()/2);

			cr->set_line_width(0.8);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			int l_width = width()-2;
			int nth_row = 1;
			int text_width = 0;
			for (auto &row : m_rows) {
				Cairo::TextExtents extents;
				cr->get_text_extents(row.name, extents);
				if (text_width < extents.width) text_width = extents.width;
				cr->move_to(0, nth_row * m_rowHeight-4);
				cr->show_text(row.name);
				nth_row++;
			}
			if (text_width) text_width += 8;
			l_width -= text_width;

			for (size_t n=0; n <= m_rows.size(); ++n) {
				hotspot(cr, n, Point(0, n * m_rowHeight + m_rowHeight/2));
			}

			nth_row = 0;

			cr->translate(text_width, 2);

			for (auto &row : m_rows) {
				bool first = true;
				double v = 0;


				for (auto &pt: row.data) {
					if (first) {
						cr->move_to(pt.xratio * l_width, nth_row * m_rowHeight +  (1 - pt.dratio) * (m_rowHeight - 4));
						v = pt.dratio;
						first = false;
					} else {
						cr->line_to(pt.xratio * l_width, nth_row * m_rowHeight +  (1 - v) * (m_rowHeight - 4));
						cr->line_to(pt.xratio * l_width, nth_row * m_rowHeight +  (1 - pt.dratio) * (m_rowHeight - 4));
						v = pt.dratio;
					}
				}
				if (nth_row % 2) CairoDrawing::black(cr); else CairoDrawing::blue(cr);
				cr->stroke();
				nth_row++;
			}

			cr->restore();
		}


	  public:
		void clear_data() { m_rows.clear(); }
		DataRow &add_data_row(const std::string &a_name) {
			size_t s = m_rows.size();
			m_rows.push_back(DataRow(a_name));
			height(m_rowHeight * (s+1));
			return m_rows[s];
		}

		void set_rowHeight(double rh) {
			m_rowHeight = rh;
			height(m_rowHeight * m_rows.size());
		}

		TraceSymbol(double x=0, double y=0, double w=100, double row_height=20) :
			BlockSymbol(x, y, w, row_height),
			m_rowHeight(row_height) {}

	};


	class CounterSymbol: public BlockSymbol {
		unsigned long m_value = 0;
		int m_nBits = 8;
		bool m_is_sync = false;

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::CLOCK, 0, WHATS_AT::NORTH);
			for (int n = 0; n < m_nBits; ++n) {
				if (hotspot(2+n).close_to(p))
					return WHATS_AT(this, WHATS_AT::OUTPUT, n, WHATS_AT::SOUTH);
			}
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::CLOCK, 0))
				return &hotspot(1);
			for (int n = 0; n < m_nBits; ++n) {
				if (what.match((void *)this, WHATS_AT::OUTPUT, n))
					return &hotspot(2+n);
			}
			return NULL;
		}

		void set_value(unsigned long v) { m_value = v; }
		void set_synch(bool synch) { m_is_sync = synch; }
		void set_nbits(int nbits) { m_nBits = nbits; }

	  protected:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BlockSymbol::draw(cr);
			cr->save();
			cr->translate(2+m_x-width()/2, m_y-height()/2);
			hotspot(cr, 0, Point(0, height()/2));         // input
			hotspot(cr, 1, Point(width()/2, 0));          // clock

			cr->set_line_width(0.8);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			std::string bits="";
			for (int n=0; n < m_nBits; ++n){
				bits.insert(0, (m_value & 1 << n)?"1":"0");
			}
			bits += " [" + int_to_hex(m_value, "0x") + "]";

			cr->move_to(10, height() - 14);
			CairoDrawing::black(cr);
			cr->show_text(bits);

			if (showing_name()) {
				cr->save();
				cr->set_font_size(7);
				for (int n = 0; n < m_nBits; ++n) {           // outputs
					double x = (n+1) * width()/(m_nBits+2);
					int bit_no = (m_nBits-1-n);
					hotspot(cr, 2+bit_no, Point(x, height()));
					cr->move_to(x, height()-2);
					cr->show_text(int_to_string(bit_no));
				}
				cr->restore();
			}

			if (m_is_sync) {
				auto mid = width() / 2;
				cr->move_to(mid - 10, 0);
				cr->line_to(mid, 15);
				cr->line_to(mid+10, 0);
				cr->set_line_width(0.9);
				cr->stroke();
			}
			cr->restore();
		}

	  public:
		CounterSymbol(double x=0, double y=0, double w=100, double row_height=20) :
			BlockSymbol(x, y, w, row_height){
			name("Counter");
		}

	};


	class ShiftRegisterSymbol: public BlockSymbol {
		unsigned long m_value = 0;
		int m_nBits = 8;

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0, WHATS_AT::WEST);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::CLOCK, 0, WHATS_AT::NORTH);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0, WHATS_AT::WEST);
			if (hotspot(3).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 1, WHATS_AT::WEST);
			for (int n = 0; n < m_nBits; ++n) {
				if (hotspot(4+n).close_to(p))
					return WHATS_AT(this, WHATS_AT::OUTPUT, n, WHATS_AT::SOUTH);
			}
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			if (what.match((void *)this, WHATS_AT::CLOCK, 0))
				return &hotspot(1);
			if (what.match((void *)this, WHATS_AT::GATE, 0))
				return &hotspot(2);
			if (what.match((void *)this, WHATS_AT::INPUT, 1))
				return &hotspot(3);
			for (int n = 0; n < m_nBits; ++n) {
				if (what.match((void *)this, WHATS_AT::OUTPUT, n))
					return &hotspot(4+n);
			}
			return NULL;
		}

		void set_value(unsigned long v) { m_value = v; }
		void set_nbits(int nbits) { m_nBits = nbits; }

	  protected:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BlockSymbol::draw(cr);
			cr->save();
			cr->translate(2+m_x-width()/2, m_y-height()/2);
			hotspot(cr, 0, Point(0, height()/2));         // input
			hotspot(cr, 1, Point(width()/2, 0));          // clock
			hotspot(cr, 2, Point(0, 5));                  // enable
			hotspot(cr, 3, Point(0, height()-5));         // shift right

			cr->set_line_width(0.8);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			std::string bits="";
			for (int n=0; n < m_nBits; ++n){
				bits.insert(0, (m_value & 1 << n)?"1":"0");
			}
			bits += " [" + int_to_hex(m_value, "0x") + "]";

			cr->move_to(10, height() - 10);
			cr->text_path(bits);
			cr->set_line_width(0.6);

			CairoDrawing::black(cr); cr->fill_preserve(); cr->stroke();

			cr->save();
			cr->set_font_size(7);
			for (int n = 0; n < m_nBits; ++n) {           // outputs
				double x = (n+1) * width()/(m_nBits+2);
				int bit_no = (m_nBits-1-n);
				hotspot(cr, 4+bit_no, Point(x, height()));
				cr->move_to(x, height()-2);
				cr->show_text(int_to_string(bit_no));
			}
			cr->restore();

			auto mid = width() / 2;
			cr->move_to(mid - 10, 0);
			cr->line_to(mid, 15);
			cr->line_to(mid+10, 0);
			cr->set_line_width(0.9);
			cr->stroke();

			cr->restore();
		}

	  public:
		ShiftRegisterSymbol(double x=0, double y=0, double w=100, double row_height=20) :
			BlockSymbol(x, y, w, row_height){
			name("Shift");
		}

	};


	class GenericDiagram: public CairoDrawing {
//		Point m_pos;

		virtual bool on_motion(double x, double y, guint state) {

			for (size_t n=0; n < m_symbols.size(); ++n) {
				Symbol &s = *m_symbols[n];
				Rect r = s.bounding_rect();
				bool selected = s.selected();
				s.selected() = r.inside(Point(x, y));
				if (selected != s.selected()) {
//					m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
					m_area->queue_draw();
				}
			}
			return false;
		}

	  public:
		struct pt : public Configurable {
			double x;
			double y;
			bool is_first;
			bool is_join;
			bool is_invert;
			bool hilight;
			bool segstart;
			Point dev;   // device coordinates for x,y
			pt(double a_x, double a_y, bool first=false, bool join=false, bool invert=false):
				x(a_x), y(a_y), is_first(first), is_join(join), is_invert(invert), hilight(false), segstart(false) {}
			pt &join(bool j=true) { is_join = j; return *this; }
			pt &first(bool f=true) { is_first = f; return *this; }
			pt &invert(bool i=true) { is_invert = i; return *this; }

			virtual bool needs_first(bool &first){ first = is_first; return true; }
			virtual bool needs_join(bool &join){ join = is_join; return true; }
			virtual bool needs_invert(bool &invert){ invert = is_invert; return true; }
			virtual bool needs_position(double &a_x, double &a_y){ a_x = x; a_y = y; return true; }

			virtual void set_first(bool first){ is_first = first; }
			virtual void set_join(bool join){ is_join = join; }
			virtual void set_invert(bool invert){ is_invert = invert; }
			virtual void set_position(double a_x, double a_y){x = a_x; y = a_y; }
		};

		struct text {
			double x;
			double y;
			std::string t;
			double m_line_width;
			bool m_underscore;
			bool m_overscore;
			bool m_bold;

			text(double a_x, double a_y, const std::string &a_text):
					x(a_x), y(a_y), t(a_text), m_line_width(0.8), m_underscore(false), m_overscore(false), m_bold(false) {}
			virtual ~text() {};

			text &line_width(double w=0.8) { m_line_width = w; return *this; }
			text &underscore(bool u=true)  { m_underscore = u; return *this; }
			text &overscore(bool o=true)   { m_overscore = o; return *this; }
			text &bold(bool b=true)        { m_bold = b; return *this; }

			virtual const std::string fetch_text() { return t; }

			void render(const Cairo::RefPtr<Cairo::Context>& cr) {
				const int LINE_HEIGHT = 12;
				cr->move_to(x, y);
				size_t end=0, q=0;
				while (end < std::string::npos) {
					auto txt = this->fetch_text();
					auto start = end; end = txt.find("\n", end);
					cr->text_path(txt.substr(start, end-start));
					cr->set_line_width(m_bold?m_line_width*1.2:m_line_width);
					cr->fill_preserve();
					cr->stroke();
					if (m_overscore) {
						cr->set_line_width(1.0);
						Cairo::TextExtents extents;
						cr->get_text_extents(txt.substr(start, end-start), extents);
						double dy = y+extents.y_bearing+(q*LINE_HEIGHT)-2;
						cr->move_to(x, dy);
						cr->line_to(x + extents.width, dy);
						cr->stroke();
					}
					if (m_underscore) {
						cr->set_line_width(0.8);
						Cairo::TextExtents extents;
						cr->get_text_extents(txt.substr(start, end-start), extents);
						cr->move_to(x, y+(q*LINE_HEIGHT)+1);
						cr->line_to(x + extents.width, y+(q*LINE_HEIGHT)+1);
						cr->stroke();
					}
					if (end != std::string::npos) {
						q++; cr->move_to(x, y+(q*LINE_HEIGHT));
						end = end + 1;
					}
				}
			}

		};

		std::vector<pt> m_points;
		std::vector<text> m_text;
		std::vector<SmartPtr<text>> m_textptr;
		std::vector<SmartPtr<Symbol>> m_symbols;

		GenericDiagram &add(pt a_pt) {m_points.push_back(a_pt); return *this; }
		GenericDiagram &add(text *a_text) {m_textptr.push_back(a_text); return *this; }
		GenericDiagram &add(const text &a_text) {m_text.push_back(a_text); return *this; }
		GenericDiagram &add(Symbol *a_symbol) {m_symbols.push_back(a_symbol); return *this; }

		virtual WHATS_AT location(Point p) {

			for (size_t n = 0; n + 1 < m_points.size(); ++n) {
				auto &p1 = m_points[n];
				auto &p2 = m_points[n+1];

				if (p.close_to(p1.dev)) {
					return WHATS_AT(&p1, WHATS_AT::POINT, n, WHATS_AT::WEST);
				} else if (p.close_to(p2.dev)) {
					return WHATS_AT(&p2, WHATS_AT::POINT, n+1, WHATS_AT::WEST);
				} else if (p.close_to_line_with(p1.dev, p2.dev)) {
					p1.segstart = true;
					p2.hilight = true;
					m_area->queue_draw();
					return WHATS_AT(NULL, WHATS_AT::LINE, n, WHATS_AT::WEST);
				} else if (p2.hilight) {
					p1.segstart = false;
					p2.hilight = false;
					m_area->queue_draw();
				}
			}

			for (size_t n=0; n < m_symbols.size(); ++n) {
				WHATS_AT w = m_symbols[n]->location(p);
				if (w.what != WHATS_AT::NOTHING)
					return w;
			}

			return CairoDrawing::location(p);
		}

		// Context menu for various things
		virtual void context(const WHATS_AT &target_info) {
			if (target_info.what == WHATS_AT::SYMBOL) {
				Symbol *sym = dynamic_cast<Symbol *>(target_info.pt);
				ContextDialogFactory().popup_context(*sym);
				m_area->queue_draw();
			}
			if (target_info.what == WHATS_AT::POINT) {
				ContextDialogFactory().popup_context(*(pt *)target_info.pt);
				m_area->queue_draw();
			}
		};

		virtual void move(const WHATS_AT &target_info, const Point &destination, bool move_dia = false) {
			if (move_dia) {
				if (target_info.what == WHATS_AT::SYMBOL) {
					Symbol *sym = dynamic_cast<Symbol *>(target_info.pt);
					position(destination.diff(sym->origin()));
				}
			} else {
				if (target_info.what == WHATS_AT::SYMBOL) {
					Symbol *sym = dynamic_cast<Symbol *>(target_info.pt);
					sym->move(destination.diff(position()));
				}
			}
		}


		virtual bool determinate() {
			return true;
		}

		virtual bool signal() {
			return false;
		}

		void draw_text(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			black(cr);
			for (size_t n=0; n < m_text.size(); ++n) {
				text &t = m_text[n];
				t.render(cr);
			}
			for (size_t n=0; n < m_textptr.size(); ++n) {
				m_textptr[n]->render(cr);
			}
			cr->restore();
		}

		void draw_points(const Cairo::RefPtr<Cairo::Context>& cr) {
			for (size_t n=0; n < m_points.size(); ++n) {
				pt &point = m_points[n];
				point.dev = Point(point.x, point.y).to_device(cr, m_dev_origin);
				if (point.hilight) {
					cr->set_line_width(2.5);
				} else {
					cr->set_line_width(1.2);
				}

				if (point.is_first) {
					cr->stroke();
					cr->move_to(point.x, point.y);
				} else {
					cr->line_to(point.x, point.y);
				}
				if ((point.hilight || point.segstart) && !point.is_join && !point.is_invert) {
					cr->stroke();
					cr->move_to(point.x, point.y);
				}
				if (point.is_join) {
					cr->stroke();
					cr->arc(point.x, point.y, 2, 0, 2*M_PI);
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(point.x, point.y);
				}
				if (point.is_invert) {
					cr->stroke();
					cr->save();
					cr->set_line_width(0.9);
					cr->arc(point.x, point.y, 3.5, 0, 2*M_PI);
					white(cr); cr->fill_preserve();
					black(cr); cr->stroke();
					cr->restore();
				}
			}
			cr->stroke();
		}

		virtual void draw_extra(const Cairo::RefPtr<Cairo::Context>& cr) {}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(position().x, position().y);

			cr->set_line_width(1.2);
			if (determinate()) {
				if (signal())
					orange(cr);
				else
					gray(cr);
			} else {
				indeterminate(cr);
			}

			for (size_t n=0; n < m_symbols.size(); ++n) {
				cr->save();
				black(cr);
				m_symbols[n]->draw_symbol(cr, m_dev_origin);
				cr->restore();
			}

			draw_points(cr);
			cr->set_source_rgba(0.15,0.15,0.35,1);
			draw_text(cr);
			draw_extra(cr);

			cr->restore();
			return false;  // true stops all further drawing
		}

		GenericDiagram(double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x, y)) { }

	};


	class ConnectionDiagram: public GenericDiagram {
		Connection &m_connection;

		virtual bool determinate() {
			return m_connection.determinate();
		}

		virtual bool signal() {
			return m_connection.signal();
		}

	  public:
		ConnectionDiagram(Connection &a_conn, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			GenericDiagram(x, y, a_area), m_connection(a_conn) {}
		Connection &connection() { return m_connection; }
	};


	class WireDiagram: public GenericDiagram {
		Wire &m_wire;

		virtual bool determinate() {
			return m_wire.determinate();
		}

		virtual bool signal() {
			return m_wire.signal();
		}

	  public:
		WireDiagram(Wire &a_wire, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			GenericDiagram(x, y, a_area), m_wire(a_wire){}
	};


	class BlockDiagram:  public GenericDiagram {
		double x, y, width, height;
		Rect device_rect;
		bool clip_is_set = false;
	  public:

		virtual void recalculate() {
			if (clip_is_set) {
				m_area->queue_draw_area(device_rect.x, device_rect.y, device_rect.w, device_rect.h);
			} else {
				m_area->queue_draw();
//				std::cout << "BlockDiagram::redraw(); note: setting the clip rectangle would be more efficient!\n";
			}
		}

		void clip(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->rectangle(0, 0, width, height);
			cr->clip();
			device_rect = Rect(0, 0, width, height).to_device(cr, m_dev_origin);
			clip_is_set = true;
		}

		BlockDiagram(double x, double y, double width, double height, const std::string &a_name, Glib::RefPtr<Gtk::DrawingArea>a_area):
			GenericDiagram(x, y, a_area), x(x), y(y), width(width), height(height), device_rect(0,0,0,0)
		{
			double dw = width / 2, dh = height / 2;
			add(new BlockSymbol(dw, dh, width, height));
			if (a_name.length())
				add(text(4, 12, a_name).line_width(0.8).underscore());
		}
	};



}
