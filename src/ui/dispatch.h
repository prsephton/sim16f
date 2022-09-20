#pragma once
#include <gtkmm.h>
#include <string>
#include <map>
#include "../utils/smart_ptr.h"

class Dispatcher {
	static std::map<std::string, SmartPtr<Glib::Dispatcher>> m_dispatcher;
public:
	Dispatcher(){}
	Dispatcher(const std::string &a_name) {
		if (m_dispatcher.find(a_name)==m_dispatcher.end())
			m_dispatcher[a_name] = new Glib::Dispatcher();
	}
	Glib::Dispatcher &dispatcher(const std::string &a_name) {
		if (m_dispatcher.find(a_name)==m_dispatcher.end())
			throw(std::string("Dispatcher by name: [")+a_name+"] not found!");
		return *(m_dispatcher[a_name].operator->());
	}
};
