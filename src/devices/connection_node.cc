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
	if (is_voltage()) return dev->rd(false);
	return 0;
}

bool MeshItem::is_voltage() {
	auto v = dynamic_cast<Voltage *>(dev);
	if (v) return true;
	if (dev->sources().size() == 0) return true;
	return false;
}

MeshItem::MeshItem(Device *d, bool r): dev(d), reversed(r) {}

void Mesh::I(double a_amps) {
	amps = a_amps;
	for (auto &item: items) {
		item.Itotal -= amps;
//	  if (shared.find(item.dev) == shared.end())
//		  item.Itotal += amps;
//	  else
//		  item.Itotal -= amps;
	}
}

std::set<MeshItem *> Mesh::shared(const Mesh &other) {
	std::set<Device *>s;
	std::set<MeshItem *>t;
	for (auto &item: other.items) s.insert(item.dev);
	for (auto &item: items)
		if (s.count(item.dev)) t.insert(&item);
	return t;
}

bool Mesh::reversed(Device *d) {
	for (auto &item: items)
		if (item.dev == d)
			return item.reversed;
	return false;
}

bool Mesh::contains(Device *d) {
	for (auto &item: items)
		if (item.dev == d)
			return true;
	return false;
}

void Mesh::add(Device *d, bool r) {
	items.push_back(MeshItem(d, r));
}


std::set<Device *>Connection_Node::target_set() const {
	const auto &t = targets();
	std::set<Device *>s(t.begin(), t.end());
	return s;
}

void Connection_Node::add_device_to_list(Device *d) {
	if (!d) return;
	for (auto dev: m_cdata->devicelist)
		if (dev == d) return;
	if (m_debug > 2)
		std::cout << "Adding device to list: " << d->name() << std::endl;
	m_cdata->devicelist.push_back(d);
}

void Connection_Node::register_node(Device *d, SmartPtr<Connection_Node> a_node) {
	m_cdata->all_nodes[d] = a_node;
}

SmartPtr<Connection_Node> Connection_Node::check_exists(Device *d) {
	if (m_cdata->all_nodes.find(d) != m_cdata->all_nodes.end()) {
		return m_cdata->all_nodes[d];
	}
	return NULL;
}

void Connection_Node::add_loop_term(Device *d) {
	m_cdata->loop_term.insert(d);
}

void Connection_Node::add_loop_start(Device *d) {
	m_cdata->loop_start.insert(d);
}

Node *Connection_Node::get_parent() {
	return m_parent.operator ->();
}

Connection_Node::Connection_Node(Device *current, Node *parent):
			m_current(current),
			m_parent(dynamic_cast<Connection_Node *>(parent)) {

	m_cdata = new Connection_Data();
	m_sources = current->sources();
	m_targets = current->targets();

	SmartPtr<Connection_Node> node = this;
	node.incRef();  // this was allocated on stack, managed externally

	if (m_sources.size()) {
		get_sources(node);
	} else
		get_targets(node);

};

Connection_Node::Connection_Node(Device *current, SmartPtr<Connection_Data>cdata, bool getting_targets):
		m_current(current), m_parent(NULL) {
	m_cdata = cdata;
	m_sources = current->sources();
	m_targets = current->targets();

	SmartPtr<Connection_Node> node = this;
//	node.incRef();  // this was allocated on stack, managed externally

	if (getting_targets)
		get_targets(node);
	else {
		get_sources(node);
	}
};

//_______________________________________________________________________________
// return a count for the number of nodes from a_dev to ground or a known node.
int Connection_Node::find_shortest_path(Device *dev) {
	if (!dev->targets().size() || check_exists(dev)) return 0;
	bool found = false;
	int shortest = 0;
	for (auto d: dev->targets()) {
		auto len = find_shortest_path(d);
		if (!found or len < shortest) shortest = len;
		found = true;
	}
	return shortest + 1;
}

//_______________________________________________________________________________________________
// A mesh is a loop without sub-loops.  We try to use the shortest path to eliminate sub-loops.
// This method returns a list of target nodes ordered by the shortest path to a known node,
//   or to a node with no targets (normally ground).
std::vector<Device *> Connection_Node::shortest_path(std::vector<Device *>a_targets){
	std::vector<Device *> ordered(a_targets);
	std::map<Device *, size_t> counts;

	auto cmp = [&](Device *a, Device *b) {
		return counts[a] < counts[b];
	};

	for (auto d: a_targets)
		counts[d] = find_shortest_path(d);
	std::sort(ordered.begin(), ordered.end(), cmp);

	return ordered;
}


//__________________________________________________________________________
// build a node map recursively
//   produces m_devicelist, m_loop_start and m_loop_term in the first node
void Connection_Node::get_sources(SmartPtr<Connection_Node> a_node) {       // back up the chain
	if (not m_sources.size()) {
		if (not check_exists(m_current)) {
			if (m_debug > 0) std::cout << "      - loop start(a): " << m_current->name() << std::endl;
			add_loop_start(m_current);
			get_targets(a_node);
		}
	} else {
		for (auto d: m_sources) {
			m_parent = check_exists(d);
			if (not m_parent) {
				new Connection_Node(d, m_cdata, false);    // seek top of chain
			} else if (not check_exists(m_current)) {
				if (m_debug > 0) std::cout << "      - loop start(b): " << m_current->name() << std::endl;
				add_loop_start(m_current);
				get_targets(a_node);
			}
		}
	}
}

void Connection_Node::get_targets(SmartPtr<Connection_Node> a_node) {
	if (not check_exists(m_current)) {
		if (m_debug > 0) std::cout << "      - node*: " << m_current->name() << std::endl;

		add_device_to_list(m_current);
		register_node(m_current, a_node);
		bool first = true;
		if (m_targets.size() == 0) {
			add_loop_term(m_current);
			if (m_debug > 0) std::cout << "      - loop end (g): " << m_current->name() << std::endl;
		}
		else
			for (auto d: shortest_path(m_targets)) {
				if (check_exists(d)) {
					if (first) {
						add_loop_term(m_current);
						if (m_debug > 0) std::cout << "      - loop end (q[" << d->name() << "]): " << m_current->name() << std::endl;
					}
				} else {
					if (not first) {
						if (m_debug > 0) std::cout << "      - loop start(c): " << d->name() << std::endl;
						add_loop_start(d);
					}
					new Connection_Node(d, m_cdata);
				}
				first = false;
			}
		get_sources(a_node);
	}
}


//___________________________________________________
// List all mesh configurations. For debugging.
void Connection_Node::show_meshes() {
	for (auto &mesh: m_cdata->meshes) {
		std::cout << "Mesh\n  Items\n";
		for (auto & item: mesh->items) {
			std::cout << "    " << (item.reversed?"*":"") << item.dev->name() << std::endl;
		}
		std::cout << std::flush;
	}
}

//________________________________________________________________
// Build a matrix M describing interconnections and resistances
// for all connected components.  Also build V containing the
// values for all voltage sources.
void Connection_Node::build_matrices(Matrix &m, Matrix &v) {
	for (size_t i=0; i< m_cdata->meshes.size(); ++i) {
		auto &i_mesh = *m_cdata->meshes[i];
		for (size_t j=0; j< m_cdata->meshes.size(); ++j) {
			if (i == j) {
				double rtotal=0, vtotal = 0;
				for (auto &item: i_mesh.items) {
					rtotal += item.R();
					vtotal += item.reversed?-item.V():item.V();
				}
				m(i, i) = rtotal;
				v(0, i) = vtotal;
			} else {
				auto &j_mesh = *m_cdata->meshes[j];
				double rtotal=0;
				for (auto item: i_mesh.shared(j_mesh)) {
					rtotal -= item->R();
				}
				if (rtotal) {
					m(i, j) = rtotal;
					m(j, i) = rtotal;
				}
			}
		}
	}
}

//___________________________________________________
// Calculate current flowing through each mesh.
void Connection_Node::calculate_I(Matrix &m, Matrix &v, double D) {
	for (size_t i=0; i< m_cdata->meshes.size(); ++i) {
		auto &i_mesh = *m_cdata->meshes[i];
		Matrix n(m);
		for (size_t j=0; j< m_cdata->meshes.size(); ++j) n(i,j) = v(0,j);
		if (m_debug > 1) {
			std::cout << "matrix " << i << ": \n";
			n.view();
		}
		double Di = n.determinant();
		i_mesh.I(Di / D);
		if (m_debug > 1) {
			std::cout << "D" << i << " is " << Di;
			std::cout << ", I" << i << " is D" << i << "/D";
			std::cout << " = " << Di << "/" << D << " = " << i_mesh.amps;
			std::cout << std::endl;
		}
	}
}

//___________________________________________________________
// Having a value for the current through a mesh, we may add
// currents for shared components, providing current values
// for each device.
void Connection_Node::add_mesh_totals() {
	for (size_t i=0; i< m_cdata->meshes.size(); ++i) {
		auto &i_mesh = *m_cdata->meshes[i];
		for (auto &item: i_mesh.items)
			if (m_cdata->amps.find(item.dev) == m_cdata->amps.end())
				m_cdata->amps[item.dev] = item.Itotal;
			else
				m_cdata->amps[item.dev] -= item.Itotal;
	}
}

//_______________________________________________________________________________
// Create a matrix M, using the available meshes. Then find it's determinant, D.
// We also find a matrix V having the voltages sources for each mesh.
// Replacing V in M by column in turn, we can calculate D0..Dn.  We can then
// calculate I[n] = D[n] / D, where I[n] is the current through mesh n.
void Connection_Node::solve_meshes() {
	if (m_debug > 0) show_meshes();

	Matrix m(m_cdata->meshes.size());
	Matrix v(1, m_cdata->meshes.size());
	build_matrices(m, v);

	double D = m.determinant();
	if (m_debug > 0) {
		std::cout << "M is \n";
		m.view();
		std::cout << "V is \n";
		v.view();
		std::cout << "D is " << D << std::endl;
	}
	if (D == 0) return;   //nothing to be done
	calculate_I(m, v, D);
	add_mesh_totals();

	for (auto dev: m_cdata->devicelist) {
		dev->set_vdrop(m_cdata->amps[dev] * dev->R());
	}
	for (auto &mesh: m_cdata->meshes) {
		if (mesh->items.size()) {
			auto &item = mesh->items[0];
			if (item.is_voltage())
				item.dev->update_voltage(item.V());  // kick off updates
		}
	}
}

//__________________________________________________________________________________
// Add nodes shared between meshes to the list.  We should end up with each mesh
// describing the components in the mesh, and a list of components shared
// with the previous mesh.  This may not describe all possible circuits, but should
// come close.
bool Connection_Node::add_shared(Mesh &mesh, Connection_Node *start, const std::set<Device *>&finish) {
	if (m_debug > 0 && not start)
		std::cout << " adding shared- start is null\n";
	if (!start) return false;
	if (m_debug > 0) {
		std::cout << " add shared from " << start->device()->name();
		std::cout << " to [";
		for (auto d: finish)
			std::cout << d->name() << ", ";
		std::cout << "]\n";
	}

	// for each loop terminating device
	//     find the set of nodes targeting the terminating device
	//         if one of the nodes targets the start, the loop is complete
	//         otherwise, the terminating devices become the subject of each node found
	//     if the loop is complete, we rewind recursively, adding each device to a new loop

	for (auto d: finish) {      // terminating devices
		SmartPtr<Connection_Node> c = m_cdata->all_nodes[d];    // the node representing [d]
		if (mesh.contains(d)) continue;                         // do not revisit mesh items
		auto sources = c->sources();
		if (!sources.size()) {
			return true;
		} else {
			for (auto s :sources) {                                 // the sources for [d]
				bool valid_source = true;
				if (mesh.contains(s)) continue;                     // do not revisit mesh items
				for (auto m: m_cdata->meshes)
					if (m->reversed(s))
						valid_source = false;                       // part of another loop
				if (!valid_source) continue;
				auto targets = s->targets();                        // all targets for [s]
				auto tset = std::set<Device *>(targets.begin(), targets.end());  // as a set
				if (tset.count(start->device()))                    // target set contains start
					return true;                                    // loop complete
				if (add_shared(mesh, start, {s})) {
					mesh.add(s, true);
					return true;
				}
			}
		}
	}

	return false;
}

//______________________________________________________________________
//  only uses the first node, produces m_meshes which represents the
// set of interconnections.
void Connection_Node::process_model() {
	m_cdata->meshes.clear();
	if (m_debug > 1) {
		std::cout << "DeviceList=";
		for (auto d: m_cdata->devicelist) {
			if (m_cdata->loop_start.find(d) != m_cdata->loop_start.end())
				std::cout << "<";
			std::cout << d->name() << ", ";
			if (m_cdata->loop_term.find(d) != m_cdata->loop_term.end())
				std::cout << ">";
		}
		std::cout << std::endl;
	}

	Mesh *mesh = new Mesh();
	m_cdata->meshes.push_back(mesh);
	for (auto &dev: m_cdata->devicelist) {
		if (m_cdata->loop_start.find(dev) != m_cdata->loop_start.end()) {
			if (mesh->items.size()) {
				mesh = new Mesh();
				m_cdata->meshes.push_back(mesh);
			}
		}
		mesh->add(dev);
		if (m_cdata->loop_term.find(dev) != m_cdata->loop_term.end()) {   // last device in the loop
			auto first = m_cdata->all_nodes[mesh->items[0].dev];
			auto last = dynamic_cast<Connection_Node *>(m_cdata->all_nodes[dev].operator ->());
			if (m_debug > 0) {
				std::cout << "*mesh first=" << mesh->items[0].dev->name();
				std::cout << ", last=" << last->device()->name();
				std::cout << "; finding shared nodes\n";
			}

			Connection_Node *start = dynamic_cast<Connection_Node *>(first.operator ->());
			auto finish = last->target_set();
			add_shared(*mesh, start, finish);
		}
	}
	solve_meshes();
}
