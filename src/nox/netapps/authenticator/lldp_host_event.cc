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
 * @ 17/07/2012
 *
 */

#include "component.hh"

#include "vlog.hh"
#include "lldp_host_event.hh"
using namespace std;
using namespace vigil;
using namespace vigil::container;

namespace vigil {

// AUTHENTICATE constructor
Lldp_host_event::Lldp_host_event(datapathid datapath_id_, uint16_t port_,
                                 ethernetaddr dladdr_, uint32_t nwaddr_,
                                 int64_t hostname_, int64_t host_netid_,
                                 uint32_t idle_timeout_, uint32_t hard_timeout_)
    : Event(static_get_name()), action(AUTHENTICATE), datapath_id(datapath_id_),
      port(port_), dladdr(dladdr_), nwaddr(nwaddr_), hostname(hostname_),
      host_netid(host_netid_), idle_timeout(idle_timeout_),
      hard_timeout(hard_timeout_)
{}

// DEAUTHENTICATE constructor
Lldp_host_event::Lldp_host_event(datapathid datapath_id_, uint16_t port_,
                                 ethernetaddr dladdr_, uint32_t nwaddr_,
                                 int64_t hostname_, int64_t host_netid_,
                                 uint32_t enabled_fields_)
    : Event(static_get_name()), action(DEAUTHENTICATE),
      datapath_id(datapath_id_), port(port_), dladdr(dladdr_), nwaddr(nwaddr_),
      hostname(hostname_), host_netid(host_netid_),
      enabled_fields(enabled_fields_)
{}

}

namespace {

static Vlog_module lg("lldp-host-event");

class LldpHostEvent_component
    : public Component {
public:
    LldpHostEvent_component(const Context* c,
                     const json_object*) 
        : Component(c) {
    }

    void configure(const Configuration*) {
    }

    void install() {
    }

private:
    
};

REGISTER_COMPONENT(container::Simple_component_factory<LldpHostEvent_component>, 
                   LldpHostEvent_component);

} // unnamed namespace
