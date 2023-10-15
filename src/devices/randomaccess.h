#pragma once

#include <typeindex>
#include <cstring>

#include "core_devices.h"


class RandomAccess {
  public:
	virtual ~RandomAccess(){};
	virtual unsigned int size() = 0;
	virtual int get_data(size_t idx) = 0;
	virtual void set_data(size_t idx, int value) = 0;
};


class DeviceRandomAccessAdapter: public RandomAccess {

	class EEPROM_Adapter: public RandomAccess {
		EEPROM &m_eeprom;
	  public:
		EEPROM_Adapter(EEPROM &a_eeprom):
			m_eeprom(a_eeprom) {}
		virtual unsigned int size() { return m_eeprom.size(); }
		virtual int get_data(size_t idx) { return m_eeprom.data[idx]; }
		virtual void set_data(size_t idx, int value) { m_eeprom.data[idx] = value; }
	};

	class Flash_Adapter: public RandomAccess {
		Flash &m_flash;
	  public:
		Flash_Adapter(Flash &a_flash):
			m_flash(a_flash) {}
		virtual unsigned int size() { return m_flash.size(); }
		virtual int get_data(size_t idx) { return m_flash.data[idx]; }
		virtual void set_data(size_t idx, int value) { m_flash.data[idx] = value; }
	};

	RandomAccess *m_adapted;
  public:
	DeviceRandomAccessAdapter(Device &a_device): m_adapted(0) {
		if (std::type_index(typeid(a_device)) == std::type_index(typeid(EEPROM)))
			m_adapted = new DeviceRandomAccessAdapter::EEPROM_Adapter(dynamic_cast<EEPROM &>(a_device));
		else if (std::type_index(typeid(a_device)) == std::type_index(typeid(Flash)))
			m_adapted = new DeviceRandomAccessAdapter::Flash_Adapter(dynamic_cast<Flash &>(a_device));
		else
			throw(std::string("Invalid Random Access Adapter: ")+typeid(a_device).name());
	}
	virtual ~DeviceRandomAccessAdapter() { delete m_adapted; }
	virtual unsigned int size() { return m_adapted->size(); }
	virtual int get_data(size_t idx) { return m_adapted->get_data(idx); }
	virtual void set_data(size_t idx, int data) { m_adapted->set_data(idx, data); }
};

