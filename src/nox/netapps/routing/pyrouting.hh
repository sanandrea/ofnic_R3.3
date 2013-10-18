/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#ifndef CONTROLLER_PYROUTEGLUE_HH
#define CONTROLLER_PYROUTEGLUE_HH 1

#include <Python.h>

#include "routing.hh"
#include "component.hh"
#include "network_graph.hh"
/*
#include "openflow-pack.hh"
#include "openflow/openflow/openflow.h"
*/


/*
 * Proxy routing "component" used to obtain routes from python.
 */

namespace vigil {
namespace applications {

struct PyPathResult{
    std::string	directPath;
    std::string	reversPath;

    PyPathResult(){;}
    PyPathResult(std::string b, std::string c):
	             directPath(b), reversPath(c){}
};
 
struct Netic_path_desc {
    Routing_module::RouteId rteId;
    std::string path;
    long int    timeToDie;
    Netic_path_info npi;
    
    Netic_path_desc(){;}
    Netic_path_desc(Routing_module::RouteId r, std::string s,uint64_t a, Netic_path_info n):
        rteId(r),path(s),timeToDie(a),npi(n){}
};

class PyRouting_module {
public:
    PyRouting_module(PyObject*);

    void configure(PyObject*);

    bool get_route(Routing_module::Route& route) const;
    bool check_route(const Routing_module::Route& route,
                     uint16_t inport, uint16_t outport) const;
    bool setup_route(const Flow& flow, const Routing_module::Route& route,
                     uint16_t inport, uint16_t outport, uint16_t flow_timeout,
                     const std::list<Nonowning_buffer>& bufs, bool check_nat);

    bool setup_flow(const Flow& flow, const datapathid& dp,
                    uint16_t outport, uint32_t bid, const Nonowning_buffer& buf,
                    uint16_t flow_timeout, const Nonowning_buffer& actions,
                    bool check_nat);

    bool send_packet(const datapathid& dp, uint16_t inport, uint16_t outport,
                     uint32_t bid, const Nonowning_buffer& buf,
                     const Nonowning_buffer& actions, bool check_nat,
                     const Flow& flow);

   /**
    * UoR
    * Installs on the route along sdpid and ddpid, a flow identified only by source and dest ip
    */
    PyPathResult netic_create_route(Netic_path_info npi, int& OUTPUT);
    
    Netic_path_desc netic_get_path_desc(uint16_t, bool& OUTPUT);

    std::string netic_get_path_list();

    int netic_remove_path(uint16_t);
   

  

private:
    Routing_module *routing;
    container::Component* c;
};

}
}

#endif
