/*
 * cairo_drawing.cc
 *
 *  Created on: 31 Aug 2022
 *      Author: paul
 */

#include <map>
#include "cairo_drawing.h"

namespace app{

  std::map<Gtk::DrawingArea *, SmartPtr<Interaction>> Interaction_Factory::m_interactions;    // singleton

  void CairoDrawingBase::draw_info(const Cairo::RefPtr<Cairo::Context>& cr, const std::string &a_info) {
		std::vector<std::string> l_lines;
		LockUI mtx;
		cr->save();
		cr->translate(10, 10);
		double dx=0, dy=0;
		size_t n=0, p=0;
		cr->set_line_width(0.4);
		while ( (n = a_info.substr(p).find_first_of("\n\r")) != std::string::npos) {
			l_lines.push_back(a_info.substr(p, n));
			p = p + n + 1;
		}
		l_lines.push_back(a_info.substr(p));
		Cairo::TextExtents extents;
		for (auto line: l_lines) {
			cr->get_text_extents(line, extents);
			if (dx < extents.width) dx = extents.width;
			dy += extents.height + 4;
		}
		double height = dy;
		bright_yellow(cr);
		cr->rectangle(0, 0, dx+8, dy+8);
		cr->fill();
		dy = 4 + height / l_lines.size();
		black(cr);
		for (auto line: l_lines) {
			cr->move_to(4, dy);
			cr->text_path(line.c_str());
			cr->fill_preserve();
			cr->stroke();
			dy += height / l_lines.size();
		}
		cr->restore();
	}

	void InterConnection::draw(const Cairo::RefPtr<Cairo::Context>& cr) {
		if (!connection) return;
		Point *p1 = const_cast<Point *>(from->point_at(src_index));
		Point *p2 = const_cast<Point *>(to->point_at(dst_index));
		Point *s;
		if (not p1) {
			std::cout << "InterConnection::draw::No point found for source\n";
			return;
		}
		if (not p2) {
			std::cout << "InterConnection::draw::No point found for target\n";
			return;
		}
		double dx = p1->x - p2->x;
		double dy = p1->y - p2->y;

		double leeway = 20;
		double min_space = 5;
		float horz, vert;
		LockUI mtx;

		cr->save();
		if (not connection->determinate()) {
			CairoDrawingBase::blue(cr);
		} else if (connection->signal()){
			CairoDrawingBase::orange(cr);
		} else {
			CairoDrawingBase::gray(cr);
		}

		double src_rot, dst_rot;
		src_index.pt->needs_orientation(src_rot);
		dst_index.pt->needs_orientation(dst_rot);

		int q = src_index.rotate_affinity(src_rot) * 10 + dst_index.rotate_affinity(dst_rot);
//		std::cout << "Code: " << std::dec << q << std::endl;

		switch (q) {
		  case 00:     // e - e
		  case 22:     // w - w
			  if (q == 0)
			  	vert = p1->x - min_space < p2->x ? p2->x + leeway : p1->x + leeway;
			  else
			  	vert = p1->x + min_space > p2->x ? p2->x - leeway : p1->x - leeway;
			  cr->move_to(p1->x, p1->y);
			  cr->line_to(vert, p1->y);
			  cr->line_to(vert, p2->y);
			  cr->line_to(p2->x,  p2->y);
			  break;
		  case 11:     // s - s
		  case 33:     // n - n
			  if (q == 11)
			  	horz = p2->y + min_space > p1->y ? p2->y + leeway : p1->y + leeway;
			  else
			  	horz = p2->y + min_space > p1->y ? p1->y - leeway : p2->y - leeway;
			  cr->move_to(p1->x, p1->y);
			  cr->line_to(p1->x, horz);
			  cr->line_to(p2->x, horz);
			  cr->line_to(p2->x,  p2->y);
			  break;
		  case 01:     // e - s
			  s = p1; p1 = p2; p2 = s;  dx = -dx; dy = -dy;
			  // no break
		  case 10:     // s - e
			  vert = p1->x - dx / 2;
			  horz = p2->y;
			  if (horz < p1->y + min_space) horz = p1->y + leeway;
			  if (vert < p2->x + leeway) {
				  vert = (p1->x > p2->x)?p1->x + leeway:p2->x + leeway;
				  horz = p1->y - dy / 2;
				  if (horz < p1->y + leeway) horz = (p1->y > p2->y)?p1->y + leeway:p2->y + leeway;
			  }
			  cr->move_to(p1->x, p1->y);
			  cr->line_to(p1->x, horz);
			  cr->line_to(vert, horz);
			  cr->line_to(vert, p2->y);
			  cr->line_to(p2->x, p2->y);
			  break;
		  case 03:     // e - n
			  s = p1; p1 = p2; p2 = s;  dx = -dx; dy = -dy;
			  // no break
		  case 30:     // n - e
			  vert = p1->x - dx / 2;
			  horz = p2->y;
			  if (horz > p1->y - min_space) horz = p1->y - leeway;
			  if (vert < p2->x + leeway) {
				  vert = (p1->x > p2->x)?p1->x + leeway:p2->x + leeway;
				  horz = p1->y - dy / 2;
				  if (horz > p1->y - leeway) horz = (p1->y > p2->y)?p2->y - leeway:p1->y - leeway;
			  }
			  cr->move_to(p1->x, p1->y);
			  cr->line_to(p1->x, horz);
			  cr->line_to(vert, horz);
			  cr->line_to(vert, p2->y);
			  cr->line_to(p2->x, p2->y);
			  break;
		  case 21:     // w - s
			  s = p1; p1 = p2; p2 = s;  dx = -dx; dy = -dy;
			  // no break
		  case 12:     // s - w
			  vert = p2->x + dx / 2;
			  horz = p2->y;

			  if (horz < p1->y + min_space) horz = p1->y + leeway;
			  if (vert > p2->x - leeway) {
				  vert = (p1->x < p2->x)?p1->x - leeway:p2->x - leeway;
				  horz = p1->y - dy / 2;
				  if (horz < p1->y + leeway) horz = (p1->y > p2->y)?p1->y + leeway:p2->y + leeway;
			  }

			  cr->move_to(p1->x, p1->y);
			  cr->line_to(p1->x, horz);
			  cr->line_to(vert, horz);
			  cr->line_to(vert, p2->y);
			  cr->line_to(p2->x, p2->y);
			  break;
		  case 23:     // w - n
			  s = p1; p1 = p2; p2 = s; dx = -dx; dy = -dy;
			  // no break
		  case 32:     // n - w
			  vert = p1->x - dx / 2;
			  horz = p2->y;
			  if (horz > p1->y - min_space) horz = p1->y - leeway;
			  if (vert > p2->x - leeway) {
				  vert = (p1->x > p2->x)?p2->x - leeway:p1->x - leeway;
				  horz = p1->y - dy / 2;
				  if (horz > p1->y - leeway) horz = (p1->y > p2->y)?p2->y - leeway:p1->y - leeway;
			  }
			  cr->move_to(p1->x, p1->y);
			  cr->line_to(p1->x, horz);
			  cr->line_to(vert, horz);
			  cr->line_to(vert, p2->y);
			  cr->line_to(p2->x, p2->y);
			  break;
		  case 02:     // e - w
			  s = p1; p1 = p2; p2 = s;  dx = -dx; dy = -dy;
			  // no break
		  case 20:     // w - e
			  if (dx < min_space) {
				  horz = p2->y + dy/2;
				  if (fabs(dy) < leeway) horz = dy<0?p1->y - leeway:p1->y + leeway;
				  float vert1 = p1->x - leeway;
				  float vert2 = p2->x + leeway;

				  cr->move_to(p1->x, p1->y);
				  cr->line_to(vert1, p1->y);
				  cr->line_to(vert1, horz);
				  cr->line_to(vert2, horz);
				  cr->line_to(vert2, p2->y);
				  cr->line_to(p2->x, p2->y);
			  } else {
				  vert = p1->x - dx/2;
				  cr->move_to(p1->x, p1->y);
				  cr->line_to(vert, p1->y);
				  cr->line_to(vert, p2->y);
				  cr->line_to(p2->x, p2->y);
			  }
			  break;
		  case 31:     // n - s
			  s = p1; p1 = p2; p2 = s;  dx = -dx; dy = -dy;
			  // no break
		  case 13:     // s - n
			if (dy > min_space) {
				float vert = p2->x + dx/2;
				if (fabs(dx) < leeway) vert = dx>0?p2->x - leeway:p2->x + leeway;
				cr->move_to(p1->x, p1->y);
				cr->line_to(p1->x, p1->y + leeway);
				cr->line_to(vert, p1->y + leeway);
				cr->line_to(vert, p2->y - leeway);
				cr->line_to(p2->x, p2->y - leeway);
				cr->line_to(p2->x, p2->y);
			} else {
				cr->move_to(p1->x, p1->y);
				cr->line_to(p1->x, p1->y - dy / 2);
				cr->line_to(p2->x, p1->y - dy / 2);
				cr->line_to(p2->x, p2->y);
			}
			break;
		}
		cr->stroke();
		cr->restore();
	}

	InterConnection::InterConnection(
			CairoDrawingBase *source, const WHATS_AT &source_info,
			CairoDrawingBase *target, const WHATS_AT &target_info)
		: from(source), to(target), src_index(source_info), dst_index(target_info),
		  connection(NULL)
	{
		if (not (source_info.what == WHATS_AT::OUTPUT || source_info.what == WHATS_AT::IN_OUT))
			return;
		if (not (target_info.what == WHATS_AT::INPUT ||
				 target_info.what == WHATS_AT::GATE ||
				 target_info.what == WHATS_AT::IN_OUT))
			return;

		if (to && from) {
			connection = from->slot(src_index);
			if (connection)
				connected = to->slot(dst_index, connection);
			if (connected) {
				std::cout << "Connected from " << source_info.asText("") << " to " << target_info.asText("") << std::endl;
				connection->queue_change(true, ":  Connect");
			}
		}
	}


	void Interaction::select_source_cursor(const Glib::RefPtr<Gdk::Window> &win, WHATS_AT::ELEMENT what) {
		if (what==WHATS_AT::IN_OUT) {
			win->set_cursor(m_cursor_in_out);
		} else if (what==WHATS_AT::OUTPUT) {
			win->set_cursor(m_cursor_output);
		} else if (what==WHATS_AT::START) {
			win->set_cursor(m_cursor_start);
		} else if (what==WHATS_AT::CLICK) {
			win->set_cursor(m_cursor_click);
		} else if (what==WHATS_AT::SYMBOL) {
			win->set_cursor(m_cursor_symbol);
		} else if (what==WHATS_AT::LINE) {
			win->set_cursor(m_cursor_line);
		} else if (what==WHATS_AT::POINT) {
			win->set_cursor(m_cursor_point);
		} else if (what==WHATS_AT::TEXT) {
			win->set_cursor(m_cursor_text);
		}
	}

	void Interaction::select_target_cursor(const Glib::RefPtr<Gdk::Window> &win, WHATS_AT::ELEMENT what) {
		if (what==WHATS_AT::IN_OUT) {
			win->set_cursor(m_cursor_input);
		} else if (what==WHATS_AT::END) {
			win->set_cursor(m_cursor_end);
		} else if (what==WHATS_AT::INPUT) {
			win->set_cursor(m_cursor_input);
		} else if (what==WHATS_AT::GATE) {
			win->set_cursor(m_cursor_input);
		}
	}

	bool Interaction::button_press_event(GdkEventButton* button_event) {
		for (auto &dwg : m_drawings) {
			if (!dwg->interactive()) continue;

			WHATS_AT w = dwg->location(Point(button_event->x, button_event->y));
			if (w.what!=WHATS_AT::NOTHING) {
				m_actions.push(Action(dwg, Point(button_event->x, button_event->y), w));
			}
		}
		while (m_actions.size() > 1) m_actions.pop();  // retain only the last action
		return true;  // Highlander: stop propagating this event
	}

	bool Interaction::button_release_event(GdkEventButton* button_event) {
		std::stack<Action> l_term;
		if (m_actions.size()) {
			if (button_event->y <= 0) {
				auto &source = m_actions.front();
				deleting(source.dwg);
			} else {
				for (auto &dwg : m_drawings) {
					WHATS_AT w = dwg->location(Point(button_event->x, button_event->y), true);
					if (w.what!=WHATS_AT::NOTHING) {
						l_term.push(Action(dwg, Point(button_event->x, button_event->y), w));
					}
				}
				if (l_term.size()) {
					auto &source = m_actions.front();
					auto &target = l_term.top();
					switch (button_event->button) {
					case 1:          // left button released
						if (source.dwg == target.dwg && target.what.what == WHATS_AT::CLICK) {
							target.dwg->click_action(target.what);
							m_area->queue_draw();
						} else {
							target.dwg->slot(source.dwg, source.what, target.what);
							m_area->queue_draw();
						}
						break;
					case 2:          // middle button released
						break;
					case 3:          // right button released
						if (source.origin.close_to(target.origin))
							target.dwg->context(target.what);    // it's a click
						break;
					}
				}
			}
			while (m_actions.size()) m_actions.pop();  // clear all actions
		}
		return true;  // Highlander: stop propagating this event
	}

	bool Interaction::motion_event(GdkEventMotion* motion_event) {
//			std::cout << "motion x=" << motion_event->x << " y=" << motion_event->y << ";" << std::endl;
		LockUI mtx;
		bool deleting = false;
		std::stack<WHATS_AT> locations;
		Glib::RefPtr<Gdk::Window> win = Glib::wrap(motion_event->window, true);

		for (auto &dwg : m_drawings) {
			if (!dwg->interactive()) continue;
			WHATS_AT w = dwg->location(Point(motion_event->x, motion_event->y));
			if (w.what!=WHATS_AT::NOTHING) {
				locations.push(w);
			}

			if (!m_actions.size()) {
				dwg->on_motion(motion_event->x, motion_event->y, motion_event->state);
			}
		}
		if (m_actions.size()) {
			auto &source = m_actions.front();
			if (source.what.what == WHATS_AT::SYMBOL) {
				Point p(motion_event->x, motion_event->y);
				p.scale(m_scale).snap(grid_size);

				if (not source.origin.close_to(p)) {
					if ((motion_event->state & Gdk::BUTTON1_MASK) == Gdk::BUTTON1_MASK) {
						if ((motion_event->state & Gdk::SHIFT_MASK) == Gdk::SHIFT_MASK)
							source.dwg->move(source.what, p, true);
						else
							source.dwg->move(source.what, p);
						m_area->queue_draw();
					}
				}
			}
			deleting = (motion_event->y <= 0);
//			std::cout << "Cursor drag: x=" << motion_event->x << "; y=" << motion_event->y << std::endl;
		}
		if (deleting) {
			win->set_cursor(m_cursor_del);
		} else if (locations.size()) {
			auto w = locations.top();
			if (m_actions.size()) {     // dragging over possible target
				select_target_cursor(win, w.what);
			} else {                    // Just moving, action selected
				select_source_cursor(win, w.what);
			}
		} else if (m_actions.size()) {  // dragging over nothing
			auto w = m_actions.front().what;
			select_source_cursor(win, w.what);
		} else {
			win->set_cursor(m_cursor_arrow);
		}
		return true;   // there can be only one!
	}

	void Interaction::recalc_scale() {
		double swidth =  m_alloc_width / m_pix_width;
		double sheight = m_alloc_height / m_pix_height;
		m_scale = swidth < sheight?swidth:sheight;  // maintain aspect ratio
	}

	void Interaction::size_changed(Gtk::Allocation& allocation) {
		m_alloc_width = allocation.get_width();
		m_alloc_height = allocation.get_height();
		recalc_scale();
	}


	void Interaction::set_extents(float a_pix_width, float a_pix_height) {
		m_pix_width = a_pix_width; m_pix_height = a_pix_height;
		recalc_scale();
	}

	double Interaction::get_scale() const { return m_scale; }

	void Interaction::add_drawing(CairoDrawingBase *drawing) {
		m_drawings.push_back(drawing);
	}

	void Interaction::remove_drawing(CairoDrawingBase *drawing) {
		for (size_t n=0; n < m_drawings.size(); ++n)
			if (m_drawings[n] == drawing) {
				m_drawings.erase(m_drawings.begin() + n);
				return;
			}
	}

	Interaction::Interaction(Glib::RefPtr<Gtk::DrawingArea> a_area): m_area(a_area) {

		m_cursor_arrow = Gdk::Cursor::create(Gdk::CursorType::ARROW);
		m_cursor_in_out = Gdk::Cursor::create(Gdk::CursorType::DOT);
		m_cursor_output = Gdk::Cursor::create(Gdk::CursorType::DOT);
		m_cursor_input = Gdk::Cursor::create(Gdk::CursorType::PLUS);
		m_cursor_start = Gdk::Cursor::create(Gdk::CursorType::LEFT_SIDE);
		m_cursor_end = Gdk::Cursor::create(Gdk::CursorType::RIGHT_SIDE);
		m_cursor_symbol = Gdk::Cursor::create(Gdk::CursorType::TCROSS);
		m_cursor_click = Gdk::Cursor::create(Gdk::CursorType::HAND1);
		m_cursor_line = Gdk::Cursor::create(Gdk::CursorType::HAND2);
		m_cursor_point = Gdk::Cursor::create(Gdk::CursorType::PENCIL);
		m_cursor_text = Gdk::Cursor::create(Gdk::CursorType::DRAFT_LARGE);
		m_cursor_del = Gdk::Cursor::create(Gdk::CursorType::PIRATE);

		m_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &Interaction::motion_event));
		m_area->signal_button_press_event().connect(sigc::mem_fun(*this, &Interaction::button_press_event));
		m_area->signal_button_release_event().connect(sigc::mem_fun(*this, &Interaction::button_release_event));
		m_area->signal_size_allocate().connect(sigc::mem_fun(*this, &Interaction::size_changed));

		m_area->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK );

	}




};

