/* Copyright 2013 (C) Universita' di Roma La Sapienza
 *
 * This file is part of OFNIC Uniroma GE.
 *
 * OFNIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OFNIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OFNIC.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * @author Andi Palo
 * @ 05/10/2012
 *
 */
#ifndef PATH_EVENT_HH
#define PATH_EVENT_HH 1

#include "event.hh"
#include "flow.hh"
#include "netinet++/datapathid.hh"
#include "network_graph.hh"
#include "openflow/openflow.h"


namespace vigil {

/** \ingroup noxevents
 *
 * 
 *
 */

struct Path_event
    : public Event
{
    enum Action {
        ADD,
        REMOVE
    };
    
    
    
    Path_event(uint16_t pathID_, boost::shared_ptr<hash_map<datapathid, uint64_t> > cookie_, boost::shared_ptr<hash_map<datapathid, ofp_match*> > matchesPtr_, Action action_)
        : Event(static_get_name()), pathID(pathID_), cookies(cookie_), matchesPtr(matchesPtr_), action(action_) {}

    Path_event(uint16_t pathID_, Action action_)
        : Event(static_get_name()), pathID(pathID_), action(action_) {}
    // -- only for use within python
    Path_event() : Event(static_get_name()) { ; }

    static const Event_name static_get_name() {
        return "Path_event";
    }

    uint16_t pathID;
    boost::shared_ptr<hash_map<datapathid, uint64_t> > cookies;
    boost::shared_ptr<hash_map<datapathid, ofp_match*> > matchesPtr;
    Action action;

};

} // namespace vigil

#endif /* port-status.hh */
