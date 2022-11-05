#pragma once

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <chrono>
#include <type_traits>
#include "../../utils/utility.h"
#include "../../devices/devices.h"
#include "common.h"
#include "cairo_drawing.h"

namespace app {

	class Counters {
		static std::map<std::string, int> m_counters;
	  public:
		static void reset() { m_counters.clear(); }
		static std::string next(std::string type) {
			if (not type.length()) type="U";
			auto c = m_counters.find(type);
			if (c == m_counters.end()) {
				m_counters[type] = 0;
				c = m_counters.find(type);
			}
			c->second += 1;
			return type + int_to_string(c->second);
		}
		static void rename(Symbol *sym, Device *dev) {
			if (dev->name().length()) {
				sym->name(dev->name());
			} else {
				sym->name(next(sym->name()));
				dev->name(sym->name());
			}
		}
	};

	template <class DeviceType, class SymbolType> class BasicDiagram: public CairoDrawing {
	  protected:
		SymbolType  m_symbol;
		DeviceType &m_device;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			WHATS_AT w = m_symbol.location(p);
			if (for_input)
				w.id = m_device.slot_id(w.id);
			return w;
		}

		virtual const Point *point_at(const WHATS_AT &w) const {
			const Point *pt = m_symbol.hotspot_at(w);
			if (!pt) return m_symbol.hotspot_at(WHATS_AT(w.pt, w.what, 0));
			return pt;
		}

		// Attempt to slot output from source into input at target.
		// If source is already connected, disconnect instead.
		virtual bool slot(const WHATS_AT &w, Connection *source) {
			return m_device.connect(*source);  // should work for terminals
		};

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			return dynamic_cast<Connection *>(&m_device);
		};


		virtual bool on_motion(double x, double y, guint state) {
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
			if (m_symbol.selected()) {
				draw_info(cr, m_device.info());
			}
			cr->restore();
			return false;
		}

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		}
		virtual void apply_config_changes() {
		}
		virtual Configurable *context() { return &m_symbol; }

		virtual void show_name(bool a_show) {
			m_symbol.show_name(a_show);
		}


	  public:
		BasicDiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, DeviceType &a_device, double x, double y, double rotation=0, double scale=1):
			CairoDrawing(a_area, Point(x,y)), m_symbol(x, y, rotation, scale), m_device(a_device) {
			Counters::rename(&m_symbol, &a_device);

		}

	};

	class VddDiagram: public BasicDiagram<Voltage, VddSymbol> {
		// Context menu for a VddSymbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		};

		virtual void apply_config_changes() {
			m_device.name(m_symbol.name());
			m_device.voltage(m_symbol.voltage());
		}

	  public:
		VddDiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, Voltage &a_device, double x, double y, double rotation=0, double scale=1):
			BasicDiagram<Voltage, VddSymbol>(a_area, a_device, x, y, rotation, scale) {
		}

	};

	class IODiagram: public BasicDiagram<Terminal, ConnectionSymbol> {

		// Context menu for a ConnectionSymbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
		}
		virtual void apply_config_changes() {
			m_device.name(m_symbol.name());
		}

	  public:
		IODiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, Terminal &a_device, double x, double y, double rotation=0, double scale=1):
			BasicDiagram<Terminal, ConnectionSymbol>(a_area, a_device, x, y, rotation, scale) {}

	};

	class ResistorDiagram : public BasicDiagram<Terminal, ResistorSymbol> {

		// Context menu for a ResistorSymbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		}
		virtual void apply_config_changes() {
			m_device.name(m_symbol.name());
			m_device.R(m_symbol.resistance());
		}

	  public:
		ResistorDiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, Terminal &a_device, double x, double y, double rotation=0, double scale=1):
			BasicDiagram<Terminal, ResistorSymbol>(a_area, a_device, x, y, rotation, scale) {
			m_device.R(m_symbol.resistance());
		}
	};

	class CapacitorDiagram : public BasicDiagram<Capacitor, CapacitorSymbol> {

		// Context menu for a ResistorSymbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		}
		virtual void apply_config_changes() {
			m_device.name(m_symbol.name());
			m_device.F(m_symbol.capacitance());
			m_device.reset();
		}

	  public:
		CapacitorDiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, Capacitor &a_device, double x, double y, double rotation=0, double scale=1):
			BasicDiagram<Capacitor, CapacitorSymbol>(a_area, a_device, x, y, rotation, scale) {
			m_device.F(m_symbol.capacitance());
		}
	};

	class InductorDiagram : public BasicDiagram<Inductor, InductorSymbol> {

		// Context menu for a ResistorSymbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		}
		virtual void apply_config_changes() {
			m_device.name(m_symbol.name());
			m_device.H(m_symbol.inductance());
			m_device.reset();
		}

	  public:
		InductorDiagram(Glib::RefPtr<Gtk::DrawingArea>a_area, Inductor &a_device, double x, double y, double rotation=0, double scale=1):
			BasicDiagram<Inductor, InductorSymbol>(a_area, a_device, x, y, rotation, scale) {
			m_device.H(m_symbol.inductance());
		}
	};


	typedef class BasicDiagram<Ground, VssSymbol> VssDiagram;
	typedef class BasicDiagram<Terminal, PinSymbol> TerminalDiagram;
	typedef class BasicDiagram<Input, InputSymbol> InputDiagram;
	typedef class BasicDiagram<Output, OutputSymbol> OutputDiagram;
	typedef class BasicDiagram<PullUp, PullUpSymbol> PullUpDiagram;
	typedef class BasicDiagram<Inverse, PinSymbol> InverseDiagram;


	template<class GateType, class SymType, bool invert=false, bool is_xor=false>
	class GateDiagram: public CairoDrawing {
		GateType &m_gate;
		double m_rotation;
		SymType m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y, guint state) {
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

		// Attempt to slot output from source into input at target.
		// If source is already connected, disconnect instead.
		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				auto &inputs = m_gate.inputs();
				if ((size_t)w.id >= inputs.size() || not inputs[w.id])
					return m_gate.connect(w.id, *source);
				else {
					m_gate.disconnect(w.id);
					return false;
				}
			}
			return false;
		};

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT)
				return &m_gate.rd();
			return NULL;
		};


		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_gate.name(m_symbol.name());
			m_gate.clone_output_name();
			m_gate.inverted(m_symbol.inverted());
		}

		virtual Configurable *context() { return &m_symbol; }

		template <class T> void create_or_symbol(std::true_type) {
			m_symbol = T(m_gate.inputs().size(), 0, 0, m_rotation, invert, is_xor);
		}
		template <class T> void create_or_symbol(std::false_type) {}

		template <class T> void create_and_symbol(std::true_type) {
			m_symbol = T(m_gate.inputs().size(), 0, 0, m_rotation, invert);
		}
		template <class T> void create_and_symbol(std::false_type) {}
		template <class T> void create_buffer_symbol(std::true_type) {
			m_symbol = SymType(0, 0, m_rotation, invert);
		}
		template <class T> void create_buffer_symbol(std::false_type) {}

		virtual void show_name(bool a_show) {
			m_symbol.show_name(a_show);
		}

		GateDiagram(GateType &a_gate, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_gate(a_gate), m_rotation(rotation)
		{
			create_or_symbol<SymType>(std::is_same<SymType, OrSymbol>());
			create_and_symbol<SymType>(std::is_same<SymType, AndSymbol>());
			create_buffer_symbol<SymType>(std::is_same<SymType, BufferSymbol>());
			Counters::rename(&m_symbol, &a_gate);
			a_gate.clone_output_name();
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

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}

		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}


		// Attempt to slot output from source into input at target.
		// If source is already connected, disconnect instead.
		virtual bool slot(const WHATS_AT &w, Connection *source) {
			return m_pin.connect(*source);  // should work for terminals
		};

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			return &m_pin;
		};

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_pin.name(m_symbol.name());
		}

		virtual Configurable *context() { return &m_symbol; }


	  public:

		virtual bool on_motion(double x, double y, guint state) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			m_symbol.signal(m_pin.signal());
			m_symbol.indeterminate(!m_pin.determinate());

			cr->save();
			position().cairo_translate(cr);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();

			return false;
		}

		void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
//			std::cout << "Connection change: " << name << " to " << conn->name() << std::endl;
			m_area->queue_draw_area((int)position().x, (int)position().y-10, 20, 20);
		}

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		PinDiagram(Connection &a_pin, double x, double y, double rotation, double scale, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_pin(a_pin), m_rotation(rotation), m_scale(scale)
		{
			DeviceEvent<Connection>::subscribe<PinDiagram>(this, &PinDiagram::on_connection_change, &a_pin);
			m_symbol = PinSymbol(0,0, m_rotation, m_scale);
			Counters::rename(&m_symbol, &a_pin);
		}
	};

	class ClampDiagram:  public CairoDrawing {
		Clamp &m_clamp;

	  public:

		virtual bool on_motion(double x, double y, guint state) {
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
			CairoDrawing(a_area, Point(x,y)), m_clamp(a_clamp) {
//			Counters::rename(&m_symbol, &a_clamp);
		}
	};

	class SchmittDiagram:  public CairoDrawing {
		Schmitt &m_schmitt;
		double m_rotation;
		bool m_dual;

		SchmittSymbol m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT)
				return &m_schmitt.rd();
			return NULL;
		}

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_schmitt.set_input((&m_schmitt.in())?NULL:source);
				return (&m_schmitt.in() != NULL);
			} else if (w.what == WHATS_AT::GATE) {
				m_schmitt.set_gate((&m_schmitt.en())?NULL:source);
				return (&m_schmitt.en() != NULL);
			} else
				return false;
		}


	  public:

		virtual bool on_motion(double x, double y, guint state) {
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
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_schmitt.name(m_symbol.name());
			m_schmitt.out_invert(m_symbol.inverted());
			m_schmitt.gate_invert(m_symbol.gate_inverted());
		}
		virtual Configurable *context() { return &m_symbol; }

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		SchmittDiagram(Schmitt &a_schmitt, double x, double y, double rotation, bool dual, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_schmitt(a_schmitt), m_rotation(rotation), m_dual(dual)
		{
			m_symbol = SchmittSymbol(0, 0, m_rotation, m_dual);
			Counters::rename(&m_symbol, &a_schmitt);
		}
	};

	class TristateDiagram: public CairoDrawing {
		Tristate &m_tris;
		bool m_point_right;

		TristateSymbol m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}


	  public:

		virtual bool on_motion(double x, double y, guint state) {
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
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_tris.name(m_symbol.name());
			m_tris.inverted(m_symbol.inverted());
			m_tris.gate_invert(m_symbol.gate_inverted());
		}
		virtual Configurable *context() { return &m_symbol; }

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

		void set_rotation(double rotation) {    // override rotation
			m_symbol.set_rotation(rotation);
		}

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_tris.input((&m_tris.input())?NULL:source);
				return (&m_tris.input() != NULL);
			} else if (w.what == WHATS_AT::GATE) {
				m_tris.gate((&m_tris.gate())?NULL:source);
				return (&m_tris.gate() != NULL);
			} else
				return false;
		}

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				if (w.id == 0) return &m_tris.rd();
			}
			return NULL;
		};



		TristateDiagram(Tristate &a_tris, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_tris(a_tris), m_point_right(point_right)
		{
			double rot = m_point_right?CairoDrawing::DIRECTION::RIGHT:CairoDrawing::DIRECTION::LEFT;
			m_symbol = TristateSymbol(0,0,rot,m_tris.inverted(), m_tris.gate_invert());
			m_symbol.name(a_tris.name());
			Counters::rename(&m_symbol, &a_tris);
		}

	};

	class RelayDiagram: public CairoDrawing {
		Relay &m_relay;
		RelaySymbol m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y, guint state) {
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
			bool closed = ((&m_relay.sw())?m_relay.sw().signal():false);
			m_symbol.closed(closed);
			m_symbol.draw_symbol(cr, m_dev_origin);

			cr->restore();
			return false;
		}

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_relay.in((&m_relay.in())?NULL:source);
				return (&m_relay.in() != NULL);
			} else if (w.what == WHATS_AT::GATE) {
				m_relay.sw((&m_relay.sw())?NULL:source);
				return (&m_relay.sw() != NULL);
			} else
				return false;
		}

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				if (w.id == 0) return &m_relay.rd();
			}
			return NULL;
		};

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		RelayDiagram(Relay &a_relay, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_relay(a_relay) {
			bool closed = ((&m_relay.sw())?m_relay.sw().signal():false);
			m_symbol = RelaySymbol(0,0,0,closed);
			Counters::rename(&m_symbol, &a_relay);
		}

	};

	class ToggleSwitchDiagram: public CairoDrawing, public prop::Switch {
		ToggleSwitch &m_switch;
		ToggleSwitchSymbol m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y, guint state) {
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
			bool closed = m_switch.closed();
			m_symbol.closed(closed);
			m_symbol.draw_symbol(cr, m_dev_origin);

			cr->restore();
			return false;
		}

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_switch.in((&m_switch.in())?NULL:source);
				return (&m_switch.in() != NULL);
			} else
				return false;
		}

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_switch.name(m_symbol.name());
		}
		virtual Configurable *context() { return &m_symbol; }

		void closed(bool a_closed) {
			prop::Switch::closed(a_closed);
			m_switch.closed(prop::Switch::closed());
			m_symbol.closed(prop::Switch::closed());
		}

		// click action for item at target
		virtual void click_action(const WHATS_AT &target_info) {
			closed(not prop::Switch::closed());
		};

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				if (w.id == 0) return &m_switch.rd();
			}
			return NULL;
		};

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		ToggleSwitchDiagram(ToggleSwitch &a_switch, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_switch(a_switch) {
			bool closed = a_switch.closed();
			m_symbol = ToggleSwitchSymbol(0,0,0,closed);
			Counters::rename(&m_symbol, &a_switch);
		}

	};


	class MuxDiagram: public CairoDrawing {
		Mux &m_mux;
		MuxSymbol m_symbol;
		double m_rotation;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

	  public:

		void draw_forward(bool a_forward){ m_symbol.draw_forward(a_forward); }

		virtual bool on_motion(double x, double y, guint state) {
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

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_mux.in(w.id, (&m_mux.in(w.id))?NULL:source);
				return (&m_mux.in(w.id) != NULL);
			} else if (w.what == WHATS_AT::GATE) {
				m_mux.select(w.id, (&m_mux.select(w.id))?NULL:source);
				return (&m_mux.select(w.id) != NULL);
			} else
				return false;
		}

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				return &m_mux.rd();
			}
			return NULL;
		};

		// Context menu for m_symbol
		virtual void context(const WHATS_AT &target_info) {
			ContextDialogFactory().popup_context(m_symbol);
			apply_config_changes();
			m_area->queue_draw();
		};
		virtual void apply_config_changes() {
			m_mux.name(m_symbol.name());
			m_mux.configure(m_symbol.inputs(), m_symbol.gate_count());
		}
		virtual Configurable *context() { return &m_symbol; }

		void set_scale(double a_scale) {m_symbol.set_scale(a_scale); }
		void flipped(bool a_flipped) { m_symbol.flipped(a_flipped); }

		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		MuxDiagram(Mux &a_mux, double x, double y, double rotation, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_mux(a_mux),  m_rotation(rotation) {
			m_symbol = MuxSymbol(0, 0, m_rotation, m_mux.no_selects(), m_mux.no_inputs());
			Counters::rename(&m_symbol, &a_mux);
		}

	};


	class LatchDiagram: public CairoDrawing {
		Latch &m_latch;
		bool m_point_right;
		Point m_size;
		BlockSymbol m_basic;
		LatchSymbol m_latchsym;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_latchsym.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_latchsym.hotspot_at(w);
		}

	  public:

		virtual bool on_motion(double x, double y, guint state) {
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

		virtual void show_name(bool a_show) {
			m_latchsym.show_name(true);
		}

		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				m_latch.D((&m_latch.D())?NULL:source);
				return (&m_latch.D() != NULL);
			} else if (w.what == WHATS_AT::GATE) {
				m_latch.Ck((&m_latch.Ck())?NULL:source);
				return (&m_latch.Ck() != NULL);
			} else
				return false;
		}

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				if (w.id == 0) return &m_latch.Q();
				if (w.id == 1) return &m_latch.Qc();
			}
			return NULL;
		};


		LatchDiagram(Latch &a_latch, bool point_right, double x, double y, Glib::RefPtr<Gtk::DrawingArea>a_area):
			CairoDrawing(a_area, Point(x,y)), m_latch(a_latch), m_point_right(point_right), m_size(70, 70)
		{
			m_basic = BlockSymbol(m_size.x/2, m_size.y/2, m_size.x, m_size.y);
			m_latchsym = LatchSymbol(0, 0, 0, !m_point_right, a_latch.clocked());
			Counters::rename(&m_latchsym, &a_latch);
		}
	};


	class CounterDiagram: public CairoDrawing {
		Counter &m_counter;
		Point m_size;
		CounterSymbol m_symbol;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

		virtual bool on_motion(double x, double y, guint state) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}

	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {

			cr->save();
			position().cairo_translate(cr);

			m_symbol.set_value(m_counter.get());
			m_symbol.set_synch(m_counter.is_sync());
			m_symbol.set_nbits(m_counter.nbits());
			m_symbol.draw_symbol(cr, m_dev_origin);

			cr->restore();
			return false;
		}

		// Attempt to slot output from source into input at target.
		// If source is already connected, disconnect instead.
		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				if (m_counter.get_input() == source) {
					m_counter.set_input(NULL);
					return false;
				}
				m_counter.set_input(source);
				return true;
			} else if (w.what == WHATS_AT::CLOCK) {
				if (m_counter.get_clock() == source) {
					m_counter.set_clock(NULL);
					return false;
				}
				m_counter.set_clock(source);
				return true;
			}
			return false;
		};

		// Return the connection at the indicated location
		virtual Connection *slot(const WHATS_AT &w) {
			if (w.what == WHATS_AT::OUTPUT) {
				return &m_counter.bit(w.id);
			}
			return NULL;
		};


		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		CounterDiagram(Counter &a_counter, Glib::RefPtr<Gtk::DrawingArea>a_area, double x, double y):
			CairoDrawing(a_area, Point(x,y)), m_counter(a_counter),
			m_size(a_counter.nbits() * 7 + 50, 30 + 20 * a_counter.is_sync())
		{
			m_symbol = CounterSymbol(m_size.x/2, m_size.y/2, m_size.x, m_size.y);
			Counters::rename(&m_symbol, &a_counter);
			a_counter.set_name(a_counter.name());
		}
	};


	class TraceDiagram: public CairoDrawing {
		SignalTrace &m_trace;
		int m_width;
		int m_rowHeight;
		Point m_size;
		TraceSymbol m_symbol;
		std::vector<std::string> m_names;

		virtual WHATS_AT location(Point p, bool for_input=false) {
			return m_symbol.location(p);
		}
		virtual const Point *point_at(const WHATS_AT &w) const {
			return m_symbol.hotspot_at(w);
		}

		virtual bool on_motion(double x, double y, guint state) {
			Rect r = m_symbol.bounding_rect();
			bool selected = m_symbol.selected();
			m_symbol.selected() = r.inside(Point(x, y));
			if (selected != m_symbol.selected()) {
				m_area->queue_draw_area(r.x-2, r.y-2, r.w+4, r.h+4);
			}
			return false;
		}


		void set_symbol_data() {
			auto first_ts = m_trace.first_us();
			auto range =  std::chrono::duration<double>(m_trace.current_us() - first_ts).count(); // @suppress("Symbol is not resolved")
			auto collated = m_trace.collate();
			int nth = 0;
			m_symbol.clear_data();
			for (auto &c : m_trace.traced()) {
				auto &q = collated[c];
				auto &row = m_symbol.add_data_row(m_names[nth++]);
				while (!q.empty()) {
					auto &data = q.front();
					auto ts = std::chrono::duration<double>(data.ts - first_ts).count(); // @suppress("Symbol is not resolved")
					double x = ts / range;
					row.add(x, data.v/c->Vdd);
					q.pop();
				}
			}
		}


		// Attempt to slot output from source into input at target.
		// If source is already connected, disconnect instead.
		virtual bool slot(const WHATS_AT &w, Connection *source) {
			if (w.what == WHATS_AT::INPUT) {
				if (m_trace.has_trace(source)) {
					m_trace.remove_trace(source);
					set_names();
					return false;
				}
				if (m_trace.add_trace(source, w.id)) {
					set_names();
					return true;
				}
			}
			return false;
		};


	  public:

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			set_symbol_data();
			cr->save();
			cr->translate(position().x+2, position().y);
			m_symbol.draw_symbol(cr, m_dev_origin);
			cr->restore();
			return false;
		}


		virtual void show_name(bool a_show) {
			m_symbol.show_name(true);
		}

		void set_rowHeight(double rh) {
			m_symbol.set_rowHeight(rh);
			m_rowHeight = rh;
		}

		void clear_traces() {
			m_trace.clear_traces();
			set_names();
		}

		void set_names() {
			m_names.resize(m_trace.traced().size());
			for (size_t n=0; n < m_names.size(); ++n) {
				m_names[n] = m_trace.traced()[n]->name();
				if (not m_names[n].length())
					m_names[n] = std::string("S") + int_to_hex(n, ".");
			}
		}


		TraceDiagram(SignalTrace &a_trace, Glib::RefPtr<Gtk::DrawingArea>a_area, double x, double y, int width=200, int row_height=20):
			CairoDrawing(a_area, Point(x,y)), m_trace(a_trace),
			m_width(width), m_rowHeight(row_height),
			m_size(m_width, a_trace.traced().size() * m_rowHeight)
		{
			m_symbol = TraceSymbol(m_size.x/2, m_size.y/2, m_size.x, row_height);
			set_names();
			Counters::rename(&m_symbol, &a_trace);
		}
	};

}
