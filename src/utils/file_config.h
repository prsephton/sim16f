//______________________________________________________________________________________________
//  Behold!  A Wheel!
//    NIHS :-)
//
//    but seriously, it seems less trouble to DIY than to find one that does what you want.
//    libconfig (https://hyperrealm.github.io/libconfig/) looks _really_ nice though, and
//    was definitely an option.  I'll use that next time.
//
//  Please refer to config.cc test section for usage details
//
#include <map>
#include <string>
#include <sstream>
#include "smart_ptr.h"

class FileConfig {
  public:
	FileConfig(const std::string &fn="");
	void flush(const std::string &a_filename="");

	bool exists(const std::string &name);
	double get_float(const std::string &name);
	int get_int(const std::string &name);
	std::string get_text(const std::string &name);
	void set_text(const std::string &name, const std::string &data);
	void set_float(const std::string &name, double data);
	void set_int(const std::string &name, int data);

  private:
	bool debug = false;

	struct ITEM {
		virtual ~ITEM() {}
		virtual std::string asText() const = 0;
		template<class T> std::string as_text(T value) const {
			std::ostringstream s; s << value;
			return s.str();
		}
	};
	struct INT : public ITEM {
		int data;
		INT(int i=0) : data(i) {}
		virtual std::string asText() const { return as_text<int>(data); }
	};
	struct FLOAT : public ITEM {
		double data;
		FLOAT(double f=0) : data(f) {}
		virtual std::string asText() const { return as_text<double>(data); }
	};
	struct TEXT : public ITEM {
		std::string data;
		TEXT(const std::string &s=""): data(s) {}
		virtual std::string asText() const { return data; }
	};

	std::string m_filename;
	typedef std::map<std::string, SmartPtr<ITEM>> ITEM_MAP;

	std::map<std::string, ITEM_MAP> m_sections;
	std::string section_name;

	bool parse_token(const std::string &line, std::string &token);
	bool parse_digits(const std::string &line, bool with_sign=true);
	bool parse_fixed_point(const std::string &line);
	bool parse_numeric(const std::string &line, FLOAT &f);
	bool parse_text(const std::string &line, std::string &t);
	bool parse_int(const std::string &line, INT &i);
	bool parse_float(const std::string &line, FLOAT &f);
	bool parse_item(const std::string &line);
	bool parse_section(const std::string &line);
	bool parse_line(const std::string line);

	std::pair<std::string, std::string> section_id(const std::string &a_name);
	const ITEM &get(const std::string &section, const std::string &id) const;
	void flush_section(const std::string &section, std::ofstream &f);

};
