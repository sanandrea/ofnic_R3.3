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

#ifndef pylinkload_proxy_HH
#define pylinkload_proxy_HH

#include <Python.h>

#include "linkload.hh"
#include "topology/topology.hh"
#include "pyrt/pyglue.hh"

namespace vigil {
namespace applications {

struct PyLoad
{
  /** Transmission load (in bytes)
   */
  uint64_t txLoad;
  /** Reception load (in bytes)
   */
  uint64_t rxLoad;
  /** Interval
   */
  time_t interval;
  PyLoad(){;}
  PyLoad(uint64_t txLoad_, uint64_t rxLoad_, time_t interval_):
txLoad(txLoad_), rxLoad(rxLoad_), interval(interval_)
  {}
};

struct Pyflow_load{
    uint64_t    p_count;
    uint64_t    b_count;
    
    Pyflow_load(){;}
    Pyflow_load(uint64_t a, uint64_t b):
    p_count(a), b_count(b){}
};

struct PyPortErrors
{
    uint64_t rxDropped;
    uint64_t txDropped;
    uint64_t rxErrors;
    uint64_t txErrors;
    uint64_t rxFrameErr;
    uint64_t rxOverErr;
    uint64_t rxCrcErr;
    uint64_t collisions;

    time_t interval;

    PyPortErrors(){;}
    PyPortErrors(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
			uint64_t e, uint64_t f, uint64_t g, uint64_t h,
			time_t interval_) :
			rxDropped(a), txDropped(b), rxErrors(c), txErrors(d), rxFrameErr(
					e), rxOverErr(f), rxCrcErr(g), collisions(h), interval(
					interval_) {
	}

};

class pylinkload_proxy{
public:
    pylinkload_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    /** Get link load.
     * @param dpid datapath id of switch
     * @param port port number
     * @return ratio of link bandwidth used (interval = 0 for invalid result)
     */
    PyLoad get_link_load(datapathid dpid, uint16_t portID,bool& found);


    PyPortErrors get_port_errors_proxy(datapathid dpid, uint16_t portID, bool& found);
    
    Pyflow_load get_flow_load(uint32_t monitorID, int& found);
    
    uint64_t create_monitor_flow(uint16_t pathID, datapathid dpid, uint64_t duration, uint64_t frequency, int& error);

    int remove_monitor_flow(uint32_t monitorID);
    
    std::string get_path_mon_ids();
    
    int dummyLoad();
protected:
    linkload* load;
    container::Component* c;
}; // class pytopology_proxy


} // applications
} // vigil

#endif  // linkload_proxy_HH
