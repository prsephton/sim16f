/*
 * nodes.h
 *
 *  Created on: 30 Oct 2022
 *      Author: paul
 */
#pragma once
#include "device_base.h"
#include "../utils/matrix.h"

struct MeshItem {
	Device *dev;
	bool reversed;
	double Itotal = 0;
	const std::string id();
	double R();
	double V(bool calc = false);
	bool is_voltage();

	MeshItem(Device *d, bool reversed=false);
};

struct Mesh {
	std::vector<MeshItem> items;
	std::set<MeshItem *> shared(const Mesh &other);

	double amps = 0;
	void I(double a_amps);
	void add(Device *d, bool reversed=false);
	bool contains(Device *d);
	bool reversed(Device *d);
};

struct Connection_Data {
	int m_debug = 0;
	std::map<Device *, SmartPtr<Node>> targets;
	std::map<Device *, SmartPtr<Node>> all_nodes;
	std::set<Device *> loop_start;
	std::set<Device *> loop_term;
	std::deque<Device *> devicelist;
	std::map<Device *, double> amps;
	std::vector<SmartPtr<Mesh>> meshes;
};

class Nexus {

	void nexus_targets(Device *D) {
		if (sources.count(D)) return;
		sources.insert(D);
		for (auto S: D->targets())
			nexus_sources(S);
	}

	void nexus_sources(Device *D) {
		if (targets.count(D)) return;
		targets.insert(D);
		for (auto T: D->sources())
			nexus_targets(T);
	}

  public:
	std::set<Device *> sources={};
	std::set<Device *> targets={};

	const std::string name() {
		if (sources.size() == 0 and targets.size() == 0) return "empty";
		std::string l_name;
		std::vector<Device *>s(sources.begin(), sources.end());
		std::vector<Device *>t(targets.begin(), targets.end());
		std::sort(s.begin(), s.end());
		std::sort(t.begin(), t.end());
		l_name = "S";
		for (auto item: s) l_name = l_name + ":" + as_text(item);
		l_name += "T";
		for (auto item: s) l_name = l_name + ":" + as_text(item);
		return l_name;
	}

	Nexus() {}
	Nexus(const Nexus &other) {
		sources = other.sources;
		targets = other.targets;
	}

	Nexus(Device *D, bool input) {
		if (input)
			nexus_sources(D);
		else
			nexus_targets(D);
	}
};


class NexusMap {
	std::map<std::string, Nexus> m_map;

  protected:
	void build_map(Device *D) {
		auto s = Nexus(D, true);
		auto l_name = s.name();
		if (m_map.find(l_name) == m_map.end()) {
			m_map[l_name] = s;
			for (auto item: s.sources)
				build_map(item);
		}

		auto t = Nexus(D, false);
		l_name = t.name();
		if (m_map.find(l_name) == m_map.end()) {
			m_map[l_name] = t;
			for (auto item: t.targets)
				build_map(item);
		}
	}

  public:
	NexusMap(Device *D) {
		build_map(D);
	}

	const std::map<std::string, Nexus> &map() const { return m_map; }
};


class Connection_Node: public Node {
//      Make a 'node' containing slots for each of the source connections.  Note
//  that a source connection does not decide the direction of the current flow,
//  merely to which end of a bidirectional device the connection is made.
//      Each slot describes a source and destination component, and all
//  components without exception, define a resistance value.
//      Thus, a node lies smack between a set of sources and destinations, and
//  all of these slots share a single electrical connection.
//      For source connections, there is a known voltage and resistance value,
//  and for the destination components we determine an effective
//  resistance value (including the destination components resistance).
//      This allows us to determine a voltage at the node, which is the same output
//  voltage for all source components as defined by the slots.
//      We represent the node voltage value as a "voltage drop" defined in each of
//  the source components.

	Device *m_current;
	SmartPtr<Connection_Node> m_parent;
	SmartPtr<Connection_Data> m_cdata;

	std::vector<Device *> m_sources;
	std::vector<Device *> m_targets;

	int debug() { return m_cdata->m_debug; }
	void debug(int a_level) { m_cdata->m_debug = a_level; }

protected:
	std::vector<Device *> sources() const { return m_sources; }
	std::vector<Device *> targets() const { return m_targets; }
	std::set<Device *>target_set() const;

	void add_device_to_list(Device *d);
	void register_node(Device *d, SmartPtr<Connection_Node> a_node);
	SmartPtr<Connection_Node> check_exists(Device *d);
	void add_loop_term(Device *d);
	void add_loop_start(Device *d);
	Node *get_parent();
	Device *device() { return m_current; }

	void set_node_nexus();
	// build a node map recursively
	//   produces m_devicelist, m_loop_start and m_loop_term in the first node
	void get_sources(SmartPtr<Connection_Node> a_node);
	void get_targets(SmartPtr<Connection_Node> a_node);
	bool add_shared(Mesh &mesh, Connection_Node *start, std::set<Device *>finish);
	std::vector<Device *> shortest_path(std::vector<Device *>a_targets);
	int find_shortest_path(Device *dev);


	void show_meshes();
	void build_matrices(Matrix &m, Matrix &v);
	void calculate_I(Matrix &m, Matrix &v, double D);
	void add_mesh_totals();
	void solve_meshes();

public:
	Connection_Node(Device *d, SmartPtr<Connection_Data>cdata, bool getting_targets=true);
	Connection_Node(Device *current, Node *parent = NULL);
	virtual ~Connection_Node() {}

	// produces m_meshes, combines and solves.   Then updates voltage drops.
	void process_model();
};

