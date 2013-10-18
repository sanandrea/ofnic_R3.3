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

#ifndef pytopology_proxy_HH
#define pytopology_proxy_HH

#include <Python.h>

#include "topology.hh"
#include "pyrt/pyglue.hh"

namespace vigil {
namespace applications {

struct PyLinkPorts {
    uint16_t src;
    uint16_t dst;

    PyLinkPorts(){;}
    PyLinkPorts(uint16_t s, uint16_t d):
        src(s), dst(d){
        }
};


//UoR
struct PyNodeInfo{
    uint64_t dpid;
    bool active;

    PyNodeInfo(){;}
    PyNodeInfo(uint64_t a, bool b):
	dpid(a),active(b){}
};

//UoR
struct PyPortInfo{
    std::string name;
    std::string state;
    std::string medium;
    std::string duplexity;
    uint16_t    portID;
    int speed;	

    PyPortInfo(){;}
    PyPortInfo(std::string n, std::string st, std::string med,std::string dup, uint16_t pID,int sp):
	name(n),state(st),medium(med),duplexity(dup),portID(pID),speed(sp){}
};


//Added by Andrea Simeoni
struct DatapathFeatures{

	bool active;
	datapathid dpid;
};


class pytopology_proxy{
public:
    pytopology_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    std::list<PyLinkPorts> get_outlinks(datapathid dpsrc, datapathid dpdst) const;
    bool is_internal(datapathid dp, uint16_t port) const;
    /** \added by Andrea Simeoni--Returns the whole topology data structure
    */
    hash_map<datapathid,Topology::DpInfo> get_whole_topology();
    /** \added by Andrea Simeoni--Returns the whole topology data structure
    */
    DatapathFeatures get_dpinfo( datapathid dp) const;

    /** UoR
   */
   
    std::list<PyNodeInfo> synchronize_network();

    /** UoR
   */
   
    std::list<PyPortInfo> synchronize_node(datapathid dp);

   /** UoR
   */
   
    PyPortInfo synchronize_port(datapathid id, std::string pname);

   /** UoR
   */
    
    bool is_valid_dp(datapathid dp);

    bool is_valid_port(datapathid dp, std::string portName);

protected:   

    Topology* topo;
    container::Component* c;
}; // class pytopology_proxy


} // applications
} // vigil

#endif  // pytopology_proxy_HH
