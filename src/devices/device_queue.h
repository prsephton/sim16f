#pragma once
#include "device_base.h"


//___________________________________________________________________________________
// A base class for device events that can be queued.
class QueueableEvent {
  public:
	virtual ~QueueableEvent() {}
	virtual void fire_event(bool debug=false) = 0;
	virtual bool compare(Device *d) = 0;
	virtual Device *device() = 0;
	virtual const std::string &name() = 0;
};

//___________________________________________________________________________________
// We can have a single event queue for all devices, and process events
// in sequence.  Events themselves are derived from a base class.
class DeviceEventQueue {
	static std::queue< SmartPtr<QueueableEvent> >events;
	static std::mutex mtx;
	static LockUI m_ui_lock;
	static bool m_paused;

  public:
	static bool debug;

	void queue_event(QueueableEvent *event) {
		mtx.lock();
		events.push(event);
		mtx.unlock();
	}

	static void pause(bool paused) {
		if (paused) {
			m_paused = true;
		}
		else if (m_paused) {
			m_paused = false;
		}
	}


	void clear() {
		mtx.lock();
		while (!events.empty()) {
			events.pop();
		}
		mtx.unlock();
	}

	int size() {
		return events.size();
	}

	void remove_events_for(Device *d) {
		mtx.lock();
		static std::queue< SmartPtr<QueueableEvent> >tmp;
		while (!events.empty()) {
			auto event = events.front(); events.pop();
			if (!event->compare(d)) tmp.push(event);
		}
		while (!tmp.empty()) {
			auto ev = tmp.front(); tmp.pop(); events.push(ev);
		}

		mtx.unlock();
	}

	inline SmartPtr<QueueableEvent> process_single() {
		if (events.empty())
			return NULL;
		else {
			mtx.lock();
			if (events.empty()) {
				mtx.unlock();
				return NULL;
			}
			auto event = events.front(); events.pop();
			while (not events.empty() && event == events.front()) events.pop();
			mtx.unlock();
//			std::cout << "dev acquired lock" << std::endl;
			event->fire_event(debug);
//			std::cout << "dev releasing lock" << std::endl;
			return event;
		}
	}

	template<typename DeviceClass> SmartPtr<QueueableEvent> wait(const std::string &name, unsigned long timeout_us=100000) {
		time_stamp expire = current_time_us() + std::chrono::duration<unsigned long, std::micro>(timeout_us);
		while (current_time_us() < expire) {
			auto event = process_single();
			if (event == SmartPtr<QueueableEvent>(NULL))
				sleep_for_us(10);
			else if (event->name() == name) {
				std::cout << name << ": Try dynamic cast" << std::endl;
				DeviceClass *w = dynamic_cast<DeviceClass *>(event->device());
				if (w) return event;
			}
		}
		return NULL;
	}


	void process_events() {
		int n;
		try {
			for (n=0; n<1000; ++n)
				if (not process_single()) break;
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		} catch (...) {}

		if (n == 1000) {
			std::cout << "Possible event loop detected" << std::endl;
//			debug = true;
		}
	}
};

//___________________________________________________________________________________
// A pub-sub for device events.  For example, when the clock triggers, we can put
// one of these events in a queue.
// There is only one list of subscribers per device.  The CPU implementation will
// traverse all known devices, and fire any outstanding events.  Whenever we change
// a connection, we also queue an event for that change, and process the queue
// directly after, effectively rippling that change through the circuit.
//
//  For example, we can do::
//    class abc {
//        void callback_for_device(Device &d, const std::string &name) {
//            ... do stoff
//        }
//    }
//
//    abc() {
//       Device mydev("this is my device");
//       DeviceEvent<Device>.subscribe<abc>(this, &callback_for_device);      // generic subscription for all Device instances
//       DeviceEvent<Device>.subscribe<abc>(this, &callback_for_device, &mydev); // specific subscription just to mydev events
//    }
//
//  The other form of callback also passes data as a vector of bytes, where the data depends on what the event produces
//
//  To queue a new event, we do:
//
//     DeviceEventQueue q;
//     q.queue_event(new EventQueue<Device>(*this, "a new event happened"))
//  or having an existing device, (eg. mydev above) we can do
//     q.queue_event(new EventQueue(&mydev, "a new event happened"))

template <class T> class DeviceEvent: public QueueableEvent {
	Device           *m_device;
	std::string       m_eventname;
	std::vector<BYTE> m_data;

	typedef std::function<void(T *device, const std::string &event, const std::vector<BYTE> &data)> Callback;

	typedef std::tuple<void *, const T *, void *> KeyType;            // Object, Source

	virtual bool compare(Device *d){ return d == m_device; }

  private:
	typedef std::map<KeyType, Callback> registry;
	typedef typename DeviceEvent<T>::registry::iterator each_subscriber;
	static registry subscribers;

  public:
	DeviceEvent(T &device, const std::string &eventname):
		m_device(&device), m_eventname(eventname), m_data(std::vector<BYTE>()) {}

	DeviceEvent(T &device, const std::string &eventname, const std::vector<BYTE> &data):
		m_device(&device), m_eventname(eventname), m_data(data) {}

	virtual Device *device() { return m_device; }
	virtual const std::string &name() { return m_eventname; }

	virtual void fire_event(bool debug=false) {
		for(each_subscriber s = subscribers.begin(); s!= subscribers.end(); ++s) {
			const T *instance = std::get<1>(s->first);
			if (!instance || instance == m_device) {
				try {
					T * device = dynamic_cast<T *>(m_device);
					if (device == NULL) throw(std::string("Null device"));
					if (debug)
						std::cout << "Event: " << device->name() << ": " << m_eventname << ": data[" << m_data.size() << "]" << std::endl;
					s->second(device, m_eventname, m_data);
					if (debug)
						std::cout << "Event returned" << std::endl;
				} catch (const std::string &e) {
					std::cout << "An error occurred while processing a device event: " << e << "\n";
				} catch (std::exception &e) {
					std::cout << "Exception raised while processing a device event: " << e.what() << "\n";
				}
			}
		}
	}

	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{ob, instance, *(void **)&callback}] = std::bind(callback, ob, _1, _2);
	}

	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{ob, instance, *(void **)&callback}] = std::bind(callback, ob, _1, _2, _3);
	}

	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event), T *instance = NULL) {
		auto key = KeyType{ob, instance, *(void **)&callback};
		if (subscribers.find(key) != subscribers.end()) {
			subscribers.erase(key);
		}
	}

	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		auto key = KeyType{ob, instance, *(void **)&callback};
		if (subscribers.find(key) != subscribers.end()) {
			subscribers.erase(key);
		}
	}
};

