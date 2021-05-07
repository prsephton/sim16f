#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>

namespace app {

	class CairoDrawing : public Component {
	  protected:
		Glib::RefPtr<Gtk::DrawingArea> m_area;
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;

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

		void white(const Cairo::RefPtr<Cairo::Context>& cr) {
		  cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
		}

		void gray(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.25);
		}

		void orange(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.75, 0.55, 0.2, 1.0);
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

		CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>area): m_area(area) {
			m_area->signal_draw().connect(sigc::mem_fun(*this, &CairoDrawing::on_draw));
		}
		virtual ~CairoDrawing() {}
	};
}
