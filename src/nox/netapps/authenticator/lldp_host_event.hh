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
#ifndef LLDP_HOST_EVENT_HH
#define LLDP_HOST_EVENT_HH


#include <boost/noncopyable.hpp>
#include <stdint.h>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"

namespace vigil {

struct Lldp_host_event
    : public Event
{
    enum Action {
        AUTHENTICATE,
        DEAUTHENTICATE,
    };

    enum Enabled_field {
        EF_SWITCH        = 0x1 << 0,
        EF_LOCATION      = 0x1 << 1,
        EF_DLADDR        = 0x1 << 2,
        EF_NWADDR        = 0x1 << 3,
        EF_HOSTNAME      = 0x1 << 4,
        EF_HOST_NETID  = 0x1 << 5,
        EF_ALL           = (0x1 << 6) - 1
    };

    // AUTHENTICATE constructor
    Lldp_host_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t idle_timeout_,
                    uint32_t hard_timeout_);

    // DEAUTHENTICATE constructor
    Lldp_host_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t enabled_fields_);

    // -- only for use within python
    Lldp_host_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Lldp_host_event";
    }

    Action              action;
    datapathid          datapath_id;
    uint16_t            port;
    ethernetaddr        dladdr;
    uint32_t            nwaddr;         // set to zero if no IP to auth
    int64_t             hostname;
    int64_t             host_netid;
    uint32_t            idle_timeout;
    uint32_t            hard_timeout;
    uint32_t            enabled_fields; // bit_mask of fields to observe
};

}

#endif
