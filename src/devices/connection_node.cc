/*
 * nodes.cc
 *
 *  Created on: 30 Oct 2022
 *      Author: paul
 */
#include "connection_node.h"

const std::string MeshItem::id() { return as_text(this); }
double MeshItem::R() { return dev->R(); }
double MeshItem::V() {
	if (is_voltage) return dev->rd(false);
	auto v = dynamic_cast<Voltage *>(dev);
	if (v) return v->rd(false);
	return 0;
}

MeshItem::MeshItem(Device *d): dev(d) {}

void Mesh::I(double a_amps) {
	amps = a_amps;
	for (auto &item: items) {
	  if (shared.find(item.dev) == shared.end())
		  item.Itotal += amps;
	  else
		  item.Itotal -= amps;
	}
}

bool Mesh::contains(Device *d) {
	for (auto &item: items)
		if (item.dev == d)
			return true;
	return false;
}

void Mesh::add(Device *d) {
	items.push_back(MeshItem(d));
}

void Mesh::add_shared(Device *d) {
  shared.insert(d);
}

void Connection_Node::add_device_to_list(Device *d) {
	if (m_parent)
		m_parent->add_device_to_list(d);
	else {
		if (!d)
			return;
		for (auto dev: m_devicelist)
			if (dev == d) return;
		m_devicelist.push_back(d);
	}
}

void Connection_Node::register_node(Device *d, SmartPtr<Connection_Node> a_node) {
	if (m_parent)
		m_parent->register_node(d, a_node);
	else {
		m_all_nodes[d] = a_node;
	}
}

SmartPtr<Connection_Node> Connection_Node::check_exists(Device *d) {
	if (m_parent) return m_parent->check_exists(d);
	if (m_all_nodes.find(d) != m_all_nodes.end()) {
		if (m_debug) std::cout << "      - loop found: " << d->name() << std::endl;
		return m_all_nodes[d];
	}
	return NULL;
}

void Connection_Node::add_loop_term(Device *d) {
	if (m_parent) {
		m_parent->add_loop_term(d);
		return;
	}
	m_loop_term.insert(d);
}

void Connection_Node::add_loop_start(Device *d) {
	if (m_parent) {
		m_parent->add_loop_start(d);
		return;
	}
	m_loop_start.insert(d);
}

Node *Connection_Node::get_parent() {
	return m_parent;
}


Connection_Node::Connection_Node(std::set<Slot *> &slots, Device *current, Node *parent):
			m_slots(slots), m_current(current),
			m_parent(dynamic_cast<Connection_Node *>(parent)) {
	bool added = true;
	while (added) {
		added = false;
		for (auto s: m_slots) {
			auto c = dynamic_cast<Terminal *>(s->dev);
			if (c) {   // sibling nodes
				added = c->add_slots(m_slots) or added;
			}
		}
	}
	for (auto slot: m_slots) {
		if (m_sources.find(slot->connection) == m_sources.end())
			m_sources.insert(slot->connection);
		if (m_destinations.find(slot->dev) == m_destinations.end())
			m_destinations.insert(slot->dev);
	}
	get_targets();
};

// build a node map recursively
//   produces m_devicelist, m_loop_start and m_loop_term in the first node
void Connection_Node::get_targets() {

	if (m_parent==NULL) {
		for (auto d: m_sources) {
			SmartPtr<Connection_Node> src_pointer = this;
			src_pointer.incRef();    // do not manage!
			add_device_to_list(d);
			register_node(d, src_pointer);
		}
	}

	bool first = true;
	for (auto d: m_destinations)
		if (m_targets.find(d) == m_targets.end()) {
			if (!first) add_loop_start(d);
			SmartPtr<Connection_Node> t = check_exists(d);   // a loop found?
			if (t == (Connection_Node *)NULL) {
				add_device_to_list(d);
				t = d->get_targets(this);
				register_node(d, t);
			} else {   // destination is already visited, so loop
				add_loop_term(m_current);
			}
			if (t != (Connection_Node *)NULL)
				m_targets[d] = t;
			first = false;
		}

}

// List all mesh configurations
void Connection_Node::show_meshes() {
	for (auto &mesh: m_meshes) {
		std::cout << "Mesh\n  Items\n";
		for (auto & item: mesh->items) {
			std::cout << "    " << item.dev->name() << std::endl;
		}
		std::cout << "  Shared\n";
		for (auto &d: mesh->shared) {
			std::cout << "    " << d->name() << std::endl;
		}
		std::cout << std::flush;
	}
}

// Build a matrix M describing interconnections and resistances
// for all connected components.  Also build V containing the
// values for all voltage sources.
void Connection_Node::build_matrices(Matrix &m, Matrix &v) {
	for (size_t i=0; i< m_meshes.size(); ++i) {
		auto &i_mesh = *m_meshes[i];
		for (size_t j=0; j< m_meshes.size(); ++j) {
			if (i == j) {
				double rtotal=0, vtotal = 0;
				for (auto &item: i_mesh.items) {
					rtotal += item.R();
					vtotal += i?-item.V():item.V();  // V negative except for 1st mesh
				}
				m(i, i) = rtotal;
				v(0, i) = vtotal;
			} else {
				auto &j_mesh = *m_meshes[j];
				double rtotal=0;
				for (auto &shared_item: j_mesh.shared) {
					for (auto &item: i_mesh.items) {
						if (item.dev == shared_item) {
							rtotal -= item.R();
						}
					}
				}
				if (rtotal) {
					m(i, j) = rtotal;
					m(j, i) = rtotal;
				}
			}
		}
	}
}

// Calculate current flowing through each mesh.
void Connection_Node::calculate_I(Matrix &m, Matrix &v, double D) {
	for (size_t i=0; i< m_meshes.size(); ++i) {
		auto &i_mesh = *m_meshes[i];
		Matrix n(m);
		for (size_t j=0; j< m_meshes.size(); ++j) n(i,j) = v(0,j);
		if (m_debug) {
			std::cout << "matrix " << i << ": \n";
			n.view();
		}
		double Di = n.determinant();
		i_mesh.I(Di / D);
		if (m_debug) {
			std::cout << "D" << i << " is " << Di;
			std::cout << ", I" << i << " is D" << i << "/D";
			std::cout << " = " << Di << "/" << D << " = " << i_mesh.amps;
			std::cout << std::endl;
		}
	}
}

// Having a value for the current through a mesh, we may add
// currents for shared components, providing current values
// for each device.
void Connection_Node::add_mesh_totals() {
	for (size_t i=0; i< m_meshes.size(); ++i) {
		auto &i_mesh = *m_meshes[i];
		for (auto &item: i_mesh.items)
			if (m_amps.find(item.dev) == m_amps.end())
				m_amps[item.dev] = item.Itotal;
			else
				m_amps[item.dev] += item.Itotal;
	}
}

// Create a matrix M, using the available meshes. Then find it's determinant, D.
// We also find a matrix V having the voltages sources for each mesh.
// Replacing V in M by column in turn, we can calculate D0..Dn.  We can then
// calculate I[n] = D[n] / D, where I[n] is the current through mesh n.
void Connection_Node::solve_meshes() {
	if (m_debug) show_meshes();

	Matrix m(m_meshes.size());
	Matrix v(1, m_meshes.size());
	build_matrices(m, v);

	double D = 1;
//	if (v.cols() > 1) {
		D = m.determinant();
		if (m_debug) {
			std::cout << "M is \n";
			m.view();
			std::cout << "V is \n";
			v.view();
			std::cout << "D is " << D << std::endl;
		}
		if (D == 0) return;   //nothing to be done
//	}
	calculate_I(m, v, D);
	add_mesh_totals();

	for (auto dev: m_devicelist) {
		dev->set_vdrop(-m_amps[dev] * dev->R());
	}
}

// Add nodes shared between meshes to the list.  We should end up with each mesh
// describing the components in the mesh, and a list of components shared
// with the previous mesh.  This may not describe all possible circuits, but should
// come close.
void Connection_Node::add_shared(Mesh *prev, Mesh &mesh, Connection_Node *start, const std::set<Device *>&finish) {
	if (!start) return;
	for (auto d: start->destinations()) {
		if (finish.find(d) != finish.end()) continue;
		if (not prev->contains(d)) continue;
		if (mesh.contains(d)) continue;
		if (prev->shared.find(d) != prev->shared.end())
			continue;
		if (mesh.shared.find(d) != mesh.shared.end())
			continue;
		mesh.add(d);
		mesh.add_shared(d);
		add_shared(prev, mesh, m_targets[d].operator ->(), finish);
	}
}

//  only uses the first node, produces m_meshes which represents the
// set of interconnections.
void Connection_Node::process_model() {
	m_meshes.clear();
	Mesh *mesh = new Mesh();
	Mesh *prev = NULL;
	m_meshes.push_back(mesh);
	for (auto &dev: m_devicelist) {
		if (m_loop_start.find(dev) != m_loop_start.end()) {
			prev = mesh;
			mesh = new Mesh();
			m_meshes.push_back(mesh);
		}
		mesh->add(dev);
		if (prev && m_loop_term.find(dev) != m_loop_term.end()) {   // last device in the mesh
			auto first = m_all_nodes[mesh->items[0].dev];
			auto last = m_all_nodes[dev];
			auto start = dynamic_cast<Connection_Node *>(first->get_parent());
			auto finish = last->destinations();
			add_shared(prev, *mesh, start, finish);
		}
	}
	if (m_meshes.size())
		if (m_meshes[0]->items.size())
			m_meshes[0]->items[0].is_voltage = true;

	solve_meshes();
}



