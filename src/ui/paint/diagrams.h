#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <type_traits>
#include "../../utils/utility.h"
#include "cairo_drawing.h"
#include "../../devices/devices.h"

namespace app {

	template<class GateType, class SymType, bool invert=false, bool is_xor=false>
	class GateDiagram: public CairoDrawing {
		GateType &m_gate;
		double m_rotation;
		SymType m_symbol;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.name(m_gate.name());
			m_symbol.inverted(m_gate.inverted());
			cr->save();
			position().cairo_translate(cr);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();
			return false;
		}


		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			m_gate.name(m_symbol.name());
			m_gate.inverted(m_symbol.inverted());
			m_area->queue_draw();
		};


		template <class T> void create_symbol(std::true_type) {
			m_symbol = T(0, 0, m_rotation, invert, is_xor);
		}

		template <class T> void create_symbol(std::false_type) {
			m_symbol = T(0, 0, m_rotation, invert);
		}

		GateDiagram(GateType &a_gate, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_gate(a_gate), m_rotation(rotation)
		{
			create_symbol<SymType>(std::is_same<SymType, OrSymbol>());
		}
	};

	typedef class GateDiagram<ABuffer, BufferSymbol> BufferDiagram;
	typedef class GateDiagram<Inverter, BufferSymbol, true> InverterDiagram;
	typedef class GateDiagram<AndGate, AndSymbol> AndDiagram;
	typedef class GateDiagram<AndGate, AndSymbol, true> NandDiagram;
	typedef class GateDiagram<OrGate, OrSymbol> OrDiagram;
	typedef class GateDiagram<OrGate, OrSymbol, true> NorDiagram;
	typedef class GateDiagram<XOrGate, OrSymbol, false, true> XOrDiagram;
	typedef class GateDiagram<XOrGate, OrSymbol, true, true> XNorDiagram;

	class PinDiagram:  public CairoDrawing {
		Connection &m_pin;
		double m_rotation;
		double m_scale;
		PinSymbol m_symbol;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}

		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			bool signal = m_pin.signal();
			bool indeterminate = !m_pin.determinate();

			cr->save();
			position().cairo_translate(cr);
			if (indeterminate) this->indeterminate(cr); else if (signal) this->green(cr); else this->gray(cr);

			cr->save();
			cr->rotate(m_rotation);
			cr->scale(m_scale, m_scale);
			cr->rectangle(0,-10,20,20);
			cr->fill();
			cr->restore();

			this->black(cr);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();

//			PinSymbol(m_x, m_y, m_rotation, m_scale).draw_symbol(cr, Point(m_xofs, m_yofs));


			return false;
		}

		void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
//			std::cout << "Connection change: " << name << " to " << conn->name() << std::endl;
			m_area->queue_draw_area((int)position().x, (int)position().y-10, 20, 20);
		}

		PinDiagram(Connection &a_pin, double x, double y, double rotation, double scale, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_pin(a_pin), m_rotation(rotation), m_scale(scale)
		{
			DeviceEvent<Connection>::subscribe<PinDiagram>(this, &PinDiagram::on_connection_change, &a_pin);
			m_symbol = PinSymbol(0,0, m_rotation, m_scale);
		}
	};

	class ClampDiagram:  public CairoDrawing {
		Clamp &m_clamp;

	  public:

		virtual bool on_motion(double x, double y) {
			return false;
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return NULL;
		}


		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			position().cairo_translate(cr);
			cr->set_line_width(1.2);
			black(cr);
			DiodeSymbol(0, -10, CairoDrawing::DIRECTION::UP).draw_symbol(cr, m_dev_origin);
			DiodeSymbol(0,  17, CairoDrawing::DIRECTION::UP).draw_symbol(cr, m_dev_origin);
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
			CairoDrawing(a_area, Point(x,y)), m_clamp(a_clamp)
		{}
	};

	class SchmittDiagram:  public CairoDrawing {
		Schmitt &m_schmitt;
		double m_rotation;
		bool m_dual;

		SchmittSymbol m_symbol;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

		// Create a new connection as described by InterConnect c.
		virtual void connect(Component *c){
			InterConnection *ic = dynamic_cast<InterConnection *>(c);
			if (ic) {
				if (ic->dst_index.what == WHATS_AT::INPUT)
					ic->connection = &m_schmitt.in();
				else if (ic->dst_index.what == WHATS_AT::GATE)
					ic->connection = &m_schmitt.en();
				Connection *conn = dynamic_cast<Connection *>(ic->connection);
				if (conn) {
					const Point *p1 = ic->from->point_at(ic->src_index);
					const Point *p2 = ic->to->point_at(ic->dst_index);
					if (p2 && p2) {
						ConnectionDiagram *cd = new ConnectionDiagram(*conn, 0, 0, m_area);
						ic->connection_diagram = cd;
						cd->add(ConnectionDiagram::pt(p1->x,p1->y).first());
						cd->add(ConnectionDiagram::pt(p2->x,p2->y));
						m_area->queue_draw();
					}
				}
			}
		}

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.name(m_schmitt.name());
			m_symbol.inverted(m_schmitt.out_invert());
			m_symbol.gate_inverted(m_schmitt.gate_invert());
			cr->save();
			position().cairo_translate(cr);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();
			return false;
		}

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			m_schmitt.name(m_symbol.name());
			m_schmitt.out_invert(m_symbol.inverted());
			m_schmitt.gate_invert(m_symbol.gate_inverted());
			m_area->queue_draw();
		};


		SchmittDiagram(Schmitt &a_schmitt, double x, double y, double rotation, bool dual, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_schmitt(a_schmitt), m_rotation(rotation), m_dual(dual)
		{
			m_symbol = SchmittSymbol(0, 0, m_rotation, m_dual);
		}
	};

	class TristateDiagram: public CairoDrawing {
		Tristate &m_tris;
		bool m_point_right;

		TristateSymbol m_symbol;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}


	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			m_tris.name(m_symbol.name());
			m_tris.inverted(m_symbol.inverted());
			m_tris.gate_invert(m_symbol.gate_inverted());
			m_area->queue_draw();
		};

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.name(m_tris.name());
			m_symbol.inverted(m_tris.inverted());
			m_symbol.gate_inverted(m_tris.gate_invert());

			cr->save();
			position().cairo_translate(cr);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();
			return false;
		}

		TristateDiagram(Tristate &a_tris, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_tris(a_tris), m_point_right(point_right)
		{
			double rot = m_point_right?CairoDrawing::DIRECTION::RIGHT:CairoDrawing::DIRECTION::LEFT;
			m_symbol = TristateSymbol(0,0,rot,m_tris.inverted(), m_tris.gate_invert());
			m_symbol.name(a_tris.name());
		}

	};

	class RelayDiagram: public CairoDrawing {
		Relay &m_relay;
		RelaySymbol m_symbol;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			position().cairo_translate(cr);

			m_symbol.draw_symbol(cr, m_dev_origin);

			cr->restore();
			return false;
		}

		RelayDiagram(Relay &a_relay, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_relay(a_relay) {
			m_symbol = RelaySymbol(0,0,0,m_relay.sw().signal());
		}

	};

	class MuxDiagram: public CairoDrawing {
		Mux &m_mux;
		MuxSymbol m_symbol;
		double m_rotation;

		virtual WHATS_AT location(Point p) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		void draw_forward(bool a_forward){ m_symbol.draw_forward(a_forward); }

		virtual bool on_motion(double x, double y) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			position().cairo_translate(cr);

			m_symbol.draw_symbol(cr, m_dev_origin);

			cr->restore();
			return false;
		}

		MuxDiagram(Mux &a_mux, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_mux(a_mux),  m_rotation(rotation) {
			m_symbol = MuxSymbol(0, 0, m_rotation, m_mux.no_selects(), m_mux.no_inputs());
		}

	};


	class LatchDiagram: public CairoDrawing {
		Latch &m_latch;
		bool m_point_right;
		Point m_size;
		BlockSymbol m_basic;
		LatchSymbol m_latchsym;

		virtual WHATS_AT location(Point p) {
			return m_latchsym.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_latchsym.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y) {
			Rect r = m_latchsym.bounding_rect();
			bool selected = r.inside(Point(x, y));
			if (selected != m_latchsym.selected()) {
				m_latchsym.selected() = selected;
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			position().cairo_translate(cr);

			m_basic.draw_symbol(cr, m_dev_origin);
			m_latchsym.draw_symbol(cr, m_dev_origin);

			cr->move_to(0, 82);
			cr->text_path(m_latch.name());
			cr->set_line_width(0.2);
			black(cr); cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

		LatchDiagram(Latch &a_latch, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_latch(a_latch), m_point_right(point_right), m_size(70, 70)
		{
			m_basic = BlockSymbol(m_size.x/2, m_size.y/2, m_size.x, m_size.y);
			m_latchsym = LatchSymbol(0, 0, 0, !m_point_right);
		}
	};


	class CounterDiagram: public CairoDrawing {
		Counter &m_counter;
		Point m_size;
		BlockSymbol m_basic;

		virtual WHATS_AT location(Point p) {
			return m_basic.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_basic.hotspot_at(w);
		}

		virtual bool on_motion(double x, double y) {
			Rect r = m_basic.bounding_rect();
			bool selected = m_basic.selected();
			m_basic.selected() = r.inside(Point(x, y));
			if (selected != m_basic.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {

			cr->save();
			position().cairo_translate(cr);
			m_basic.draw_symbol(cr, m_dev_origin);

			cr->set_line_width(0.8);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);

			std::string bits="";
			unsigned long value = m_counter.get();
			for (size_t n=0; n < m_counter.nbits(); ++n){
				bits.insert(0, (value & 1 << n)?"1":"0");
			}
			bits += " [" + int_to_hex(value, "0x") + "]";

			cr->move_to(10, m_size.y - 10);
			cr->text_path(bits);
			cr->set_line_width(0.6);

			black(cr); cr->fill_preserve(); cr->stroke();

			if (m_counter.is_sync()) {
				auto mid = m_size.x / 2;
				cr->move_to(mid - 10, 0);
				cr->line_to(mid, 15);
				cr->line_to(mid+10, 0);
				cr->set_line_width(0.9);
				cr->stroke();
			}

			cr->restore();
			return false;
		}

		CounterDiagram(Counter &a_counter, Glib::RefPtr<Gtk::DrawingArea>a_area, double x, double y):
			CairoDrawing(a_area, Point(x,y)), m_counter(a_counter),
			m_size(a_counter.nbits() * 7 + 50, 30 + 20 * a_counter.is_sync())
		{
			m_basic = BlockSymbol(m_size.x/2, m_size.y/2, m_size.x, m_size.y);
		}
	};


	class TraceDiagram: public CairoDrawing {
		SignalTrace &m_trace;
		int m_width;
		int m_rowHeight;
		Point m_size;
		BlockSymbol m_basic;
		std::vector<std::string> m_names;

		virtual WHATS_AT location(Point p) {
			return m_basic.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_basic.hotspot_at(w);
		}

		virtual bool on_motion(double x, double y) {
			Rect r = m_basic.bounding_rect();
			bool selected = m_basic.selected();
			m_basic.selected() = r.inside(Point(x, y));
			if (selected != m_basic.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}


	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {

			cr->save();
			cr->translate(position().x+2, position().y);
			m_basic.draw_symbol(cr, m_dev_origin);
			cr->set_line_width(0.8);
			cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
			auto first_ts = m_trace.first_us();
			auto range =  std::chrono::duration<double>(m_trace.current_us() - first_ts).count();
			int width = m_width-2;
			int nth_row = 1;
			int text_width = 0;
			for (auto &name : m_names) {
				Cairo::TextExtents extents;
				cr->get_text_extents(name, extents);
				if (text_width < extents.width) text_width = extents.width;
				cr->move_to(0, nth_row * m_rowHeight-4);
				cr->text_path(name);
				cr->fill_preserve(); cr->stroke();
				nth_row++;
			}
			if (text_width) text_width += 8;
			cr->translate(text_width, 2);
			width -= text_width;
			auto collated = m_trace.collate();
			nth_row = 0;
			for (auto &c : m_trace.traced()) {
				auto &q = collated[c];
				bool first = true;
				double v = 0;

				while (!q.empty()) {
					auto &data = q.front();
					auto ts = std::chrono::duration<double>(data.ts - first_ts).count();
					float x = ts / range;
//					std::cout << x << "[" << data.v <<  "] ";
					if (first) {
						cr->move_to(x * width, nth_row * m_rowHeight +  (1 - data.v / c->Vdd) * (m_rowHeight - 4));
						v = data.v;
						first = false;
					} else {
						cr->line_to(x * width, nth_row * m_rowHeight +  (1 - v / c->Vdd) * (m_rowHeight - 4));
						cr->line_to(x * width, nth_row * m_rowHeight +  (1 - data.v / c->Vdd) * (m_rowHeight - 4));
						v = data.v;
					}
					q.pop();
				}
//				std::cout << std::endl;
				if (nth_row % 2) black(cr); else blue(cr);
				cr->stroke();
				nth_row++;
			}

			cr->restore();
			return false;
		}

		TraceDiagram(SignalTrace &a_trace, Glib::RefPtr<Gtk::DrawingArea>a_area, double x, double y, int width=200, int row_height=20):
			CairoDrawing(a_area, Point(x,y)), m_trace(a_trace),
			m_width(width), m_rowHeight(row_height),
			m_size(m_width, a_trace.traced().size() * m_rowHeight)
		{
			m_basic = BlockSymbol(m_size.x/2, m_size.y/2, m_size.x, m_size.y);
			m_names.resize(a_trace.traced().size());
			for (size_t n=0; n < m_names.size(); ++n) {
				m_names[n] = m_trace.traced()[n]->name();
				if (not m_names[n].length())
					m_names[n] = std::string("S") + int_to_hex(n, ".");
			}
		}
	};

}
