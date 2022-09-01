#pragma once
#include <gtkmm.h>
#include <map>
#include "../cpu_data.h"

class Component: public Glib::ObjectBase {
  public:
	virtual void exiting(){}
};

class Sim16F : public Gtk::Window
{
  public:

	Sim16F(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
//	Sim16F(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade, bool is_glad);

	virtual ~Sim16F();

	void init_cpu(CPU_DATA &cpu);

  protected:
	typedef std::map<Glib::ustring, Component * > part;
	typedef part::iterator each_part;
	part m_parts;

	bool delete_event(GdkEventAny *e){
		for (each_part p=m_parts.begin(); p != m_parts.end(); ++p)
			p->second->exiting();
		return false;
	}

	Glib::RefPtr<Gtk::Builder> m_refGlade;
	CPU_DATA *m_cpu;
};

void run_application(CPU_DATA &cpu);
