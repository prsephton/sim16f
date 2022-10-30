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
	Connection *dev;
	double Itotal = 0;
	const std::string id();
	double R();
	double V();

	MeshItem(Connection *d);
};

struct Mesh {
	std::vector<MeshItem> items;
	std::set<Device *> shared;
	double amps = 0;
	void I(double a_amps);
	void add(Device *d);
	bool contains(Device *d);
	void add_shared(Device *d);
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

	bool m_debug = false;

	std::set<Slot *> m_slots;
	Device *m_current;
	Connection_Node *m_parent;

	std::set<Device *> m_sources;
	std::set<Device *> m_destinations;
	std::map<Device *, SmartPtr<Connection_Node>> m_targets;
	std::map<Device *, SmartPtr<Connection_Node>> m_all_nodes;
	std::set<Device *> m_loop_start;
	std::set<Device *> m_loop_term;
	std::deque<Device *> m_devicelist;
	std::map<Device *, double> m_amps;
	std::vector<SmartPtr<Mesh>> m_meshes;

protected:
	std::set<Device *> sources() { return m_sources; }
	std::set<Device *> destinations() { return m_destinations; }

	void add_device_to_list(Device *d);
	void register_node(Device *d, SmartPtr<Connection_Node> a_node);
	SmartPtr<Connection_Node> check_exists(Device *d);
	void add_loop_term(Device *d);
	void add_loop_start(Device *d);
	Node *get_parent();

	// build a node map recursively
	//   produces m_devicelist, m_loop_start and m_loop_term in the first node
	void get_targets();
	void add_shared(Mesh *prev, Mesh &mesh, Connection_Node *start, const std::set<Device *>&finish);

	void show_meshes();
	void build_matrices(Matrix &m, Matrix &v);
	void calculate_I(Matrix &m, Matrix &v, double D);
	void add_mesh_totals();
	void solve_meshes();

public:
	Connection_Node(std::set<Slot *> &slots, Device *current, Node *parent = NULL);
	virtual ~Connection_Node() {}

	// produces m_meshes, combines and solves.   Then updates voltage drops.
	void process_model();
};

