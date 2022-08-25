#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <cmath>
#include "cairo_drawing.h"
#include "../../devices/devices.h"

namespace app {

	struct Point {
		double x;
		double y;
		bool arrow;
		bool term;

		Point(double a_x, double a_y, bool a_arrow=false, bool a_term=false):
			x(a_x), y(a_y), arrow(a_arrow), term(a_term) {}
	};

	struct Rect {
		double x;
		double y;
		double w;
		double h;

		bool inside(Point p) {
			// is px,py inside rect(x,y,w,h) ?
			double lx=x, ly=y, lw = w, lh = h;
			if (lw < 0) { lx += lw; w = abs(lw); }
			if (lh < 0) { ly += lh; h = abs(lh); }
			p.x -= lx; p.y -= ly;
			if (p.x >= 0 && p.x <= lw && p.y >= 0 && p.y < lh) {
//				std::cout << "Test ("<< px <<", " << py << ") inside rect(" << x << ", " << y << ", " << w << ", " << h << ")" << std::endl;
				return true;
			}
			return false;
		}


		Rect(double a_x, double a_y, double a_w, double a_h):
			x(a_x), y(a_y), w(a_w), h(a_h) {}
	};

	class Symbol {
		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;

	  protected:
		double m_x;
		double m_y;
		double m_rotation;
		double m_scale;
		bool   m_selected;
		Rect   m_rect;
		Point  m_ofs;


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

		const bool selected() const { return m_selected; }
		bool &selected() { return m_selected; }

		Symbol(double x=0, double y=0, double rotation=0, double scale=1.0):
			m_x(x), m_y(y), m_rotation(rotation), m_scale(scale), m_selected(false),
			m_rect(0,0,0,0), m_ofs(0,0) {}
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			rotate(cr); scale(cr);
			bounding_rect(cr, Rect(0, -10, 20, 20));
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -7, 10, 14));
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(-5, 0, 10, 10));
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -20, 20, 40));
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -15, 30, 30));
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
		BufferSymbol(double x=0, double y=0, double rotation=0, bool inverted=false):
			Symbol(x, y, rotation), m_inverted(inverted) {}
	};

	class AndSymbol: public Symbol {
		bool m_inverted;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double h = 30;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -h/2, h*1.5, h));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->move_to(h,-h/2); cr->line_to(0,-h/2); cr->line_to(0,h/2); cr->line_to(h,h/2);
			cr->stroke();
			cr->arc(h,0,h/2, -M_PI/2, M_PI/2);
			cr->stroke();

			if (m_inverted) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h*3/2 + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}

			cr->restore();
		}
		AndSymbol(double x=0, double y=0, double rotation=0, bool inverted=false):
			Symbol(x, y, rotation), m_inverted(inverted) {}
	};

	class OrSymbol: public Symbol  {
		bool m_inverted;
		bool m_xor;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			double h   = 30.0;
			double ofs = h/8;
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			bounding_rect(cr, Rect(0, -h/2, h*1.5, h));

			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->set_line_width(1.2);

			cr->save();
			cr->rectangle(-ofs, -h/2, h-ofs, h);
			cr->clip();
			cr->scale(1/4.0, 1.0);
			cr->arc(-2*ofs, 0, h/2, -M_PI/2, M_PI/2);
			cr->restore();
			cr->stroke();
			if (m_xor) {
				cr->save();
				cr->scale(1/4.0, 1.0);
				cr->arc(ofs*2, 0, h/2, -M_PI/2, M_PI/2);
				cr->restore();
				cr->stroke();
			}

			cr->move_to(-ofs,-h/2); cr->line_to(h-ofs*2,-h/2);
			cr->move_to(-ofs, h/2); cr->line_to(h-ofs*2, h/2);
			cr->stroke();

			cr->save();
			cr->scale(1.5, 1);
			cr->arc(h/2, 0.0, h/2, -M_PI/2, M_PI/2);
			cr->restore();
			cr->stroke();

			if (m_inverted) {
				cr->save();
				cr->set_line_width(0.8);
				cr->arc(h*3/2 + 3.5, 0, 3.5, 0, 2*M_PI);
				cr->set_source_rgba(1.0, 1.0, 1.0, 1.0); cr->fill_preserve();
				cr->set_source_rgba(0.0, 0.0, 0.0, 1.0); cr->stroke();
				cr->restore();
			}
			cr->restore();
		}
		OrSymbol(double x=0, double y=0, double rotation=0, bool inverted=false, bool is_xor=false):
			Symbol(x, y, rotation), m_inverted(inverted), m_xor(is_xor){}
	};

	class MuxSymbol: public Symbol  {
		int m_gates, m_inputs;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			int cw = 5, ch = 14;
			int width = cw * (m_gates+1);
			int height = ch * (m_inputs+1);
			bounding_rect(cr, Rect(0, -height/2, width, height));
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -height/2 - width); cr->line_to(0, height/2 + width);
			cr->line_to(width,  height/2);
			cr->line_to(width, -height/2);
			cr->close_path();
			cr->stroke();

			cr->set_line_width(0.2);
			double h = (height+2*width) / (m_inputs+1);
			double ofs = 3;  // adjust for character height
			for (int r=0; r < m_inputs; ++r) {
				cr->move_to((width-cw)/2, ofs + height/2.0 + width - (r+1) * h);
				cr->save();
				cr->scale(0.8, 0.8);
				cr->text_path(int_to_hex(r, "", ""));
				cr->fill_preserve(); cr->stroke();
				cr->restore();
			}
			cr->restore();
		}
		MuxSymbol(double x=0, double y=0, double rotation=0, int gates=1, int inputs=2):
			Symbol(x, y, rotation), m_gates(gates), m_inputs(inputs) {}
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

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			if (m_dual) {
				AndSymbol s = AndSymbol();
				s.draw(cr);
				bounding_rect(cr, Rect(0, -15, 45, 30));
			} else {
				cr->move_to(0, -22); cr->line_to(0, 22);
				cr->line_to(45, 0); cr->close_path();
				bounding_rect(cr, Rect(0, -22, 45, 44));
			}
			cr->stroke();
			cr->set_line_width(0.8);
			cr->move_to( 4,6); cr->line_to(10,6); cr->line_to(15,-6); cr->line_to(26,-6);
			cr->move_to(10,6); cr->line_to(15,6); cr->line_to(20,-6);
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
		int m_x;
		int m_y;

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
			pt(double a_x, double a_y, bool first=false, bool join=false, bool invert=false):
				x(a_x), y(a_y), is_first(first), is_join(join), is_invert(invert) {}
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

			text &line_width(double w=0.8) { m_line_width = w; return *this; }
			text &underscore(bool u=true)  { m_underscore = u; return *this; }
			text &overscore(bool o=true)   { m_overscore = o; return *this; }
			text &bold(bool b=true)        { m_bold = b; return *this; }
		};

		std::vector<pt> m_points;
		std::vector<text> m_text;
		std::vector<SmartPtr<Symbol>> m_symbols;

		GenericDiagram &add(pt a_pt) {m_points.push_back(a_pt); return *this; }
		GenericDiagram &add(text a_text) {m_text.push_back(a_text); return *this; }
		GenericDiagram &add(Symbol *a_symbol) {m_symbols.push_back(a_symbol); return *this; }

		virtual bool determinate() {
			return true;
		}

		virtual bool signal() {
			return false;
		}

		void draw_text(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<text> &a_text) {
			cr->save();
			black(cr);
			for (size_t n=0; n < a_text.size(); ++n) {
				const int LINE_HEIGHT = 12;
				const text &t = a_text[n];
				cr->move_to(t.x, t.y);
				size_t end=0, q=0;
				while (end < std::string::npos) {
					auto start = end; end = t.t.find("\n", end);
					cr->text_path(t.t.substr(start, end-start));
					cr->set_line_width(t.m_bold?t.m_line_width*1.2:t.m_line_width);
					cr->fill_preserve();
					cr->stroke();
					if (t.m_overscore) {
						cr->set_line_width(1.0);
						Cairo::TextExtents extents;
						cr->get_text_extents(t.t.substr(start, end-start), extents);
						double y = t.y+extents.y_bearing+(q*LINE_HEIGHT)-2;
						cr->move_to(t.x, y);
						cr->line_to(t.x + extents.width, y);
						cr->stroke();
					}
					if (t.m_underscore) {
						cr->set_line_width(0.8);
						Cairo::TextExtents extents;
						cr->get_text_extents(t.t.substr(start, end-start), extents);
						cr->move_to(t.x, t.y+(q*LINE_HEIGHT)+1);
						cr->line_to(t.x + extents.width, t.y+(q*LINE_HEIGHT)+1);
						cr->stroke();
					}
					if (end != std::string::npos) {
						q++; cr->move_to(t.x, t.y+(q*LINE_HEIGHT));
						end = end + 1;
					}
				}
			}
			cr->restore();
		}

		void draw_points(const Cairo::RefPtr<Cairo::Context>& cr, std::vector<pt> &a_points) {
			for (size_t n=0; n < a_points.size(); ++n) {
				pt &point = a_points[n];
				if (point.is_first) {
					cr->stroke();
					cr->move_to(point.x, point.y);
				} else {
					cr->line_to(point.x, point.y);
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
			cr->translate(m_x, m_y);

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
				m_symbols[n]->draw_symbol(cr, Point(m_xofs, m_yofs));
				cr->restore();
			}

			draw_points(cr, m_points);
			cr->set_source_rgba(0.15,0.15,0.35,1);
			draw_text(cr, m_text);
			draw_extra(cr);

			cr->restore();
			return false;  // true stops all further drawing
		}

		GenericDiagram(double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area), m_x(x), m_y(y){}

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
