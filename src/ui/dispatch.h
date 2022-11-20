#pragma once
#include <gtkmm.h>
#include <string>
#include <map>
#include "application.h"
#include "../utils/smart_ptr.h"

class Dispatcher {
	typedef std::map<std::string, SmartPtr<Glib::Dispatcher>> DISPATCH;
	static std::map<Component *, DISPATCH> m_dispatcher;

	static DISPATCH &find(Component *c, const std::string &a_name) {
		if (m_dispatcher.find(c)==m_dispatcher.end())
			throw(std::string("Drawing area for dispatcher: [")+a_name+"] not found!");
		auto &dispatch = m_dispatcher[c];

		if (dispatch.find(a_name)==dispatch.end())
			throw(std::string("Dispatcher by name: [")+a_name+"] not found!");
		return dispatch;
	}

public:
	Dispatcher(){}
	Dispatcher(Component *c, const std::string &a_name) {
		if (m_dispatcher.find(c)==m_dispatcher.end())
			m_dispatcher[c][a_name] = new Glib::Dispatcher();
		else {
			auto &dispatch = m_dispatcher[c];
			if (dispatch.find(a_name) == dispatch.end())
				dispatch[a_name] = new Glib::Dispatcher();
		}
	}
	static void emit(Component *c, const std::string &a_name) {
		DISPATCH &dispatch = find(c, a_name);
		dispatch[a_name]->emit();
	}
	Glib::Dispatcher &dispatcher(Component *c, const std::string &a_name) {
		DISPATCH &dispatch = find(c, a_name);
		return *(dispatch[a_name].operator->());
	}
};
