/*
 * scratch_writer.h
 *
 *  Created on: 02 Nov 2022
 *      Author: paul
 *
 *      Loads and Save to & from an FML formatted file.  XML because it easily stores hierarchical info, and
 *      allows easy traversal through document elements.
 *
 *      ScratchXML class may be instantiated through injection of a visitor which may be queried to produce an
 *      XML document, or instantiated by reading a specified file in another constructor. In both cases, ScratchXML
 *      contains a document which represents the content of the scratch pad.
 *      The document may then either be dumped to file, or traversed to load the information into the scratch pad.
 */

#pragma once
#include <string>
#include <vector>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <cstdio>
#include "utility.h"

struct ScratchConverter {   // a visitor interface
	typedef enum { str, integer, fp, boolean } ATTR_TYPE;
	typedef std::map<std::string, std::string> INFO_MAP;

	struct ATTRIBUTE {
		std::string name;
		ATTR_TYPE type;
		struct {
			std::string str;
			int integer;
			double fp;
			bool boolean;
		} data;
	};

	virtual bool first_item() = 0;                                 // locate first element to store
	virtual bool next_item() = 0;                                  // advance to the next element
	virtual bool first_connection() = 0;                           // find first connection for the current element
	virtual bool next_connection() = 0;                            // advance to the next connection

	virtual std::string get_id() = 0;                              // retrieve identifier for this element
	virtual std::string element_class() = 0;                       // The physical, gates, porta, portb etc.
	virtual std::string element_label() = 0;                       // The kind of element, eg: "Resistor"
	virtual std::map<std::string, std::string> attributes() = 0;   // what attributes stored for this element?
	virtual ATTR_TYPE atype(const std::string &a_attr) = 0;        // what is the type for this attribute?

	virtual INFO_MAP source_info() = 0;  						   // get attributes describing connection source
	virtual INFO_MAP target_info() = 0;                            // get attributes describing connection target

	virtual void create_element(                                   // instruction to create an element in the scratch pad
			const std::string name,
			const std::string cls,
			const std::string label,
			std::vector<ATTRIBUTE> attributes) = 0;
	virtual void connect(                                          // instruction to connect one element to another
			INFO_MAP from_attrs,
			INFO_MAP to_attrs) = 0;
	virtual ~ScratchConverter() {};
};


class ScratchXML {
	xmlDocPtr doc = NULL;
	typedef unsigned char uchar;

	bool read_xml_file(const std::string &a_filename) {
		doc = xmlReadFile(a_filename.c_str(), "utf-8", XML_PARSE_NOBLANKS);
		if (!doc)
			throw std::string("Could not read the Scratch Pad document.");
		auto root = xmlDocGetRootElement(doc);
		if (!root) {
			clear_document();
			return false;
		}
		if (xmlStrcmp(root->name, (uchar *)"scratch")) {
			clear_document();
			throw std::string("This file is not a Scratch Pad document!");
		}
		return true;
	}

	void clear_document() {
		if (doc) xmlFreeDoc(doc);
		doc = NULL;
	}

	std::vector<ScratchConverter::ATTRIBUTE> attributes(xmlNodePtr node) {
		std::vector<ScratchConverter::ATTRIBUTE> attrs;
		while (node) {
			ScratchConverter::ATTRIBUTE a;
			a.name = std::string((char *)node->name);
			std::string type((char *)xmlGetProp(node, (uchar *)"type"));

			if (type == "string") {
				a.type = ScratchConverter::str;
				a.data.str = std::string((char *)xmlNodeGetContent(node));
			} else if (type == "float") {
				a.type = ScratchConverter::fp;
				a.data.fp = as_double((char *)xmlNodeGetContent(node));
			} else if (type == "integer") {
				a.type = ScratchConverter::integer;
				a.data.integer = as_int((char *)xmlNodeGetContent(node));
			} else if (type == "boolean") {
				a.type = ScratchConverter::boolean;
				a.data.boolean = std::string((char *)xmlNodeGetContent(node)) == "true";
			}
			attrs.push_back(a);
			node = node->next;
		}
		return attrs;
	}

	std::map<std::string, std::string> cinfo(xmlNodePtr node) {
		std::map<std::string, std::string> info;
		std::string id((char *)xmlGetProp(node, (uchar *)"id"));
		std::string slot_id((char *)xmlGetProp(node, (uchar *)"slot_id"));
		std::string slot_type((char *)xmlGetProp(node, (uchar *)"slot_type"));
		std::string slot_dir((char *)xmlGetProp(node, (uchar *)"slot_dir"));
		info["id"] = id;
		info["slot_id"] = slot_id;
		info["slot_type"] = slot_type;
		info["slot_dir"] = slot_dir;
		return info;
	}

  public:

	// traverse the document, calling functions in ScratchConverter to
	// load elements into the UI.
	void load(ScratchConverter &c) {
		auto root = xmlDocGetRootElement(doc);
		auto node = root->children;

		while (node) {
			if (std::string((char *)node->name) == "components") {
				auto component = node->children;
				while (component) {
					std::string cls((char *)xmlGetProp(component, (uchar *)"class"));
					std::string label((char *)xmlGetProp(component, (uchar *)"label"));
					std::string name((char *)component->name);
					c.create_element(name, cls, label, attributes(component->children));
					component = component->next;
				}
			} else if (std::string((char *)node->name) == "connections") {
				auto target = node->children;
				while (target) {
					auto target_info = cinfo(target);
					auto source = target->children;
					while (source) {
						auto source_info = cinfo(source);
						c.connect(source_info, target_info);
						source = source->next;
					}
					target = target->next;
				}
			}
			node = node->next;
		}
	}

	// Dump the current document to the file provided.
	void dump(const std::string &a_filename) {
		xmlSaveFormatFileEnc(a_filename.c_str(), doc, "utf-8", 1);
	}

	ScratchXML(const std::string &a_filename) {
		if (!read_xml_file(a_filename)) {
			throw std::string("The file [") + a_filename + "] could not be read.";
		}
	}

	ScratchXML(ScratchConverter &c) {
		doc = xmlNewDoc((uchar *)"1.0");
		auto root = xmlNewNode(NULL, (uchar *)"scratch");
		xmlDocSetRootElement(doc, root);
		auto components = xmlNewChild(root, NULL, (uchar *)"components", NULL);
		auto connections = xmlNewChild(root, NULL, (uchar *)"connections", NULL);

		if (c.first_item()) {
			while (true) {
				auto id = c.get_id();
				auto component = xmlNewChild(components, NULL, (uchar *)id.c_str(), NULL);
				xmlSetProp(component, (uchar *)"class", (uchar *)c.element_class().c_str());
				xmlSetProp(component, (uchar *)"label", (uchar *)c.element_label().c_str());
				auto attributes = c.attributes();
				for (auto attr: attributes) {
					auto e = xmlNewChild(component, NULL, (uchar *)attr.first.c_str(), (uchar *)attr.second.c_str());
					uchar *type = (uchar *)"unknown";
					switch (c.atype(attr.first)) {
					  case ScratchConverter::str:
						  type = (uchar *)"string";
						  break;
					  case ScratchConverter::integer:
						  type = (uchar *)"integer";
						  break;
					  case ScratchConverter::fp:
						  type = (uchar *)"float";
						  break;
					  case ScratchConverter::boolean:
						  type = (uchar *)"boolean";
						  break;
					}
					xmlSetProp(e, (uchar *)"type", type);
				}
				if (c.first_connection()) {
					auto target = c.target_info();
					auto to = xmlNewChild(connections, NULL, (uchar *)"target", NULL);
					for (auto inf: target)
						xmlSetProp(to, (uchar *)inf.first.c_str(), (uchar *)inf.second.c_str());

					while (true) {
						auto source = c.source_info();
						if (not source.size()) continue;

						auto from = xmlNewChild(to, NULL, (uchar *)"source", NULL);
						for (auto inf: source)
							xmlSetProp(from, (uchar *)inf.first.c_str(), (uchar *)inf.second.c_str());

						if (not c.next_connection()) break;
					}
				}
				if (not c.next_item()) break;
			}
		}
	}

	~ScratchXML() {
		clear_document();
		xmlCleanupParser();
	}
};
