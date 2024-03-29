#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include "dlg_context.h"
#include "../application.h"
#include "../dispatch.h"

namespace app {

/*
 * 1. What was clicked on?
 * 2. What mouse button was clicked?
 * 3. What action to perform after click?
 * 4. Does action terminate after button release?
 *
 * -  Symbols have a bounding rectangle
 * -  Symbols may have hot spots
 * -     hot spots can identify input#, output#, etc.
 * -  Symbols only deal with visuals, and how hot spots relate to drawing space, and are not concerned
 * -    with the logical behaviour of what they represent.
 * -  Devices are classes of object which define a behaviour.  Inverters, gates, etc, are examples
 * -  of devices.
 * -  Devices do not have any interest in display logic, and are not intrinsically bound to symbols.
 * -
 * -  Diagrams may define one or more symbols, and reference one or more devices.
 * -  Diagrams are interactive controllers for devices, while symbols are static visual components.
 * -  Diagrams declare an absolute origin in drawing space, while symbols have a relative offset, so
 *    relocating a diagram will also relocate any symbols defined by the diagram.
 *
 *    A GtkDrawingArea can have many diagrams associated with it, each represented as a CairoDrawingBase()
 *    instance, and registered within an Interaction().
 *
 *    There is just one Interaction() per GtkDrawingArea,  The Interaction() is responsible for proxying
 *    any mouse movement or keyboard events to the appropriate CairoDrawing.  If each CairoDrawing were
 *    to register for these Gtk events separately, there would be a complete shambles.
 *
 * -  With regard to devices in general,
 * 		-  An 'output' defines a connection object, which may be plugged into one or more empty slots.
 * 		-  An 'input' defines a slot which may be filled with a single connection object.
 * 		-  An 'i/o' connection is an 'output' which may be switched into a 'high impedence' mode.
 * 		-  A wire has an arbitrary list of connections which may be either 'i/o' connections
 *      	or outputs.
 * 		-  A terminal is an 'i/o' connection which also looks like a wire.
 *
 */


	//________________________________________________________________
	//  A point somewhere in a Cartesian coordinate system.
	// ... on closer investigation, there's more to the point ...
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	struct Point {
		double x;
		double y;
		bool arrow;
		bool term;

		Point(): x(0), y(0), arrow(false), term(false) {}
		Point(double a_x, double a_y, bool a_arrow=false, bool a_term=false):
			x(a_x), y(a_y), arrow(a_arrow), term(a_term) {}
		Point(const Point &p): x(p.x), y(p.y), arrow(p.arrow), term(p.term) {}
		Point &scale(double a_scale) {
			x /= a_scale; y /= a_scale;
			return *this;
		}
		Point &snap(double a_grid_size) {
			x = ((int)(x/ a_grid_size)) * a_grid_size;
			y = ((int)(y/ a_grid_size)) * a_grid_size;
			return *this;
		}
		Point diff(const Point &p) const {
			return Point(x-p.x, y-p.y);
		}
		Point add(const Point &p) const {
			return Point(x+p.x, y+p.y);
		}
		Point mul(double factor) const {
			return Point(x*factor, y*factor);
		}
		Point mul(const Point &p) const {
			return Point(x*p.x, y*p.y);
		}
		Point to_device(const Cairo::RefPtr<Cairo::Context>& cr, const Point &dev_ofs) {
			Point p(x, y);
			cr->user_to_device(p.x, p.y);
			p.x -= dev_ofs.x; p.y -= dev_ofs.y;
			return p;
		}

		bool project_onto(const Point &p1, const Point &p2, Point &p, double &dist) const {
			Point delta_line = p2.diff(p1);
			Point delta_this = diff(p1);

			double len_square = delta_line.x * delta_line.x + delta_line.y * delta_line.y;
			if (len_square == 0) return false;

			double t = (delta_line.x * delta_this.x + delta_line.y * delta_this.y) / len_square;
			if (t < 0 || t > 1) return false;  // interpolation parameter

			p = p1.add(delta_line.mul(t));
			dist = fabs(delta_line.x * delta_this.y - delta_line.y * delta_this.x) / sqrt(len_square);

			return true;
		}

		bool close_to_line_with(const Point &p1, const Point &p2) const {
			const double npix = 3;
			Point p;
			double dist;

			if (project_onto(p1, p2, p, dist))
				if (dist <= npix) return true;
			return false;
		}

		void cairo_translate(const Cairo::RefPtr<Cairo::Context>& cr) const {
			cr->translate(x, y);
		}

		bool close_to(const Point &b) const {
			const double npix = 4;
			double dx = b.x - x;
			double dy = b.y - y;
			return (dx * dx + dy * dy) < npix * npix;
		}
	};

	//_________________________________________________
	// A common rectangle object
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
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

		Rect to_device(const Cairo::RefPtr<Cairo::Context>& cr, const Point &dev_ofs) {
			Point p(x, y);
			cr->user_to_device(p.x, p.y);
			p.x -= dev_ofs.x; p.y -= dev_ofs.y;
			Point d(w, h);
			cr->user_to_device_distance(d.x, d.y);
			return Rect(p.x, p.y, d.x, d.y);
		}

		Rect(double a_x, double a_y, double a_w, double a_h):
			x(a_x), y(a_y), w(a_w), h(a_h) {}
	};


	//___________________________________________________________
	// Describe something that a mouse pointer points at
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
	struct WHATS_AT {
		typedef enum {NOTHING, INPUT, OUTPUT, GATE, IN_OUT, CLOCK, CLICK, START, END, SYMBOL, LINE, POINT, TEXT} ELEMENT;
		typedef enum {EAST, SOUTH, WEST, NORTH} AFFINITY;

		Configurable *pt;
		ELEMENT what;
		int id;         // slot id
		int loc;        // location for reconnect
		AFFINITY dir = EAST;

		int rotate_affinity(double a_rotation) {
			int quad = round(a_rotation*2 / M_PI);        // a number with which to rotate affinity
			return (int) (AFFINITY)(((int)dir + quad) % 4);
		}

		WHATS_AT(Configurable *a_pt, ELEMENT a_element, int a_id, AFFINITY d=EAST): pt(a_pt), what(a_element), id(a_id), loc(a_id), dir(d) {}
		bool match(void *a_pt, ELEMENT a_what, int a_id) const {
			return (pt == a_pt && what == a_what && id == a_id);
		}
		const std::string asText(void *address) const {
			std::ostringstream s;
			s << std::hex << address;
			return s.str();
		}
		const std::string asText(int i) const {
			std::ostringstream s;
			s << i;
			return s.str();
		}
		const std::string asText(const std::string &prefix) const {
			return prefix + "::" + asText(pt) + "::" + asText((int)what) + "::" + asText(id);
		}

	};

	//___________________________________________________________________
	// Common or interface functions for a CairoDrawing
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class CairoDrawingBase: public Component {

	  protected:
		Glib::RefPtr<Gtk::DrawingArea> m_area;
		Point m_pos;                    // Position of this drawing
		Point m_mouse_pos;              // Position of the mouse pointer
		Point m_dev_origin;             // Origin of point 0,0 in device cooridinates
		double m_scale;                 // Scaling factor
		bool m_interactive;             // Can the user interact with this diagram?

	  public:

		CairoDrawingBase(Glib::RefPtr<Gtk::DrawingArea>area, const Point &a_pos): m_area(area), m_pos(a_pos), m_mouse_pos(0,0), m_dev_origin(0,0),
			m_scale(1.0), m_interactive(false) {
		}
		virtual ~CairoDrawingBase() {}


		virtual WHATS_AT location(Point p, bool for_input=false) {
			return WHATS_AT(NULL, WHATS_AT::NOTHING, 0);
		}

		virtual const Point *point_at(const WHATS_AT &w) const {
			return NULL;
		}

		bool interactive() const { return m_interactive; }
		void interactive(bool a_interactive) { m_interactive = a_interactive; }     // allow/disallow interaction
		double scale() { return m_scale; }
		void position(const Point &a_pos) { m_pos = a_pos; }
		const Point &position() const { return m_pos; }
		virtual bool deleting(CairoDrawingBase *drawing) { return false; }

		// This is overridden in CairoDrawing::slot, and not in specializations of CairoDrawing
		virtual void slot(CairoDrawingBase *source, const WHATS_AT &source_info, const WHATS_AT &target_info) {};

		//________________________________________________________________________________
		//  These two methods are overridden in specialization diagrams (see diagrams.h)
		//    to support connecting drawing elements to one another.
		//
		// Attempt to slot output from source into input at target.  Return false if we could not do it, or if
		// an existing slot was removed.  Otherwise, if a valid connection was slotted, return true.
		virtual bool slot(const WHATS_AT &w, Connection *source) { return false; };

		// Return the source connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) { return NULL; };

		// context editor for item at target
		virtual void context(const WHATS_AT &target_info) {};
		virtual Configurable *context() { return NULL; }

		// click action for item at target
		virtual void click_action(const WHATS_AT &target_info) {};

		// move the indicated item to requested location.  with move_dia=true, move the whole diagram, else the symbol
		virtual void move(const WHATS_AT &target_info, const Point &destination, bool move_dia = false) {
			if (destination.y < 0) return;
			if (destination.x < 0) return;
			position(destination);
		};

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) = 0;
		virtual bool on_motion(double x, double y, guint a_state) {
			m_mouse_pos = Point(x,y);
			m_area->queue_draw_area(2, 2, 100, 20);
			return false;
		}
		static void black(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
		}

		static void brightred(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(1.0, 0.5, 0.5, 1.0);
		}

		static void darkblue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.5, 1.0);
		}

		static void lightblue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.5, 0.5, 1.0, 1.0);
		}

		static void blue(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 1.0, 1.0);
		}

		static void selected(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.75);
		}

		static void white(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
		}

		static void gray(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.25);
		}

		static void orange(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.75, 0.55, 0.2, 1.0);
		}

		static void green(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.5, 0.95, 0.5, 1.0);
		}

		static void bright_yellow(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(1.0, 1.0, 0.75, 1.0);
		}

		static void indeterminate(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.2, 0.5, 0.75, 1.0);
		}
		static void draw_indicator(const Cairo::RefPtr<Cairo::Context>& cr, bool ind) {
			if (ind) { orange(cr); } else { gray(cr); }
			cr->save();
			cr->fill_preserve();
			cr->set_source_rgba(0.0, 0.0, 0.0, 1.0);
			cr->set_line_width(0.4);
			cr->stroke();
			cr->restore();
		}
		virtual void apply_config_changes() {}
		virtual void show_name(bool a_show) {}
		void draw_info(const Cairo::RefPtr<Cairo::Context>& cr, const std::string &a_info);
	};

	//_______________________________________________________________________
	// Connections between CairoDrawing elements
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	struct InterConnection: public Component {
		CairoDrawingBase *from;     		// source CairoDrawing for this connection
		CairoDrawingBase *to;       		// destination CairoDrawing for this connection
		WHATS_AT src_index;
		WHATS_AT dst_index;
		Connection *connection;         	    // a connection, or null.
		bool connected = false;

		void draw(const Cairo::RefPtr<Cairo::Context>& cr);

		InterConnection(
				CairoDrawingBase *source, const WHATS_AT &source_info,
				CairoDrawingBase *target, const WHATS_AT &target_info);

	};

	//___________________________________________________________
	//  Interactions with CairoDrawing elements
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class Interaction : public Component {             // Proxy
		std::vector<CairoDrawingBase *> m_drawings;

		Glib::RefPtr<Gtk::DrawingArea> m_area;
		Glib::RefPtr<Gdk::Cursor> m_cursor_arrow;
		Glib::RefPtr<Gdk::Cursor> m_cursor_in_out;
		Glib::RefPtr<Gdk::Cursor> m_cursor_input;
		Glib::RefPtr<Gdk::Cursor> m_cursor_output;
		Glib::RefPtr<Gdk::Cursor> m_cursor_start;
		Glib::RefPtr<Gdk::Cursor> m_cursor_end;
		Glib::RefPtr<Gdk::Cursor> m_cursor_click;
		Glib::RefPtr<Gdk::Cursor> m_cursor_symbol;
		Glib::RefPtr<Gdk::Cursor> m_cursor_line;
		Glib::RefPtr<Gdk::Cursor> m_cursor_point;
		Glib::RefPtr<Gdk::Cursor> m_cursor_text;
		Glib::RefPtr<Gdk::Cursor> m_cursor_del;

		int grid_size = 5;
		float m_pix_width = 860.0;
		float m_pix_height = 620.0;
		float m_alloc_width = m_pix_width;
		float m_alloc_height = m_pix_height;

		double m_scale;

		struct Action {
			CairoDrawingBase *dwg;
			Point origin;
			WHATS_AT what;
			Action(	CairoDrawingBase *a_dwg, const Point &a_origin, const WHATS_AT &a_what): dwg(a_dwg), origin(a_origin), what(a_what) {}
		};
		std::queue<Action> m_actions;

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

		void select_source_cursor(const Glib::RefPtr<Gdk::Window> &win, WHATS_AT::ELEMENT what);
		void select_target_cursor(const Glib::RefPtr<Gdk::Window> &win, WHATS_AT::ELEMENT what);
		bool button_press_event(GdkEventButton* button_event);
		bool button_release_event(GdkEventButton* button_event);
		bool motion_event(GdkEventMotion* motion_event);
		void recalc_scale();
		void size_changed(Gtk::Allocation& allocation);
	  public:

		void set_extents(float a_pix_width, float a_pix_height);
		double get_scale() const;
		void deleting(CairoDrawingBase *drawing) {
			for (auto dwg: m_drawings)
				if (dwg->deleting(drawing))
					break;
		}
		void add_drawing(CairoDrawingBase *drawing);
		void remove_drawing(CairoDrawingBase *drawing);
		Interaction(Glib::RefPtr<Gtk::DrawingArea> a_area);

	};


	class Interaction_Factory {        // Produces one interaction per Gtk::DrawingArea
		static std::map<Gtk::DrawingArea *, SmartPtr<Interaction>> m_interactions;    // singleton

	  public:
		Interaction_Factory() {}

		Interaction *produce(Glib::RefPtr<Gtk::DrawingArea> a_area) {
			auto i = m_interactions.find(a_area.operator->());
			if (i == m_interactions.end()) {
				m_interactions[a_area.operator->()] = new Interaction(a_area);
				i = m_interactions.find(a_area.operator->());
			}
			return i->second.operator ->();
		}
	};

	//_____________________________________________________________________
	// CairoDrawing instances are visual representations of components
	//   or groups of components
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
	class CairoDrawing : public CairoDrawingBase {

	  protected:
		Interaction_Factory m_interactions;                        // a factory for managing interactions
		std::map<std::string, SmartPtr<Component> > m_components;  // a registry for components added to the diagram
		std::map<std::string, SmartPtr<InterConnection> > m_connections;

		sigc::connection m_on_draw;

		// attempt to slot output from source into input at target
		// An input "slot" can only have one source at a time.  Sources may be used any number of times.
		virtual void slot(CairoDrawingBase *source, const WHATS_AT &source_info, const WHATS_AT &target_info) {
			if (source != this) {
				SmartPtr<InterConnection> ic = new InterConnection(source, source_info, this, target_info);
				std::string key = target_info.asText(std::string("Connection"));
				if (ic->connected) {
					m_components[key] = ic;
					m_connections[key] = ic;
				} else {    // disconnect toggle?
					for (auto c: m_components) {
						InterConnection *i = dynamic_cast<InterConnection *>(c.second.operator ->());
						if (i && i->src_index.match(source_info.pt, source_info.what, source_info.id))
							if (i->dst_index.match(target_info.pt, target_info.what, i->dst_index.id)) {
								std::cout << "Disconnecting " << c.first << std::endl;
								m_components.erase(c.first);
								m_connections.erase(c.first);
								break;
							}
					}
				}
			}
		};

		bool draw_content(const Cairo::RefPtr<Cairo::Context>& cr) {
			LockUI mtx;
//			std::cout << "dwg acquired lock" << std::endl;

			m_dev_origin = Point(0,0);
			cr->save();
			cr->user_to_device(m_dev_origin.x, m_dev_origin.y);
			for (auto &component: m_components) {
				InterConnection *ic = dynamic_cast<InterConnection *>(component.second.operator ->());
				if (ic) ic->draw(cr);
			}
			double l_scale = m_interactions.produce(m_area)->get_scale();
			cr->scale(l_scale, l_scale);
			bool ok = on_draw(cr);
			cr->restore();
//			std::cout << "dwg releasing lock" << std::endl;
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
			std::string coords = std::string("x: ") + int_to_string((int)m_mouse_pos.x) + "; y: " + int_to_string((int)m_mouse_pos.y);
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

		virtual void refresh() {
			m_area->queue_draw();
		}

	  public:
		struct DIRECTION {
			static constexpr double UP      = -M_PI/2.0;
			static constexpr double RIGHT   = 0.0;
			static constexpr double DOWN    = M_PI/2.0;
			static constexpr double LEFT    = M_PI;
		};

		// A CairoDrawing can contain other CairoDrawing components
		SmartPtr<Component> &component(const std::string &a_name) {
			return m_components[a_name];
		}

		std::map<std::string, SmartPtr<InterConnection> > &connections() {
			return m_connections;
		}

		void pix_extents(float a_pix_width, float a_pix_height) {
			m_interactions.produce(m_area)->set_extents(a_pix_width, a_pix_height);
		}

		CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>area, const Point &a_pos = Point(0,0)): CairoDrawingBase(area, a_pos) {
			m_on_draw = m_area->signal_draw().connect(sigc::mem_fun(*this, &CairoDrawing::draw_content));
			Dispatcher(this, "refresh").dispatcher(this, "refresh").connect(sigc::mem_fun(*this, &CairoDrawing::refresh));
			m_interactions.produce(area)->add_drawing(this);         // register this CairoDRawing area with interactions
		}

		virtual ~CairoDrawing() {
			m_on_draw.disconnect();
			m_interactions.produce(m_area)->remove_drawing(this);         // deregister this CairoDRawing area with interactions
		}

	};
}
