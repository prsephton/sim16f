#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <cmath>
#include <limits>

namespace app {

	class CairoDrawing : public Component {

	  protected:
		Glib::RefPtr<Gtk::DrawingArea> m_area;
		double m_xpos, m_ypos, m_xofs, m_yofs;
		double m_scale;

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;
		virtual bool on_motion(double x, double y) {
			m_xpos = x; m_ypos = y;
			m_area->queue_draw_area(2, 2, 100, 20);
			return false;
		}
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
		bool motion_event(GdkEventMotion* motion_event) {
//			std::cout << "motion x=" << motion_event->x << " y=" << motion_event->y << ";" << std::endl;
			return on_motion(motion_event->x, motion_event->y);
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

		CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>area): m_area(area), m_xpos(0), m_ypos(0), m_xofs(0), m_yofs(0) {
			m_area->signal_size_allocate().connect(sigc::mem_fun(*this, &CairoDrawing::size_changed));
			m_area->signal_draw().connect(sigc::mem_fun(*this, &CairoDrawing::draw_content));
			m_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &CairoDrawing::motion_event));
			m_area->add_events(Gdk::POINTER_MOTION_MASK);
		}
		virtual ~CairoDrawing() {}
	};
}
