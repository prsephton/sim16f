#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include "cairo_drawing.h"
#include "../../devices/devices.h"

namespace app {

	class Symbol {
	  protected:
		double m_x;
		double m_y;
		double m_rotation;

	  public:
		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;

		Symbol(double x=0, double y=0, double rotation=0): m_x(x), m_y(y), m_rotation(rotation) {}
		virtual ~Symbol() {}
	};


	class PinSymbol: public Symbol {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
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
		PinSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
	};

	class DiodeSymbol: public Symbol  {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->move_to(0, -6);
			cr->line_to(0,  6);
			cr->line_to(6,  0);
			cr->close_path();
			cr->stroke();
			cr->move_to(7, -7);
			cr->line_to(7,  7);
			cr->stroke();
			cr->restore();
		}
		DiodeSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
	};

	class AndSymbol: public Symbol {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			cr->move_to(40,-20); cr->line_to(0,-20); cr->line_to(0,20); cr->line_to(40,20);
			cr->stroke();
			cr->arc(40,0,20, -M_PI/2, M_PI/2);
			cr->stroke();
			cr->restore();
		}
		AndSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
	};

	class OrSymbol: public Symbol  {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->save();
			cr->rectangle(-5, -20, 40, 40);
			cr->clip();
			cr->arc(-40, 0, 40, -M_PI/2, M_PI/2);
			cr->stroke();
			cr->restore();
			cr->move_to(-5,-20); cr->line_to(40,-20);
			cr->move_to(-5, 20); cr->line_to(40, 20);
			cr->stroke();
			cr->arc(40,0,20, -M_PI/2, M_PI/2);
			cr->stroke();
			cr->restore();
		}
		OrSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation){}
	};

	class MuxSymbol: public Symbol  {
		int m_gates, m_inputs;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			int cw = 5, ch = 14;
			int width = cw * (m_gates+1);
			int height = ch * (m_inputs+1);
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

	class SchmittSymbol: public Symbol  {
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->rotate(m_rotation);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			AndSymbol().draw(cr);

			cr->stroke();
			cr->set_line_width(0.8);
			cr->move_to( 8,10); cr->line_to(18,10); cr->line_to(23,-10); cr->line_to(38,-10);
			cr->move_to(18,10); cr->line_to(23,10); cr->line_to(28,-10);
			cr->stroke();
			cr->restore();
		}
		SchmittSymbol(double x=0, double y=0, double rotation=0): Symbol(x, y, rotation) {}
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

	class BufferSymbol: public Symbol  {
		bool m_inverted;
	  public:

		virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->translate(m_x, m_y);
			cr->set_line_width(1.2);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			cr->rotate(m_rotation);
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

	class GenericDiagram: public CairoDrawing {
		int m_x;
		int m_y;

	  public:
		struct pt {
			double x;
			double y;
			bool is_first;
			bool is_join;
			bool is_invert;
			pt(double a_x, double a_y, bool first=false, bool join=false, bool invert=false):
				x(a_x), y(a_y), is_first(first), is_join(join), is_invert(invert) {}
		};

		struct text {
			double x;
			double y;
			std::string t;
			text(double a_x, double a_y, const std::string &a_text):
					x(a_x), y(a_y), t(a_text) {}
		};

		std::vector<pt> m_points;
		std::vector<text> m_text;
		std::vector<SmartPtr<Symbol>> m_symbols;

		void add(pt a_pt) {m_points.push_back(a_pt); }
		void add(text a_text) {m_text.push_back(a_text); }
		void add(Symbol *a_symbol) {m_symbols.push_back(a_symbol); }

		virtual bool determinate() {
			return true;
		}

		virtual bool signal() {
			return false;
		}

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
				m_symbols[n]->draw(cr);
			}
			for (size_t n=0; n < m_points.size(); ++n) {
				pt &point = m_points[n];
				if (point.is_join) {
					cr->stroke();
					cr->arc(point.x, point.y, 2, 0, 2*M_PI);
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(point.x, point.y);
				} else if (point.is_first) {
					cr->stroke();
					cr->move_to(point.x, point.y);
				} else {
					cr->line_to(point.x, point.y);
				}
				if (point.is_invert) {
					cr->stroke();
					cr->save();
					cr->set_line_width(0.8);
					cr->arc(point.x, point.y, 3.5, 0, 2*M_PI);
					white(cr); cr->fill_preserve();
					black(cr); cr->stroke();
					cr->restore();
				}
			}
			cr->stroke();
			cr->set_line_width(0.4);
			black(cr);
			for (size_t n=0; n < m_text.size(); ++n) {
				const int LINE_HEIGHT = 12;
				text &t = m_text[n];
				cr->move_to(t.x, t.y);
				size_t end=0, q=0;
				while (end < std::string::npos) {
					auto start = end; end = t.t.find("\n", end);
					cr->text_path(t.t.substr(start, end-start));
					if (end != std::string::npos) {
						q++; cr->move_to(t.x, t.y+(q*LINE_HEIGHT));
						end = end + 1;
					}
				}
				cr->fill_preserve();
				cr->stroke();
			}
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
}
