#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <cmath>
#include "cairo_drawing.h"
#include "dlg_context.h"
#include "../../devices/devices.h"

namespace app {

	class Symbol : public Configurable {

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;

	  protected:
		std::string m_name;
		double m_x;
		double m_y;
		double m_rotation;
		double m_scale;
		bool   m_selected;
		Rect   m_rect;
		Point  m_ofs;
		std::vector<Point> m_hotspots;

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
			cr->rotate(m_rotation);
		}

		void scale(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->scale(m_scale, m_scale);
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
			m_name(""), m_x(x), m_y(y), m_rotation(rotation), m_scale(scale), m_selected(false),
			m_rect(0,0,0,0), m_ofs(0,0) {
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
		}
	};

	class PinSymbol: public Symbol {

	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::IN_OUT, 0);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::IN_OUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -10, 20, 20));
			hotspot(cr, 0, Point(0, 0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rectangle(0,-10,20,20);
			cr->stroke();
			cr->move_to( 0,-10);
			cr->line_to(20, 10);
			cr->move_to(20,-10);
			cr->line_to( 0, 10);
			cr->stroke();
			cr->restore();
		}

		PinSymbol(double x=0, double y=0, double rotation=0, double scale=1.0): Symbol(x, y, rotation, scale) {}
	};

	class DiodeSymbol: public Symbol  {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0);
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
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
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
		DiodeSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
	};

	class VssSymbol: public Symbol {
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::INPUT, 0))
				return &hotspot(0);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(-5, 0, 10, 10));
			hotspot(cr, 0, Point(0, 0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_BUTT);
			cr->move_to(-5, 0); cr->line_to(0, 10); cr->line_to(5, 0);
			cr->close_path();
			cr->fill_preserve(); cr->stroke();
			cr->restore();
		}
		VssSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
	};

	class FETSymbol: public Symbol {
		bool m_nType;
		bool m_with_vss;
		bool m_with_vdd;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0);
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0);
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
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
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
			Symbol(x, y, rotation), m_nType(nType), m_with_vss(with_vss), m_with_vdd(with_vdd) {}
	};

	class BufferSymbol: public Symbol  {
		bool m_inverted;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0);
			if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0);
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
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -15, 30, 30));
			hotspot(cr, 0, Point(0, 0));
			hotspot(cr, 1, Point(30, 0));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -15); cr->line_to(0, 15);
			cr->line_to(30, 0); cr->close_path();
			if (m_inverted) {
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

		void inverted(bool a_invert) { m_inverted = a_invert; }
		bool inverted() const { return m_inverted; }

		BufferSymbol(double x=0, double y=0, double rotation=0, bool inverted=false):
			Symbol(x, y, rotation), m_inverted(inverted) {}
	};

	class AndSymbol: public Symbol {
		bool m_inverted;
		int  m_inputs;
	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<m_inputs; ++n)
				if (hotspot(n).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n);
			if (hotspot(m_inputs).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, m_inputs);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<m_inputs; ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(n);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(m_inputs);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double h = 30;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			bounding_rect(cr, Rect(0, -h/2, h, h));
			for (int n=1; n<=m_inputs; ++n) {
				double y = -h/2+n*h/(m_inputs+1);
				hotspot(cr, n-1, Point(0, y));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, m_inputs, Point(h, 0));

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->move_to(h/2,-h/2); cr->line_to(0,-h/2); cr->line_to(0,h/2); cr->line_to(h/2,h/2);
			cr->stroke();
			cr->arc(h/2,0, h/2, -M_PI/2, M_PI/2);
			cr->stroke();


			if (m_inverted) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}

			cr->restore();
		}
		AndSymbol(int a_inputs, double x=0, double y=0, double rotation=0, bool inverted=false):
			Symbol(x, y, rotation), m_inverted(inverted), m_inputs(a_inputs){}
	};

	class OrSymbol: public Symbol  {
		bool m_inverted;
		bool m_xor;
		int  m_inputs;
	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<m_inputs; ++n)
				if (hotspot(n).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n);
			if (hotspot(m_inputs).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, m_inputs);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<m_inputs; ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(n);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(m_inputs);
			return NULL;
		}

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double h   = 30.0;
			double ofs = h/8;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			bounding_rect(cr, Rect(0, -h/2, h, h));
			for (int n=1; n<=m_inputs; ++n) {
				double y = -h/2+n*h/(m_inputs+1);
				hotspot(cr, n-1, Point(0, -h/2+n*h/(m_inputs+1)));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, m_inputs, Point(h, 0));

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->save();
			cr->rectangle(0, -h/2, h-ofs, h);
			cr->clip();
			cr->scale(1/4.0, 1.0);
			cr->arc(-ofs, 0, h/2, -M_PI/2, M_PI/2);
			cr->restore();
			cr->stroke();

			if (m_xor) {
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

			if (m_inverted) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}
			cr->restore();
		}
		OrSymbol(int a_inputs, double x=0, double y=0, double rotation=0, bool inverted=false, bool is_xor=false):
			Symbol(x, y, rotation), m_inverted(inverted), m_xor(is_xor), m_inputs(a_inputs){}
	};


	class TristateSymbol: public BufferSymbol {
		bool m_gate_inverted;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0);
			if (hotspot(3).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 1);

			return BufferSymbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			if (what.match((void *)this, WHATS_AT::GATE, 0))
					return &hotspot(2);
			if (what.match((void *)this, WHATS_AT::GATE, 1))
					return &hotspot(3);
			return BufferSymbol::hotspot_at(what);
		}

		// context menu integration
		virtual bool needs_inverted(bool &a_inverted){ a_inverted = inverted(); return true; }
		virtual bool needs_gate_inverted(bool &a_inverted){ a_inverted = gate_inverted(); return true; }

		virtual void set_inverted(bool a_inverted){ inverted(a_inverted); }
		virtual void set_gate_inverted(bool a_inverted){ gate_inverted(a_inverted); }

		void gate_inverted(bool a_invert) { m_gate_inverted = a_invert; }
		bool gate_inverted() const { return m_gate_inverted; }

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			BufferSymbol::draw(cr);
			cr->save();

			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);

			hotspot(cr, 2, Point(15,  7.5));
			hotspot(cr, 3, Point(15, -7.5));

			cr->set_line_width(1.2);
			if (m_gate_inverted) {
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

		TristateSymbol(double x=0, double y=0, double rotation=0, bool inverted=false,  bool gate_inverted=false):
			BufferSymbol(x, y, rotation, inverted), m_gate_inverted(gate_inverted) {}

	};

	class LatchSymbol: public Symbol {
		bool m_point_right;
		bool D, Ck, Q;
	  public:

		virtual WHATS_AT location(Point p) {
			if (hotspot(0).close_to(p))
				return WHATS_AT(this, WHATS_AT::INPUT, 0);
			else if (hotspot(1).close_to(p))
				return WHATS_AT(this, WHATS_AT::GATE, 0);
			else if (hotspot(2).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0);
			else if (hotspot(3).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 1);
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
			CairoDrawing::draw_indicator(cr, m_point_right?D:Q);
			if (m_point_right) {
				cr->move_to(0, 50); cr->line_to(7,56); cr->line_to(0, 62); cr->close_path();
			} else {
				cr->rectangle(0, 52, 5, 8);
			}
			CairoDrawing::draw_indicator(cr, m_point_right?Ck:(!Q));
			cr->rectangle(65, 10, 5, 8);
			CairoDrawing::draw_indicator(cr, m_point_right?Q:D);
			if (m_point_right) {
				cr->rectangle(65,52, 5, 8);
			} else {
				cr->move_to(70, 50); cr->line_to(63,56); cr->line_to(70, 62); cr->close_path();
			}
			CairoDrawing::draw_indicator(cr, m_point_right?(!Q):Ck);
		}

		LatchSymbol(double x=0, double y=0, double rotation=0, bool backward=false):
			Symbol(x, y, rotation), m_point_right(!backward), D(false), Ck(false), Q(false) {}
	};


	class MuxSymbol: public Symbol  {
		int m_gates, m_inputs;
		bool m_forward;
	  public:

		virtual WHATS_AT location(Point p) {
			for (int n=0; n<m_gates; ++n)
				if (hotspot(n).close_to(p))
					return WHATS_AT(this, WHATS_AT::GATE, n);
			for (int n=0; n<m_inputs; ++n)
				if (hotspot(m_gates+n).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n);
			if (hotspot(m_gates+m_inputs).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, 0);
			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			for (int n=0; n<m_gates; ++n)
				if (what.match((void *)this, WHATS_AT::GATE, n))
					return &hotspot(n);
			for (int n=0; n<m_inputs; ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(m_gates+n);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(m_inputs+m_gates);
			return NULL;
		}


		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);

			int cw = 5, ch = 14;
			int width = cw * (m_gates+1);
			int height0 = ch * (m_inputs+1) + width*2;
			int height1 = ch * (m_inputs+1);
			bounding_rect(cr, Rect(0, -height0/2, width, height0));

			for (int n=1; n<=m_gates; ++n) {
				double x = width*n/(m_gates+1);
				double y = x - height0/2;
				hotspot(cr, n-1, Point(x,  y));
				cr->move_to(x, y);
				cr->line_to(x, y-3.5);
				cr->stroke();
			}
			for (int n=1; n<=m_inputs; ++n) {
				double y=-height0/2+height0*n/(m_inputs+1);
				hotspot(cr, m_gates+n-1, Point(0, y));
				cr->move_to(-3.5, y);
				cr->line_to(0, y);
				cr->stroke();
			}
			hotspot(cr, m_gates+m_inputs, Point(width, 0));

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -height0/2); cr->line_to(0, height0/2);
			cr->line_to(width,  height1/2);
			cr->line_to(width, -height1/2);
			cr->close_path();
			cr->stroke();

			cr->set_line_width(0.2);
			double h = (height0) / (m_inputs+1);
			for (int r=0; r < m_inputs; ++r) {
				cr->move_to(width/2, height0/2.0 - (r+1) * h);
				cr->save();
				cr->rotate(-m_rotation);
				cr->rel_move_to(-cw/2, 0);
				cr->scale(0.8, 0.8);
				cr->text_path(int_to_hex(m_forward?r:(m_inputs-r-1), "", ""));
				cr->fill_preserve(); cr->stroke();
				cr->restore();
			}
			cr->restore();
		}

		void draw_forward(bool a_forward){ m_forward = a_forward; }
		MuxSymbol(double x=0, double y=0, double rotation=0, int gates=1, int inputs=2):
			Symbol(x, y, rotation), m_gates(gates), m_inputs(inputs), m_forward(true) {}
	};

	class ALUSymbol: public Symbol  {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
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
			Symbol(x, y, rotation) {}
	};

	class SchmittSymbol: public Symbol  {
		bool m_dual;
	  public:

		virtual WHATS_AT location(Point p) {
			int n_inputs = m_dual?2:1;

			for (int n=0; n<n_inputs; ++n)
				if (hotspot(n).close_to(p))
					return WHATS_AT(this, WHATS_AT::INPUT, n);
			if (hotspot(n_inputs).close_to(p))
				return WHATS_AT(this, WHATS_AT::OUTPUT, n_inputs);

			return Symbol::location(p);
		}

		virtual const Point *hotspot_at(const WHATS_AT &what) const {
			int n_inputs = m_dual?2:1;
			for (int n=0; n<n_inputs; ++n)
				if (what.match((void *)this, WHATS_AT::INPUT, n))
					return &hotspot(n);
			if (what.match((void *)this, WHATS_AT::OUTPUT, 0))
				return &hotspot(n_inputs);
			return NULL;
		}


		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			int x1 = 4, x2 = 10, x3 = 15, x4 = 20, x5 = 26;
			int y1 = 6, y2 = -6;

			if (m_dual) {
				AndSymbol s = AndSymbol(2);
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
				BufferSymbol s = BufferSymbol();
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
			Symbol(x, y, rotation), m_dual(dual) {}
	};

	class BlockSymbol: public Symbol  {
		double m_w;
		double m_h;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			bounding_rect(cr, Rect(-m_w/2, -m_h/2, m_w, m_h));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rectangle(-m_w/2, -m_h/2, m_w, m_h);
			cr->stroke();
			cr->restore();
		}
		BlockSymbol(double x=0, double y=0, double w=100, double h=100):
			Symbol(x, y, 0), m_w(w), m_h(h) {}
	};

	class RelaySymbol: public Symbol  {
		bool m_flipped;
		bool m_closed;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double sz = 20.0;

			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, 0, sz*4, -sz));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

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
			Symbol(x, y, rotation), m_flipped(flipped), m_closed(closed) {}
	};

	class GenericDiagram: public CairoDrawing {
//		Point m_pos;

		virtual bool on_motion(double x, double y) {

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
		struct pt {
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

			for (size_t n=0; n < m_symbols.size(); ++n) {
				WHATS_AT w = m_symbols[n]->location(p);
				if (w.what != WHATS_AT::NOTHING)
					return w;
			}

			for (size_t n = 0; n + 1 < m_points.size(); ++n) {
				auto &p1 = m_points[n];
				auto &p2 = m_points[n+1];

				if (p.close_to(p1.dev)) {
					return WHATS_AT(this, WHATS_AT::POINT, n);
				} else if (p.close_to(p2.dev)) {
					return WHATS_AT(this, WHATS_AT::POINT, n+1);
				} else if (p.close_to_line_with(p1.dev, p2.dev)) {
					p1.segstart = true;
					p2.hilight = true;
					m_area->queue_draw();
					return WHATS_AT(this, WHATS_AT::LINE, n);
				} else if (p2.hilight) {
					p1.segstart = false;
					p2.hilight = false;
					m_area->queue_draw();
				}
			}

			return CairoDrawing::location(p);
		}

		virtual void move(const WHATS_AT &target_info, const Point &destination, bool move_dia = false) {
			if (move_dia) {
				if (target_info.what == WHATS_AT::SYMBOL) {
					Symbol *sym = (Symbol *)target_info.pt;
					position(destination.diff(sym->origin()));
				}
			} else {
				if (target_info.what == WHATS_AT::SYMBOL) {
					Symbol *sym = (Symbol *)target_info.pt;
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
	  public:

		void redraw() {
			m_area->queue_draw_area(x, y, width, height);
		}

		BlockDiagram(double x, double y, double width, double height, const std::string &a_name, Glib::RefPtr<Gtk::DrawingArea>a_area):
			GenericDiagram(x, y, a_area), x(x), y(y), width(width), height(height)
		{
			double dw = width / 2, dh = height / 2;
			add(new BlockSymbol(dw, dh, width, height));
			if (a_name.length())
				add(text(4, 12, a_name).line_width(0.8).underscore());
		}
	};



}
