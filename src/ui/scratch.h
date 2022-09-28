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
		virtual void add_diagram(CairoDrawingBase *a_drawing)=0;

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

			std::map<std::string, SmartPtr<Device> > m_devices;

			Glib::RefPtr<Gtk::Menu> rails;
			Glib::RefPtr<Gtk::Menu> gates;
			Glib::RefPtr<Gtk::Menu> functions;
			Glib::RefPtr<Gtk::Menu> physical;
			Glib::RefPtr<Gtk::Menu> analog;
			Glib::RefPtr<Gtk::Menu> porta;
			Glib::RefPtr<Gtk::Menu> portb;
			int m_dev = 0;

			const std::string asText(void *address) const {
				std::ostringstream s;
				s << std::hex << address;
				return s.str();
			}

			//					auto dia = new GenericDiagram(0, 0, scratch->area());
			//					dia->add(GenericDiagram::text(0,0,"test"));

			void on_menu_rails() {
				auto item = rails->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Vdd") {
					Voltage *l_dev = new Voltage(5, "Vdd");
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new VddDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Vss") {
					Ground *l_dev = new Ground();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new VssDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Input") {
					Input *l_dev = new Input();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new InputDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Connection") {
					Terminal *l_dev = new Terminal();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new IODiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Output") {
					Output *l_dev = new Output();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new OutputDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Pull-Up") {
					PullUp *l_dev = new PullUp(5, "5v");
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new PullUpDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Terminal") {
					Terminal *l_dev = new Terminal();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new TerminalDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				}
				std::cout << "A menu option [" << l_label << "] was selected" << std::endl;
			}


			void on_menu_analog() {
				auto item = analog->get_active();
				const Glib::ustring l_label = item->get_label();
				if (l_label == "Resistor") {
					Terminal *l_dev = new Terminal();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new ResistorDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
				} else if (l_label == "Capacitor") {
					Capacitor *l_dev = new Capacitor();
					m_devices[asText(l_dev)] = l_dev;
					auto dia = new CapacitorDiagram(scratch->area(), *l_dev, 0, 0);
					scratch->add_diagram(dia);
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
				connect_children(analog.operator ->(), sigc::mem_fun(*this, &Menu::on_menu_analog));

			}
		};

		Menu m_menu;

	  public:

		void process_queue(){
			sleep_for_us(100);
		}

		virtual const Glib::RefPtr<Gtk::Builder>& glade() { return m_refGlade; }
		virtual Glib::RefPtr<Gtk::DrawingArea> area() { return m_area; }
		virtual void add_diagram(CairoDrawingBase *a_drawing) {
			std::string cname = "Component." + int_to_string(m_comp_id++);
			m_components[cname] = a_drawing;
			a_drawing->interactive(true);
			a_drawing->position(Point(50,50));
		}

		ScratchDiagram(CPU_DATA &a_cpu, const Glib::RefPtr<Gtk::Builder>& a_refGlade):
			CairoDrawing(Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(a_refGlade->get_object("dwg_scratch"))),
			m_cpu(a_cpu), m_refGlade(a_refGlade), m_menu(this)
		{
			pix_extents(800,600);
			interactive(true);
		}
		virtual ~ScratchDiagram() {}

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

