#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include <queue>
#include "application.h"
#include "paint/cairo_drawing.h"
#include "paint/common.h"
#include "paint/diagrams.h"
#include "../cpu_data.h"
#include "../utils/smart_ptr.h"

namespace app {

	class Scratch {    // an interface we can use to create diagrams
	  public:
		virtual const Glib::RefPtr<Gtk::Builder>& glade()=0;
		virtual Glib::RefPtr<Gtk::DrawingArea> area()=0;
		virtual void add_diagram(CairoDrawingBase *a_drawing, SmartPtr<Device> dev)=0;
		virtual CPU_DATA &cpu()=0;

		Scratch() {}
		virtual ~Scratch() {}
	};

	class ScratchDiagram: public CairoDrawing, public Scratch  {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		int m_comp_id = 0;

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->save();
			white(cr);
			cr->paint();
			black(cr);
			show_coords(cr);
			cr->move_to(120, 20);
			cr->scale(2.0, 2.0);
			cr->set_line_width(0.1);
			cr->text_path("Scratch Diagram Editor");
			cr->fill_preserve(); cr->stroke();
			cr->restore();
			return false;
		}

	  protected:

		class Menu {
			Scratch *scratch;

			Glib::RefPtr<Gtk::Menu> rails;
			Glib::RefPtr<Gtk::Menu> gates;
			Glib::RefPtr<Gtk::Menu> functions;
			Glib::RefPtr<Gtk::Menu> physical;
			Glib::RefPtr<Gtk::Menu> analog;
			Glib::RefPtr<Gtk::Menu> porta;
			Glib::RefPtr<Gtk::Menu> portb;
			int m_dev = 0;

			//					auto dia = new GenericDiagram(0, 0, scratch->area());
			//					dia->add(GenericDiagram::text(0,0,"test"));

			void on_menu_rails() {
				auto item = rails->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Vdd") {
					Voltage *l_dev = new Voltage(5, "Vdd");
					l_dev->debug(true);
					auto dia = new VddDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Vss") {
					Ground *l_dev = new Ground();
//					l_dev->debug(true);
					auto dia = new VssDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Input") {
					Input *l_dev = new Input();
					auto dia = new InputDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Connection") {
					Terminal *l_dev = new Terminal();
					auto dia = new IODiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Output") {
					Output *l_dev = new Output();
					auto dia = new OutputDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Pull-Up") {
					PullUp *l_dev = new PullUp(5, "5v");
					auto dia = new PullUpDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Terminal") {
					Terminal *l_dev = new Terminal();
					auto dia = new TerminalDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				}
				std::cout << "A menu option [" << l_label << "] was selected" << std::endl;
			}


			void on_menu_analog() {
				auto item = analog->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Resistor") {
					Terminal *l_dev = new Terminal();
					l_dev->debug(true);
					auto dia = new ResistorDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Capacitor") {
					Capacitor *l_dev = new Capacitor();
//					l_dev->debug(true);
					auto dia = new CapacitorDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Inductor") {
					Inductor *l_dev = new Inductor();
//					l_dev->debug(true);
					auto dia = new InductorDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia, l_dev);
				}
			}

			void on_menu_functions() {
				auto item = functions->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Trace") {
					SignalTrace *l_dev = new SignalTrace({});
					auto dia = new TraceDiagram(*l_dev, scratch->area(), 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Schmitt Trigger") {
					Schmitt *l_dev = new Schmitt();
					auto dia = new SchmittDiagram(*l_dev, 0, 0, 0, true, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "TriState") {
					Tristate *l_dev = new Tristate();
					auto dia = new TristateDiagram(*l_dev, true, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "D-Latch") {
					Latch *l_dev = new Latch(); l_dev->clocked(true);
					auto dia = new LatchDiagram(*l_dev, true, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "SR-Latch") {
					Latch *l_dev = new Latch(); l_dev->clocked(false);
					auto dia = new LatchDiagram(*l_dev, true, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "MUX") {
					Mux *l_dev = new Mux(std::vector<Connection *>({NULL,NULL}), std::vector<Connection *>({NULL}));
					auto dia = new MuxDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Counter") {
					Counter *l_dev = new Counter(8);
					auto dia = new CounterDiagram(*l_dev, scratch->area(), 0, 0);
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Shift Register") {

				} else if (l_label == "Clock") {

				}
			}


			void on_menu_physical() {
				auto item = physical->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Relay") {
					Relay *l_dev = new Relay({});
					auto dia = new RelayDiagram(*l_dev, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				}
			}

			void on_menu_gates() {
				auto item = gates->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Buffer") {
					ABuffer *l_dev = new ABuffer({});
					auto dia = new BufferDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Inverter") {
					Inverter *l_dev = new Inverter({});
					auto dia = new InverterDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "And") {
					AndGate *l_dev = new AndGate(std::vector<Connection *>({NULL, NULL}));
					l_dev->debug(true);
					auto dia = new AndDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Nand") {
					AndGate *l_dev = new AndGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new NandDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Or") {
					OrGate *l_dev = new OrGate(std::vector<Connection *>({NULL, NULL}));
					auto dia = new OrDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Nor") {
					OrGate *l_dev = new OrGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new NorDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "Xor") {
					XOrGate *l_dev = new XOrGate(std::vector<Connection *>({NULL, NULL}));
					auto dia = new XOrDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				} else if (l_label == "NXor") {
					XOrGate *l_dev = new XOrGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new XNorDiagram(*l_dev, 0, 0, 0, scratch->area());
					scratch->add_diagram(dia, l_dev);
				}
			}


			void on_menu_porta() {

				auto add_pinDiagram = [&](Connection &c){
					SmartPtr<Device>pin = &c;
					c.debug(true);
					pin.incRef();   // prevent pointer disposal when out of scope
					auto dia = new PinDiagram(c, 0, 0, 0, 1, scratch->area());
					scratch->add_diagram(dia, pin);
				};

				auto item = porta->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "RA0/AN0") {
					add_pinDiagram(dynamic_cast<SinglePortA_Analog &>(*(scratch->cpu().porta.RA[0])).pin());
				} else if (l_label == "RA1/AN1") {
					add_pinDiagram(dynamic_cast<SinglePortA_Analog &>(*(scratch->cpu().porta.RA[1])).pin());
				} else if (l_label == "RA2/AN2/Vref") {
					add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA2 &>(*(scratch->cpu().porta.RA[2])).pin());
				} else if (l_label == "RA3/AN3/CMP1") {
					add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA3 &>(*(scratch->cpu().porta.RA[3])).pin());
				} else if (l_label == "RA4/TOCKI/CMP2") {
					add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA4 &>(*(scratch->cpu().porta.RA[4])).pin());
				} else if (l_label == "RA5/MCLR/Vpp") {
					add_pinDiagram(dynamic_cast<SinglePortA_MCLR_RA5 &>(*(scratch->cpu().porta.RA[5])).pin());
				} else if (l_label == "RA6/OSC2/CLKOUT") {
					add_pinDiagram(dynamic_cast<SinglePortA_RA6_CLKOUT &>(*(scratch->cpu().porta.RA[6])).pin());
				} else if (l_label == "RA7/OSC1/CLKIN") {
					add_pinDiagram(dynamic_cast<PortA_RA7 &>(*(scratch->cpu().porta.RA[7])).pin());
				}
			}


			void on_menu_portb() {

				auto add_pinDiagram = [&](Connection &c){
					SmartPtr<Device>pin = &c;
					pin.incRef();     // prevent pointer disposal when out of scope
					auto dia = new PinDiagram(c, 0, 0, 0, 1, scratch->area());
					scratch->add_diagram(dia, pin);
				};

				auto item = portb->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "RB0/INT") {
					add_pinDiagram(dynamic_cast<PortB_RB0 &>(*(scratch->cpu().portb.RB[0])).pin());
				} else if (l_label == "RB1/RX/DT") {
					add_pinDiagram(dynamic_cast<PortB_RB1 &>(*(scratch->cpu().portb.RB[1])).pin());
				} else if (l_label == "RB2/TX/CK") {
					add_pinDiagram(dynamic_cast<PortB_RB2 &>(*(scratch->cpu().portb.RB[2])).pin());
				} else if (l_label == "RB3/CCP1") {
					add_pinDiagram(dynamic_cast<PortB_RB3 &>(*(scratch->cpu().portb.RB[3])).pin());
				} else if (l_label == "RB4/PGM") {
					add_pinDiagram(dynamic_cast<PortB_RB4 &>(*(scratch->cpu().portb.RB[4])).pin());
				} else if (l_label == "RB5") {
					add_pinDiagram(dynamic_cast<PortB_RB5 &>(*(scratch->cpu().portb.RB[5])).pin());
				} else if (l_label == "RB6/T1OSO/T1CKI/PGC") {
					add_pinDiagram(dynamic_cast<PortB_RB6 &>(*(scratch->cpu().portb.RB[6])).pin());
				} else if (l_label == "RB7/T1OSI/PGD") {
					add_pinDiagram(dynamic_cast<PortB_RB7 &>(*(scratch->cpu().portb.RB[7])).pin());
				}
			}

			void connect_children(Gtk::Menu * a_menu, Glib::SignalProxy<void>::SlotType a_slot) {
				auto children = a_menu->get_children();
				for (auto child: children) {
					auto item = dynamic_cast<Gtk::MenuItem *>(child);
					auto sub = item->get_submenu();
					if (sub) {
						connect_children(sub, a_slot);
					} else {
						item->signal_activate().connect(a_slot);
					}
				}
			}

		  public:
			Menu(Scratch *a_scratch): scratch(a_scratch) {
				rails = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_rails"));
				gates = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_gates"));
				functions = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_functions"));
				physical = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_physical"));
				analog = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_analog"));
				porta = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_porta"));
				portb = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_portb"));

				connect_children(rails.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_rails));
				connect_children(gates.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_gates));
				connect_children(analog.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_analog));
				connect_children(functions.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_functions));
				connect_children(physical.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_physical));
				connect_children(porta.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_porta));
				connect_children(portb.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_portb));

			}
		};

		Menu m_menu;

		struct DeviceDiagram {
			std::string cname;
			CairoDrawingBase *drawing;
			SmartPtr<Device>dev;
			std::map<std::string, SmartPtr<Component> > &components;
			int &comp_id;

			DeviceDiagram(CairoDrawingBase *a_drawing, SmartPtr<Device> a_dev,
					std::map<std::string, SmartPtr<Component> > &a_components,
					int &a_comp_id):
				drawing(a_drawing), dev(a_dev), components(a_components), comp_id(a_comp_id) {
				cname = "Component." + int_to_string(comp_id++);
				components[cname] = drawing;
			}
			~DeviceDiagram() {
				components.erase(cname);
			}
		};

		std::map<CairoDrawingBase *, SmartPtr<DeviceDiagram> > m_devices;

		const std::string asText(void *address) const {
			std::ostringstream s;
			s << std::hex << address;
			return s.str();
		}

		virtual bool deleting(CairoDrawingBase *drawing) {
			if (m_devices.find(drawing) != m_devices.end()) {
				m_devices.erase(drawing);
				return true;
			}
			return false;
		}

	  public:

		void process_queue(){
			sleep_for_us(100);
		}

		virtual CPU_DATA &cpu() { return m_cpu; }
		virtual const Glib::RefPtr<Gtk::Builder>& glade() { return m_refGlade; }
		virtual Glib::RefPtr<Gtk::DrawingArea> area() { return m_area; }
		virtual void add_diagram(CairoDrawingBase *a_drawing, SmartPtr<Device> a_dev) {
			m_devices[a_drawing] = new DeviceDiagram(a_drawing, a_dev, m_components, m_comp_id);

			a_drawing->interactive(true);
			a_drawing->position(Point(50,50));
			a_drawing->show_name(true);
		}

		void on_clock(Clock *c, const std::string &name, const std::vector<BYTE>&a_data) {
			if (name == "CLKOUT") m_area->queue_draw();
		}

		ScratchDiagram(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_scratch"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade), m_menu(this)
		{
			pix_extents(800,600);
			interactive(true);
			DeviceEvent<Clock>::subscribe<ScratchDiagram>(this, &ScratchDiagram::on_clock);
		}
		virtual ~ScratchDiagram() {
			DeviceEvent<Clock>::unsubscribe<ScratchDiagram>(this, &ScratchDiagram::on_clock);

		}

	};

	class ScratchComponent: public Component  {
		ScratchDiagram m_diagram;
		bool m_exiting;

		bool process_queue(){
			m_diagram.process_queue();
			return !m_exiting;
		}

		virtual void exiting(){
			m_exiting = true;
		}

	  public:
		ScratchComponent(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade): Component(),
		  m_diagram(a_cpu, a_refGlade), m_exiting(false) {
			Glib::signal_idle().connect( sigc::mem_fun(*this, &ScratchComponent::process_queue) );
		}
	};

}

