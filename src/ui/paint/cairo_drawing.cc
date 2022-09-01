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

};

