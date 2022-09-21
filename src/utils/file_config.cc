#include "file_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

bool FileConfig::parse_token(const std::string &line, std::string &token) {
	size_t first = line.find_first_not_of(" \t");
	if (first==std::string::npos) return false;
	std::string ss = line.substr(first);
	size_t last = ss.find_first_of(" \t\n\r");
	if (last != std::string::npos) ss = ss.substr(0,last);
	if (!ss.length()) return false;
	token = ss;
	return true;
}

bool FileConfig::parse_digits(const std::string &line, bool with_sign) {
	std::string ss;
	if (with_sign and line.find_first_of("+-") == 0)
		ss = line.substr(1);
	else
		ss = line;

	size_t end = ss.find_first_not_of("0123456789");
	if (end == std::string::npos) return true;
	return false;
}

bool FileConfig::parse_fixed_point(const std::string &line) {
	size_t dec = line.find(".");
	if (dec == std::string::npos) return parse_digits(line);
	if (!parse_digits(line.substr(0,dec))) return false;
	if (!parse_digits(line.substr(dec+1), false)) return false;
	return true;
}

bool FileConfig::parse_numeric(const std::string &line, FLOAT &f) {
	char *p;
	f.data = strtod(line.c_str(), &p);
	return (p && *p == 0);
}

bool FileConfig::parse_int(const std::string &line, INT &i) {
	std::string token;
	if (!parse_token(line, token)) return false;
	if (!parse_digits(token)) return false;
	char *p;
	i.data = strtol(token.c_str(), &p, 10);
	if (debug)  std::cout << " is an int" << std::endl;
	return (p && *p == 0);
}

bool FileConfig::parse_float(const std::string &line, FLOAT &f) {
	std::string token;
	if (!parse_token(line, token)) return false;

	size_t e = token.find_first_of("eE");
	if (e==std::string::npos) {
		if (!parse_fixed_point(token)) return false;
		if (!parse_numeric(token, f)) return false;
	} else {
		if (!parse_fixed_point(token.substr(0, e))) return false;
		if (!parse_fixed_point(token.substr(e+1))) return false;
		if (!parse_numeric(line, f)) return false;
	}
	if (debug) std::cout << " is a float " << std::endl;
	return true;
}

bool FileConfig::parse_text(const std::string &line, std::string &t) {
	size_t start = line.find_first_not_of(" \t");
	if (start == std::string::npos) {
		t = ""; return true;
	}
	if (line[start] == '"') {
		size_t end = line.find_last_of("\"");
		t = line.substr(start+1, end-start-1);
		return true;
	}
	t = line.substr(start);
	return true;
}

bool FileConfig::parse_item(const std::string &line) {
	size_t first = line.find_first_of("=");
	if (first == std::string::npos) return false;
	std::string iname;
	if (!parse_token(line.substr(0, first), iname)) return false;
	if (debug) std::cout << "[" << section_name << "." << iname << "]";

	std::string ss = line.substr(first+1);
	FLOAT f;
	INT i;
	if (parse_int(ss, i))
		m_sections[section_name][iname] = new INT(i);
	else if (parse_float(ss, f))
		m_sections[section_name][iname] = new FLOAT(f);
	else if (parse_text(ss, ss)) {
		m_sections[section_name][iname] = new TEXT(ss);
		if (debug) std::cout << " is text " << std::endl;
	}
	return true;
}

bool FileConfig::parse_section(const std::string &line) {
	size_t last = line.find_first_of("]");
	if (last==std::string::npos)
		return false;
	return parse_token(line.substr(0, last), section_name);
}

bool FileConfig::parse_line(const std::string line) {
	size_t first = line.find_first_not_of(" \t");
	if (first==std::string::npos)
		return false;
	if (line[first] == '#')    // a comment
		return true;
	if (line[first] == ';')    // a comment
		return true;
	if (line[first] == '[')
		return parse_section(line.substr(first+1));
	return parse_item(line.substr(first));
	return true;
}

const FileConfig::ITEM &FileConfig::get(const std::string &section, const std::string &id) const {
	auto l_sect = m_sections.find(section);
	if (l_sect == m_sections.end())
		throw(std::string("Section [") + section + "] not found in configuration file");
	auto l_item = l_sect->second.find(id);
	if (l_item == l_sect->second.end())
		throw(std::string("Item [") + id +"] not found in section [" + section + "]");
	return *(l_item->second);
}

std::pair<std::string, std::string> FileConfig::section_id(const std::string &a_name) {
	std::pair<std::string, std::string> sid;
	size_t sep = a_name.find(".");
	if (sep == std::string::npos) {
		sid.first = ""; sid.second = a_name;
	} else {
		sid.first = a_name.substr(0, sep);
		sid.second = a_name.substr(sep+1);
	}
	return sid;
}

FileConfig::FileConfig(const std::string &fn): m_filename(fn) {
	std::ifstream f(fn);
	for (std::string line; std::getline(f, line);) {
		if (line.length() && !parse_line(line))
			std::cout << "Parse failure: [" << line << "]" << std::endl;
	}
}

void FileConfig::flush_section(const std::string &section, std::ofstream &f) {
	if (section.length()) {
		f << "[" << section << "]" << std::endl;
		for (auto item: m_sections[section])
			f << "  " << item.first << " = " << item.second->asText() << std::endl;
	} else {
		for (auto item: m_sections[section])
			f << item.first << " = " << item.second->asText() << std::endl;
	}
	f << std::endl;
}

void FileConfig::flush(const std::string &a_filename) {
	if (a_filename.length()) m_filename = a_filename;
	if (!m_filename.length()) {
		throw std::string("Cannot flush configuration to a file name of zero length");
	}
	std::ofstream f(m_filename);
	flush_section("", f);
	for (auto section: m_sections) {
		if (section.first.length())
			flush_section(section.first, f);
	}
}

bool FileConfig::exists(const std::string &name) {
	auto sid = section_id(name);
	try {
		get(sid.first, sid.second);
		return true;
	} catch (...) {
		return false;
	}
}

double FileConfig::get_float(const std::string &name) {
	auto sid = section_id(name);
	const ITEM &i = get(sid.first, sid.second);
	try {
		return dynamic_cast<const FLOAT &>(i).data;
	} catch (...) {
		try {
			return dynamic_cast<const INT &>(i).data;
		} catch (...) {
			throw(sid.first + "." + sid.second + " is not a float");
		}
	}
}

int FileConfig::get_int(const std::string &name) {
	auto sid = section_id(name);
	const ITEM &i = get(sid.first, sid.second);
	try {
		return dynamic_cast<const INT &>(i).data;
	} catch (...) {
		throw(sid.first + "." + sid.second + " is not an integer");
	}
}

std::string FileConfig::get_text(const std::string &name) {
	auto sid = section_id(name);
	const ITEM &i = get(sid.first, sid.second);
	try {
		return i.asText();
	} catch (...) {
		throw(sid.first + "." + sid.second + " is not text");
	}
}

void FileConfig::set_text(const std::string &name, const std::string &data) {
	auto sid = section_id(name);
	m_sections[sid.first][sid.second] = new TEXT(data);
}

void FileConfig::set_float(const std::string &name, double data) {
	auto sid = section_id(name);
	m_sections[sid.first][sid.second] = new FLOAT(data);
}

void FileConfig::set_int(const std::string &name, int data) {
	auto sid = section_id(name);
	m_sections[sid.first][sid.second] = new INT(data);
}

#ifdef __TEST__
#include <cassert>
#include <cmath>
int main(int a_argc, char *a_argv[]) {
	FileConfig c;
	c.set_int("q", 1);
	c.set_int("a", 3);
	c.set_text("section1.a", "line 1");
	c.set_text("section1.b", "line 2 3");
	c.set_text("section1.c", "line 4");
	c.set_float("section2.q", 1.23);
	c.flush("cfg.cfg");
	FileConfig cfg("cfg.cfg");
	try {
		assert(cfg.get_int("q") == 1);
		assert(cfg.get_int("a") == 3);
		assert(cfg.get_text("section1.a") == "line 1");
		assert(cfg.get_text("section1.b") == "line 2 3");
		assert(cfg.get_text("section1.c") == "line 4");
		assert(fabs(cfg.get_float("section2.q")) - 1.23 < 1.0e-6);
		assert(cfg.get_text("section2.q") == "1.23");
		assert(cfg.get_float("q") == 1.0);
		cfg.flush("cfg.cfg");
	} catch (std::string &s) {
		std::cout << "Error: " << s << std::endl;
	}
}
#endif
