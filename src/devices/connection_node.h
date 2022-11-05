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
	double Itotal = 0;
	const std::string id();
	double R();
	double V();
	bool is_voltage();

	MeshItem(Device *d);
};

struct Mesh {
	std::vector<MeshItem> items;
	std::set<MeshItem *> shared(const Mesh &other);

	double amps = 0;
	void I(double a_amps);
	void add(Device *d);
	bool contains(Device *d);
};

struct Connection_Data {
	std::map<Device *, SmartPtr<Node>> targets;
	std::map<Device *, SmartPtr<Node>> all_nodes;
	std::set<Device *> loop_start;
	std::set<Device *> loop_term;
	std::deque<Device *> devicelist;
	std::map<Device *, double> amps;
	std::vector<SmartPtr<Mesh>> meshes;
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

	int m_debug = 0;

	Device *m_current;
	SmartPtr<Connection_Node> m_parent;
	SmartPtr<Connection_Data> m_cdata;

	std::vector<Device *> m_sources;
	std::vector<Device *> m_targets;

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

	// build a node map recursively
	//   produces m_devicelist, m_loop_start and m_loop_term in the first node
	void get_sources(SmartPtr<Connection_Node> a_node);
	void get_targets(SmartPtr<Connection_Node> a_node);
	bool add_shared(Mesh *prev, Mesh &mesh, Connection_Node *start, const std::set<Device *>&finish);
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

