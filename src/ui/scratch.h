#include <gtkmm.h>
#include <sstream>
#include <iomanip>
#include <queue>
#include "application.h"
#include "paint/cairo_drawing.h"
#include "paint/common.h"
#include "paint/diagrams.h"
#include "fileselection.h"
#include "../cpu_data.h"
#include "../utils/smart_ptr.h"
#include "../utils/scratch_writer.h"

namespace app {

	struct DeviceDiagram {  // A component we store in our scratch pad
		std::string dtype;
		std::string label;
		std::string cname;
		CairoDrawing *drawing;
		SmartPtr<Device>dev;
		std::map<std::string, SmartPtr<Component> > &components;
		int this_id=0;
		int &comp_id;

		std::map<std::string, SmartPtr<InterConnection>>::iterator connection;;

		bool first_connection() {
			connection = drawing->connections().begin();
			return connection != drawing->connections().end();
		}

		bool next_connection() {
			++connection;
			return connection != drawing->connections().end();
		}

		const std::string id() {
			return std::string("Citem") + as_text(this_id);
		}

		DeviceDiagram(
				const std::string &a_dtype,
				const std::string &a_label,
				CairoDrawing *a_drawing,
				SmartPtr<Device> a_dev,
				std::map<std::string, SmartPtr<Component> > &a_components,
				int &a_comp_id,
				const std::string &a_cname=""):
					dtype(a_dtype), label(a_label), drawing(a_drawing), dev(a_dev),
					components(a_components), comp_id(a_comp_id) {
			this_id = comp_id;
			cname = a_cname.length()?a_cname:"Component." + int_to_string(comp_id);
			comp_id++;
			components[cname] = drawing;
		}
		~DeviceDiagram() {
			components.erase(cname);
		}
	};


	class Scratch {    // an interface we can use to create diagrams
	  public:
		virtual const Glib::RefPtr<Gtk::Builder>& glade()=0;
		virtual Glib::RefPtr<Gtk::DrawingArea> area()=0;
		virtual DeviceDiagram * add_diagram(
				const std::string &a_devtype, const std::string &a_label,
				CairoDrawing *a_drawing, SmartPtr<Device> dev,
				const std::string &cname)=0;
		virtual void clear() = 0;
		virtual CPU_DATA &cpu()=0;
		virtual void load_file() = 0;
		virtual void save_file() = 0;
		virtual bool saved() = 0;
		Scratch() {}
		virtual ~Scratch() {}
	};

	class ScratchDiagram: public CairoDrawing, public Scratch, public ScratchConverter  {
		CPU_DATA &m_cpu;
		Glib::RefPtr<Gtk::Builder> m_refGlade;
		int m_comp_id = 0;
		bool m_saved = true;

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
		FileSelection *m_file_chooser;
		std::string m_filename;

		class Menu {
			Scratch *scratch;

			Glib::RefPtr<Gtk::Menu> file;
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
				create_rails(l_label);
			}



			void on_menu_analog() {
				auto item = analog->get_active();
				const Glib::ustring l_label = item->get_label();
				create_analog(l_label);
			}


			void on_menu_functions() {
				auto item = functions->get_active();
				const Glib::ustring l_label = item->get_label();
				create_functions(l_label);
			}


			void on_menu_physical() {
				auto item = physical->get_active();
				const Glib::ustring l_label = item->get_label();
				create_physical(l_label);
			}

			void on_menu_gates() {
				auto item = gates->get_active();
				const Glib::ustring l_label = item->get_label();
				create_gates(l_label);
			}

			void on_menu_porta() {
				auto item = porta->get_active();
				const Glib::ustring l_label = item->get_label();
				create_porta(l_label);
			}

			void on_menu_portb() {
				auto item = portb->get_active();
				const Glib::ustring l_label = item->get_label();
				create_portb(l_label);
			}

			bool check_saved() {
				if (not scratch->saved()) {
					auto dialog = Gtk::MessageDialog(
							"This scratch pad has unsaved changes.\nDo you want to continue?",
							false,
							Gtk::MESSAGE_WARNING,
							Gtk::BUTTONS_OK_CANCEL,
							true);
					auto result = dialog.run();
					if (result == Gtk::RESPONSE_CANCEL) {
						return false;
					}
				}
				return true;
			}

			void on_menu_file() {
				auto item = file->get_active();
				const Glib::ustring l_label = item->get_label();

				if (l_label == "New") {
					if (check_saved())
						scratch->clear();
				} else if (l_label == "Save") {
					scratch->save_file();
				} else if (l_label == "Load") {
					if (check_saved())
						scratch->load_file();
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

			DeviceDiagram *create_rails(const Glib::ustring &a_label, const std::string &a_cname="")
			{
				std::string dtype = "rails";
				if (a_label == "Vdd") {
					Voltage *l_dev = new Voltage(5, "Vdd");
					l_dev->debug(true);
					auto dia = new VddDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Vss") {
					Ground *l_dev = new Ground();
//					l_dev->debug(true);
					auto dia = new VssDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Input") {
					Input *l_dev = new Input();
					auto dia = new InputDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Connection") {
					Terminal *l_dev = new Terminal();
					auto dia = new IODiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Output") {
					Output *l_dev = new Output();
					auto dia = new OutputDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Pull-Up") {
					PullUp *l_dev = new PullUp(5, "5v");
					auto dia = new PullUpDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Terminal") {
					Terminal *l_dev = new Terminal();
					auto dia = new TerminalDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				}
				return NULL;
			}

			DeviceDiagram *create_analog(const Glib::ustring &a_label, const std::string &a_cname="")
			{
				std::string dtype = "analog";
				if (a_label == "Resistor") {
					Terminal *l_dev = new Terminal();
					l_dev->debug(true);
					auto dia = new ResistorDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Capacitor") {
					Capacitor *l_dev = new Capacitor();
//					l_dev->debug(true);
					auto dia = new CapacitorDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Inductor") {
					Inductor *l_dev = new Inductor();
//					l_dev->debug(true);
					auto dia = new InductorDiagram(scratch->area(), *l_dev, 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				}
				return NULL;
			}

			DeviceDiagram *create_functions(const Glib::ustring &a_label, const std::string &a_cname="") {
				std::string dtype = "functions";
				if (a_label == "Trace") {
					SignalTrace *l_dev = new SignalTrace({});
					auto dia = new TraceDiagram(*l_dev, scratch->area(), 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Schmitt Trigger") {
					Schmitt *l_dev = new Schmitt();
					auto dia = new SchmittDiagram(*l_dev, 0, 0, 0, true, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "TriState") {
					Tristate *l_dev = new Tristate();
					auto dia = new TristateDiagram(*l_dev, true, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "D-Latch") {
					Latch *l_dev = new Latch(); l_dev->clocked(true);
					auto dia = new LatchDiagram(*l_dev, true, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "SR-Latch") {
					Latch *l_dev = new Latch(); l_dev->clocked(false);
					auto dia = new LatchDiagram(*l_dev, true, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "MUX") {
					Mux *l_dev = new Mux(std::vector<Connection *>({NULL,NULL}), std::vector<Connection *>({NULL}));
					auto dia = new MuxDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Counter") {
					Counter *l_dev = new Counter(8);
					auto dia = new CounterDiagram(*l_dev, scratch->area(), 0, 0);
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Shift Register") {

				} else if (a_label == "Clock") {

				}
				return NULL;
			}

			DeviceDiagram *create_physical(const Glib::ustring &a_label, const std::string &a_cname="")
			{
				std::string dtype = "physical";
				if (a_label == "Switch") {
					ToggleSwitch *l_dev = new ToggleSwitch({});
					auto dia = new ToggleSwitchDiagram(*l_dev, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Relay") {
					Relay *l_dev = new Relay({});
					auto dia = new RelayDiagram(*l_dev, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				}
				return NULL;
			}

			DeviceDiagram *create_gates(const Glib::ustring &a_label, const std::string &a_cname="") {
				std::string dtype = "gates";
				if (a_label == "Buffer") {
					ABuffer *l_dev = new ABuffer({});
					auto dia = new BufferDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Inverter") {
					Inverter *l_dev = new Inverter({});
					auto dia = new InverterDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "And") {
					AndGate *l_dev = new AndGate(std::vector<Connection *>({NULL, NULL}));
					l_dev->debug(true);
					auto dia = new AndDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Nand") {
					AndGate *l_dev = new AndGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new NandDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Or") {
					OrGate *l_dev = new OrGate(std::vector<Connection *>({NULL, NULL}));
					auto dia = new OrDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Nor") {
					OrGate *l_dev = new OrGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new NorDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "Xor") {
					XOrGate *l_dev = new XOrGate(std::vector<Connection *>({NULL, NULL}));
					auto dia = new XOrDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				} else if (a_label == "NXor") {
					XOrGate *l_dev = new XOrGate(std::vector<Connection *>({NULL, NULL}), true);
					auto dia = new XNorDiagram(*l_dev, 0, 0, 0, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, l_dev, a_cname);
				}
				return NULL;
			}

			DeviceDiagram *create_porta(const Glib::ustring &a_label, const std::string &a_cname="") {
				std::string dtype = "porta";
				auto add_pinDiagram = [&](Connection &c){
					SmartPtr<Device>pin = &c;
					c.debug(true);
					pin.incRef();   // prevent pointer disposal when out of scope
					auto dia = new PinDiagram(c, 0, 0, 0, 1, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, pin, a_cname);
				};

				if (a_label == "RA0/AN0") {
					return add_pinDiagram(dynamic_cast<SinglePortA_Analog &>(*(scratch->cpu().porta.RA[0])).pin());
				} else if (a_label == "RA1/AN1") {
					return add_pinDiagram(dynamic_cast<SinglePortA_Analog &>(*(scratch->cpu().porta.RA[1])).pin());
				} else if (a_label == "RA2/AN2/Vref") {
					return add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA2 &>(*(scratch->cpu().porta.RA[2])).pin());
				} else if (a_label == "RA3/AN3/CMP1") {
					return add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA3 &>(*(scratch->cpu().porta.RA[3])).pin());
				} else if (a_label == "RA4/TOCKI/CMP2") {
					return add_pinDiagram(dynamic_cast<SinglePortA_Analog_RA4 &>(*(scratch->cpu().porta.RA[4])).pin());
				} else if (a_label == "RA5/MCLR/Vpp") {
					return add_pinDiagram(dynamic_cast<SinglePortA_MCLR_RA5 &>(*(scratch->cpu().porta.RA[5])).pin());
				} else if (a_label == "RA6/OSC2/CLKOUT") {
					return add_pinDiagram(dynamic_cast<SinglePortA_RA6_CLKOUT &>(*(scratch->cpu().porta.RA[6])).pin());
				} else if (a_label == "RA7/OSC1/CLKIN") {
					return add_pinDiagram(dynamic_cast<PortA_RA7 &>(*(scratch->cpu().porta.RA[7])).pin());
				}
				return NULL;
			}

			DeviceDiagram *create_portb(const Glib::ustring &a_label, const std::string &a_cname="") {
				std::string dtype = "portb";
				auto add_pinDiagram = [&](Connection &c){
					SmartPtr<Device>pin = &c;
					pin.incRef();     // prevent pointer disposal when out of scope
					auto dia = new PinDiagram(c, 0, 0, 0, 1, scratch->area());
					return scratch->add_diagram(dtype, a_label, dia, pin, a_cname);
				};

				if (a_label == "RB0/INT") {
					return add_pinDiagram(dynamic_cast<PortB_RB0 &>(*(scratch->cpu().portb.RB[0])).pin());
				} else if (a_label == "RB1/RX/DT") {
					return add_pinDiagram(dynamic_cast<PortB_RB1 &>(*(scratch->cpu().portb.RB[1])).pin());
				} else if (a_label == "RB2/TX/CK") {
					return add_pinDiagram(dynamic_cast<PortB_RB2 &>(*(scratch->cpu().portb.RB[2])).pin());
				} else if (a_label == "RB3/CCP1") {
					return add_pinDiagram(dynamic_cast<PortB_RB3 &>(*(scratch->cpu().portb.RB[3])).pin());
				} else if (a_label == "RB4/PGM") {
					return add_pinDiagram(dynamic_cast<PortB_RB4 &>(*(scratch->cpu().portb.RB[4])).pin());
				} else if (a_label == "RB5") {
					return add_pinDiagram(dynamic_cast<PortB_RB5 &>(*(scratch->cpu().portb.RB[5])).pin());
				} else if (a_label == "RB6/T1OSO/T1CKI/PGC") {
					return add_pinDiagram(dynamic_cast<PortB_RB6 &>(*(scratch->cpu().portb.RB[6])).pin());
				} else if (a_label == "RB7/T1OSI/PGD") {
					return add_pinDiagram(dynamic_cast<PortB_RB7 &>(*(scratch->cpu().portb.RB[7])).pin());
				}
				return NULL;
			}


			Menu(Scratch *a_scratch): scratch(a_scratch) {
				file = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_file"));
				rails = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_rails"));
				gates = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_gates"));
				functions = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_functions"));
				physical = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_physical"));
				analog = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_analog"));
				porta = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_porta"));
				portb = Glib::RefPtr<Gtk::Menu>::cast_dynamic(scratch->glade()->get_object("mn_portb"));

				connect_children(file.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_file));
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

		std::map<CairoDrawingBase *, SmartPtr<DeviceDiagram> > m_devices;
		std::map<CairoDrawingBase *, SmartPtr<DeviceDiagram> >::iterator m_current_device;

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

		virtual CPU_DATA &cpu() { return m_cpu; }
		virtual const Glib::RefPtr<Gtk::Builder>& glade() { return m_refGlade; }
		virtual Glib::RefPtr<Gtk::DrawingArea> area() { return m_area; }

		Glib::RefPtr<Gtk::Window> main_window(){
			return Glib::RefPtr<Gtk::Window>::cast_dynamic(m_refGlade->get_object("sim16f_main"));
//			Window *w = m_window.operator->();
		}
		virtual DeviceDiagram *add_diagram(
				const std::string &a_devtype,
				const std::string &a_label,
				CairoDrawing *a_drawing,
				SmartPtr<Device> a_dev,
				const std::string &a_cname) {

			auto dia = new DeviceDiagram(a_devtype, a_label, a_drawing, a_dev, m_components, m_comp_id, a_cname);
			m_devices[a_drawing] = dia;

			a_drawing->interactive(true);
			a_drawing->position(Point(50,50));
			a_drawing->show_name(true);
			m_saved = false;
			return dia;
		}

		virtual void clear(){
			m_devices.clear();
			m_saved = true;
		}

		bool saved() { return m_saved; }

		void load_file() {
			try {
				std::string l_filename = m_file_chooser->load_scratch_file();
				ScratchXML xml(l_filename);
				clear();
				xml.load(*this);
				m_saved = true;
			} catch (std::string &err) {
				auto dialog = Gtk::MessageDialog(
						err, false,
						Gtk::MESSAGE_INFO,
						Gtk::BUTTONS_OK,
						true);
				dialog.run();
			}
		}

		void save_file() {
			try {
				std::string l_filename = m_filename.substr(0, m_filename.find("."))+".scratch";
				l_filename = m_file_chooser->save_scratch_file(l_filename);
				l_filename = l_filename.substr(0, l_filename.find("."))+".scratch";

				ScratchXML xml(*this);
				xml.dump(l_filename);
				m_saved = true;
			} catch (std::string &err) {
				auto dialog = Gtk::MessageDialog(
						err, false,
						Gtk::MESSAGE_INFO,
						Gtk::BUTTONS_OK,
						true);
				dialog.run();
			}
		}

		//__________________________________________________________
		//  ScratchConverter Methods
		bool first_item() {                                // locate first element to store
			m_current_device = m_devices.begin();
			return (m_current_device != m_devices.end());
		}

		bool next_item() {                                  // locate next element
			return (++m_current_device != m_devices.end());
		}

		bool first_connection() {
			auto dwg = m_current_device->second;
			return dwg->first_connection();
		}

		bool next_connection() {
			auto dwg = m_current_device->second;
			return dwg->next_connection();
		}

		std::map<std::string, std::string> source_info(){   // attributes describing connection source
			std::map<std::string, std::string> info;
			auto dwg = m_current_device->second;
			auto ic = dwg->connection->second;         // interconnect
			auto source = m_devices.find(ic->from);   // the target diagram for the connection
			if (source != m_devices.end()) {
				auto &what = ic->src_index;
				info["slot_type"] = as_text(what.what);
				info["slot_id"] = as_text(what.loc);
				info["slot_dir"] = as_text(what.dir);
				info["id"] = source->second->id();
			}
			return info;
		}

		std::map<std::string, std::string> target_info(){     // attributes describing connection target
			std::map<std::string, std::string> info;
			auto dwg = m_current_device->second;
			auto ic = dwg->connection->second;
			auto target = m_devices.find(ic->to);   // the target diagram for the connection
			if (target != m_devices.end()) {
				auto &what = ic->dst_index;
				info["slot_type"] = as_text(what.what);
				info["slot_id"] = as_text(what.loc);
				info["slot_dir"] = as_text(what.dir);
				info["id"] = target->second->id();
			}
			return info;
		}

		std::string get_id() {                                // retrieve identifier for this element
			return m_current_device->second->id();
		}

		std::string element_class()  {                        // The kind of element, eg: "Resistor"
			return m_current_device->second->dtype;
		}

		std::string element_label()  {                        // The kind of element, eg: "Resistor"
			return m_current_device->second->label;
		}

		std::map<std::string, std::string>  attributes()  {             // what attributes stored for this element?
			std::map<std::string, std::string> attrs;

			auto dwg = m_current_device->second->drawing;
			Configurable *cfg = dwg->context();

			if (cfg) {
				std::string st;
				double d;
				float f;
				double w,h,red,green,blue;
				int i;
				bool b;

				attrs["position"] = as_text(dwg->position().x, dwg->position().y);

				if (cfg->needs_name(st)){ attrs["name"] = st; }
				if (cfg->needs_voltage(d)){ attrs["voltage"] = as_text(d); }
				if (cfg->needs_resistance(d)){ attrs["resistance"] = as_text(d); }
				if (cfg->needs_capacitance(d)){ attrs["capacitance"] = as_text(d); }
				if (cfg->needs_inductance(d)){ attrs["inductance"] = as_text(d); }
				if (cfg->needs_inverted(b)){ attrs["inverted"] = as_text(b); }
				if (cfg->needs_gate_inverted(b)){ attrs["gate_inverted"] = as_text(b); }
				if (cfg->needs_orientation(d)){ attrs["orientation"] = as_text(d); }
				if (cfg->needs_scale(d)){ attrs["scale"] = as_text(d); }
				if (cfg->needs_trigger(b)){ attrs["trigger"] = as_text(b); }
				if (cfg->needs_size(w, h)){ attrs["size"] = as_text(w, h); }
				if (cfg->needs_rows(i)){ attrs["rows"] = as_text(i); }
				if (cfg->needs_inputs(i)){ attrs["inputs"] = as_text(i); }
				if (cfg->needs_selectors(i)){ attrs["selectors"] = as_text(i); }
				if (cfg->needs_ntype(b)){ attrs["ntype"] = as_text(b); }
				if (cfg->needs_synchronous(b)){ attrs["synchronous"] = as_text(b); }
				if (cfg->needs_first(b)){ attrs["first"] = as_text(b); }
				if (cfg->needs_join(b)){ attrs["join"] = as_text(b); }
				if (cfg->needs_invert(b)){ attrs["invert"] = as_text(b); }
				if (cfg->needs_underscore(b)){ attrs["underscore"] = as_text(b); }
				if (cfg->needs_overscore(b)){ attrs["overscore"] = as_text(b); }
				if (cfg->needs_bold(b)){ attrs["bold"] = as_text(b); }
				if (cfg->needs_switch(b)){ attrs["switch"] = as_text(b); }
				if (cfg->needs_font(st, f)){ attrs["font"] = st + " " + as_text(f); }
				if (cfg->needs_fg_colour(red, green, blue)){ attrs["fg_colour"] = as_text(red,green,blue); }
				if (cfg->needs_xor(b)){ attrs["xor"] = as_text(b); }
			} else {
				auto dwg = m_current_device->second->drawing;
				auto dev = m_current_device->second->dev;
				attrs["name"] = dev->name();
				attrs["position"] = as_text(dwg->position().x, dwg->position().y);
			}
			return attrs;
		}

		ATTR_TYPE atype(const std::string &attr)  {        // what is the type for this attribute?
			if (std::string("voltage, resistance, capacitance, inductance, orientation, scale").find(attr) != std::string::npos)
				return ScratchConverter::fp;

			if (std::string(":inverted, :gate_inverted, :trigger, :ntype, :synchronous").find(":"+attr) != std::string::npos)
				return ScratchConverter::boolean;

			if (std::string("first, join, invert, underscore, overscore, bold, switch, xor").find(attr) != std::string::npos)
				return ScratchConverter::boolean;

			if (std::string("rows, inputs, selectors").find(attr) != std::string::npos)
				return ScratchConverter::integer;

			return ScratchConverter::str;
		}

		void create_element(
				const std::string name,
				const std::string cls,
				const std::string label,
				std::vector<ATTRIBUTE> attributes) {
			std::cout << "Create element name=" << name << ", class=" << cls << ", label=" << label << std::endl;
			DeviceDiagram *dia = NULL;

			if (cls == "rails")
				dia = m_menu.create_rails(label, name);
			else if (cls == "gates")
				dia = m_menu.create_gates(label, name);
			else if (cls == "functions")
				dia = m_menu.create_functions(label, name);
			else if (cls == "physical")
				dia = m_menu.create_physical(label, name);
			else if (cls == "analog")
				dia = m_menu.create_analog(label, name);
			else if (cls == "porta")
				dia = m_menu.create_porta(label, name);
			else if (cls == "portb")
				dia = m_menu.create_portb(label, name);

			if (dia) {
				Configurable *cfg = dia->drawing->context();
				for (auto &attr: attributes) {
					if (attr.name == "name")
						dia->dev->name(attr.data.str);
					else if (attr.name == "position") {
						auto x_str = attr.data.str.substr(1, attr.data.str.find(',')-1);
						auto y_str = attr.data.str.substr(attr.data.str.find(',')+1);
						y_str = y_str.substr(0, y_str.find(")"));
						double x = as_double(x_str);
						double y = as_double(y_str);
						dia->drawing->position(Point(x,y));
					} else if (attr.name == "orientation") {
						cfg->set_orientation(attr.data.fp);
					} else if (attr.name == "scale") {
						cfg->set_scale(attr.data.fp);
					} else if (attr.name == "voltage") {
						cfg->set_voltage(attr.data.fp);
					} else if (attr.name == "resistance") {
						cfg->set_resistance(attr.data.fp);
					}
				}
				dia->drawing->apply_config_changes();
			}
		}

		WHATS_AT get_info(std::map<std::string, std::string>attrs, CairoDrawingBase *&drawing){
			auto found = m_components.find(attrs["id"]);
			if (found == m_components.end())
				throw std::string("Cannot locate component: ")+attrs["id"];
			drawing = dynamic_cast<CairoDrawingBase *>(found->second.operator ->());
			if (!drawing)
				throw std::string("Cannot find the drawing for: ")+attrs["id"];
			Configurable *cfg = drawing->context();
			int id = as_int(attrs["slot_id"]);
			WHATS_AT::ELEMENT element = (WHATS_AT::ELEMENT)as_int(attrs["slot_type"]);
			WHATS_AT::AFFINITY dir = (WHATS_AT::AFFINITY)as_int(attrs["slot_dir"]);
			return WHATS_AT(cfg, element, id, dir);
		}

		virtual void connect(std::map<std::string, std::string>from_attrs, std::map<std::string, std::string>to_attrs) {
			std::cout << "Connect " << from_attrs["id"] << " to " << to_attrs["id"] << std::endl;
			CairoDrawingBase *source, *target;
			auto source_info = get_info(from_attrs, source);
			auto target_info = get_info(to_attrs, target);
			if (m_devices.find(target) != m_devices.end()) {
				auto dia = m_devices[target];
				target_info.id = dia->dev->slot_id(target_info.id);
			}
			target->slot(source, source_info, target_info);
		}

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`

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
			m_refGlade->get_widget_derived("file_chooser", m_file_chooser);
		}
		virtual ~ScratchDiagram() {
			DeviceEvent<Clock>::unsubscribe<ScratchDiagram>(this, &ScratchDiagram::on_clock);

		}

	};

	class ScratchComponent: public Component  {
		ScratchDiagram m_diagram;
		bool m_exiting;

		bool process_queue(){
//			m_diagram.process_queue();
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

