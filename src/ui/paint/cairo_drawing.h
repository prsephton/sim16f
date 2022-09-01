#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include "../application.h"

namespace app {

/*
 * struct GdkEventButton {
 *     GdkEventType type;
 *     GdkWindow* window;
 *     gint8 send_event;
 *     guint32 time;
 *     gdouble x;
 *     gdouble y;
 *     gdouble* axes;
 *     GdkModifierType* state;
 *     guint button;
 *     GdkDevice* device;
 *     gdouble x_root;
 *     gdouble y_root;
 *  }
 */

/*
 * 1. What was clicked on?
 * 2. What mouse button was clicked?
 * 3. What action to perform after click?
 * 4. Does action terminate after button release?
 *
 * -  Symbols have a bounding rectangle
 * -  Symbols may have hot spots
 * -     hot spots can identify input# or output#
 * -  Diagrams may define one or more symbols
 * -  Diagrams are interactive, while symbols are static
 * -  Diagrams declare an absolute origin, while symbols have a relative offset
 * -  A single output connection may connect to one or more inputs; literally,
 *      the output is plugged into an input slot; there is only one
 *      connection.
 * -  A wire has an arbitrary list of connections which may be either inputs
 *      or outputs.
 * -  A terminal is an i/o connection which also looks like a wire.
 *
 */



	struct Point {
		double x;
		double y;
		bool arrow;
		bool term;

		Point(double a_x, double a_y, bool a_arrow=false, bool a_term=false):
			x(a_x), y(a_y), arrow(a_arrow), term(a_term) {}

		bool close_to(const Point &b) {
			const double npix = 3;
			double dx = b.x - x;
			double dy = b.y - y;
			return (dx * dx + dy * dy) < npix * npix;
		}

	};

	struct Rect {
		double x;
		double y;
		double w;
		double h;

		bool inside(Point p) const {
			// is px,py inside rect(x,y,w,h) ?
			double lx=x, ly=y, lw = w, lh = h;

			if (lw < 0) { lx += lw; lw = abs(lw); }
			if (lh < 0) { ly += lh; lh = abs(lh); }
			p.x -= lx; p.y -= ly;
			if (p.x >= 0 && p.x <= lw && p.y >= 0 && p.y < lh) {
				return true;
			}
			return false;
		}


		Rect(double a_x, double a_y, double a_w, double a_h):
			x(a_x), y(a_y), w(a_w), h(a_h) {}
	};

	struct WHATS_AT {
		typedef enum {NOTHING, INPUT, OUTPUT, IN_OUT, START, END, SYMBOL} ELEMENT;

		void *pt;
		ELEMENT what;
		int id;

		WHATS_AT(void *a_pt, ELEMENT a_element, int a_id): pt(a_pt), what(a_element), id(a_id){}
	};


	class CairoDrawingBase: public Component {

	  protected:
		Glib::RefPtr<Gtk::DrawingArea> m_area;
		double m_xpos, m_ypos, m_xofs, m_yofs;
		double m_scale;
		bool m_interactive;

	  public:

		CairoDrawingBase(Glib::RefPtr<Gtk::DrawingArea>area): m_area(area), m_xpos(0), m_ypos(0), m_xofs(0), m_yofs(0),
			m_scale(1.0), m_interactive(false) {
		}
		virtual ~CairoDrawingBase() {}


		virtual WHATS_AT location(Point p) {
			return WHATS_AT(this, WHATS_AT::NOTHING, 0);
		}
		bool interactive() const { return m_interactive; }
		void interactive(bool a_interactive) { m_interactive = a_interactive; }

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;
		virtual bool on_motion(double x, double y) {
			m_xpos = x; m_ypos = y;
			m_area->queue_draw_area(2, 2, 100, 20);
			return false;
		}
	};


	class Interaction : public Component {             // Proxy
		std::vector<CairoDrawingBase *> m_drawings;

		Glib::RefPtr<Gtk::DrawingArea> m_area;
		Glib::RefPtr<Gdk::Cursor> m_cursor_arrow;
		Glib::RefPtr<Gdk::Cursor> m_cursor_in_out;
		Glib::RefPtr<Gdk::Cursor> m_cursor_input;
		Glib::RefPtr<Gdk::Cursor> m_cursor_output;
		Glib::RefPtr<Gdk::Cursor> m_cursor_start;
		Glib::RefPtr<Gdk::Cursor> m_cursor_end;
		Glib::RefPtr<Gdk::Cursor> m_cursor_symbol;


		bool button_press_event(GdkEventButton* button_event) {

			return true;  // Highlander: stop propagating this event
		}

		bool button_release_event(GdkEventButton* button_event) {

			return true;  // Highlander: stop propagating this event
		}

		bool motion_event(GdkEventMotion* motion_event) {
//			std::cout << "motion x=" << motion_event->x << " y=" << motion_event->y << ";" << std::endl;

			std::stack<WHATS_AT> locations;
			Glib::RefPtr<Gdk::Window> win = Glib::wrap(motion_event->window, true);

			for (auto &dwg : m_drawings) {
				WHATS_AT w = dwg->location(Point(motion_event->x, motion_event->y));
				if (w.what!=WHATS_AT::NOTHING) {
					locations.push(w);
				}
				dwg->on_motion(motion_event->x, motion_event->y);
			}
			if (locations.size()) {
				auto w = locations.top();
				if (w.what==WHATS_AT::IN_OUT) {   // normal cursor
					win->set_cursor(m_cursor_in_out);
				} else if (w.what==WHATS_AT::INPUT) {   // normal cursor
					win->set_cursor(m_cursor_input);
				} else if (w.what==WHATS_AT::OUTPUT) {   // normal cursor
					win->set_cursor(m_cursor_output);
				} else if (w.what==WHATS_AT::START) {   // normal cursor
					win->set_cursor(m_cursor_start);
				} else if (w.what==WHATS_AT::END) {   // normal cursor
					win->set_cursor(m_cursor_end);
				} else if (w.what==WHATS_AT::SYMBOL) {   // normal cursor
					win->set_cursor(m_cursor_symbol);
//					std::cout << w.pt << ": sym" << std::endl;
				}
			} else {
				win->set_cursor(m_cursor_arrow);
			}
			return true;   // there can be only one!
		}


	  public:

		void add_drawing(CairoDrawingBase *drawing) {
			m_drawings.push_back(drawing);
		}

		Interaction(Glib::RefPtr<Gtk::DrawingArea> a_area): m_area(a_area) {
			m_cursor_arrow = Gdk::Cursor::create(Gdk::CursorType::ARROW);
			m_cursor_in_out = Gdk::Cursor::create(Gdk::CursorType::DOTBOX);
			m_cursor_output = Gdk::Cursor::create(Gdk::CursorType::DOT);
			m_cursor_input = Gdk::Cursor::create(Gdk::CursorType::PLUS);
			m_cursor_start = Gdk::Cursor::create(Gdk::CursorType::LEFT_SIDE);
			m_cursor_end = Gdk::Cursor::create(Gdk::CursorType::RIGHT_SIDE);
			m_cursor_symbol = Gdk::Cursor::create(Gdk::CursorType::TCROSS);


			m_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &Interaction::motion_event));
			m_area->signal_button_press_event().connect(sigc::mem_fun(*this, &Interaction::button_press_event));
			m_area->signal_button_release_event().connect(sigc::mem_fun(*this, &Interaction::button_release_event));

			m_area->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK );

		}

	};


	class Interaction_Factory {        // Produces one interaction per Gtk::DrawingArea
		static std::map<Gtk::DrawingArea *, SmartPtr<Interaction>> m_interactions;    // singleton

	  public:
		Interaction_Factory() {}

		SmartPtr<Interaction> produce(Glib::RefPtr<Gtk::DrawingArea> a_area) {
			auto i = m_interactions.find(a_area.operator->());
			if (i == m_interactions.end()) {
				m_interactions[a_area.operator->()] = new Interaction(a_area);
				i = m_interactions.find(a_area.operator->());
			}
			return i->second;
		}
	};


	class CairoDrawing : public CairoDrawingBase {

	  protected:
		Interaction_Factory m_interactions;


		bool draw_content(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_xofs=0; m_yofs=0;
			cr->save();
			cr->user_to_device(m_xofs, m_yofs);
			cr->scale(m_scale, m_scale);
			bool ok = on_draw(cr);
			cr->restore();
			return ok;
		}

		void show_coords(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			cr->set_source_rgba(0.2,1,1,1);
			cr->rectangle(14,0,100,16);
			cr->fill();
			cr->set_source_rgba(0,0,0,1);
			cr->set_line_width(0.7);
			cr->move_to(14, 10);
			std::string coords = std::string("x: ") + int_to_string((int)m_xpos) + "; y: " + int_to_string((int)m_ypos);
			cr->text_path(coords);
			cr->fill_preserve(); cr->stroke();
			cr->restore();
		}

		// distance from a point
		double distance(double x, double y, double p1, double p2) {
			double dx = p1 - x;
			double dy = p2 - y;
			double squares = dx * dx + dy * dy;
			if (squares) return sqrt(squares);
			return 0;
		}

		// distance from a line segment
		double distance(double x, double y, double px1, double py1, double px2, double py2) {
			px2 -= px1; py2 -= py1;
			x -= px1; y -= py1;               // ensure lines both use 0,0 as base, compare apples to apples
			double theta = atan2(py2, px2);
			double sin_theta = sin(theta);
			double cos_theta = cos(theta);
			double yc = y * cos_theta + x * sin_theta;        // rotate point by theta
			double xc = x * cos_theta - y * sin_theta;
			double xr = px2 * cos_theta - py2 * sin_theta;    // xr == max length
			if (xc < 0 || xc > xr)
				return std::numeric_limits<double>::infinity();  // No perpendicular solution
			return fabs(yc);
		}

	  public:
		struct DIRECTION {
			static constexpr double UP      = -M_PI/2.0;
			static constexpr double RIGHT   = 0.0;
			static constexpr double DOWN    = M_PI/2.0;
			static constexpr double LEFT    = M_PI;
		};

		void black(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
		}

		void darkblue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.5, 1.0);
		}

		void lightblue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.5, 0.5, 1.0, 1.0);
		}

		void blue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 1.0, 1.0);
		}

		void selected(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.75);
		}

		void white(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
		}

		void gray(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.25);
		}

		void orange(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.75, 0.55, 0.2, 1.0);
		}

		void green(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.5, 0.95, 0.5, 1.0);
		}

		void indeterminate(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.2, 0.5, 0.75, 1.0);
		}

		void draw_indicator(const Cairo::RefPtr<Cairo::Context>& cr, bool ind) {
			if (ind) { orange(cr); } else { gray(cr); }
			cr->save();
			cr->fill_preserve();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->set_line_width(0.4);
			cr->stroke();
			cr->restore();
		}

		void size_changed(Gtk::Allocation& allocation) {
			double swidth = allocation.get_width() / 860.0;
			double sheight = allocation.get_height() / 620.0;
//			std::cout << "w: " << std::dec << (int)(allocation.get_width()) << "; h: " << std::dec << (int)(allocation.get_height()) << std::endl;
			m_scale = swidth < sheight?swidth:sheight;  // maintain aspect ratio
		}


		CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>area): CairoDrawingBase(area) {
			m_area->signal_size_allocate().connect(sigc::mem_fun(*this, &CairoDrawing::size_changed));
			m_area->signal_draw().connect(sigc::mem_fun(*this, &CairoDrawing::draw_content));
			m_interactions.produce(area)->add_drawing(this);         // register this CairoDRawing area with interactions
		}
		virtual ~CairoDrawing() {}
	};
}
