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

#include "pylinkload.hh"

#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("pylinkload");
}

pylinkload_proxy::pylinkload_proxy(PyObject* ctxt) {
	if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
		throw runtime_error("Unable to access Python context.");
	}

	c = ((PyContext*) SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void pylinkload_proxy::configure(PyObject* configuration) {
	c->resolve(load);
}

void pylinkload_proxy::install(PyObject* p) {
}

PyLoad pylinkload_proxy::get_link_load(datapathid dpid, uint16_t portID, bool& found) {

	linkload::load cLoad = load->get_link_load(dpid, portID, found);

	PyLoad ret(cLoad.txLoad, cLoad.rxLoad, cLoad.interval);
	return ret;

}
Pyflow_load pylinkload_proxy::get_flow_load(uint32_t monitorID, int& error) {

	linkload::flow_load fLoad = load->get_flow_load(monitorID, error);

	Pyflow_load ret(fLoad.p_count, fLoad.b_count);
	return ret;

}
PyPortErrors pylinkload_proxy::get_port_errors_proxy(datapathid dpid,
		uint16_t portID, bool& found) {
	linkload::ErrorsPerSecond errors = load->get_port_errors(dpid, portID,found);

	PyPortErrors ret(errors.rxDropped, errors.txDropped, errors.rxErrors,
			errors.txErrors, errors.rxFrameErr, errors.rxOverErr,
			errors.rxCrcErr, errors.collisions, errors.interval);
	return ret;
}

int pylinkload_proxy::remove_monitor_flow(uint32_t monitorID){
        return load->remove_monitor_flow(monitorID);
}

uint64_t pylinkload_proxy::create_monitor_flow(uint16_t pathID, datapathid dpid, uint64_t duration, uint64_t frequency, int& error){
    return load->create_monitor_flow(pathID, dpid, duration, frequency, error);
}
std::string pylinkload_proxy::get_path_mon_ids(){
    return load->get_path_mon_ids();
}

int pylinkload_proxy::dummyLoad() {
	load->dummyLoad();
	return 0;
}

