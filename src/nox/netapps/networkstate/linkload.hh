/* Copyright 2010 (C) Stanford University.
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
#ifndef LINKLOAD_HH
#define LINKLOAD_HH

#include "component.hh"
#include "config.h"
#include "flow-stats-in.hh"
#include "hash_map.hh"
#include "port-stats-in.hh"
#include "network_graph.hh"
#include "netinet++/datapathid.hh"
#include "datapathmem.hh"
#include "path-event.hh"
#include <boost/shared_array.hpp>

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/**  Default interval to query for link load (in s)
 */
#define LINKLOAD_DEFAULT_INTERVAL 10

namespace vigil
{
  using namespace std;
  using namespace vigil::container;
  /** \brief linkload: tracks port stats of links and its load
   * \ingroup noxcomponents
   * 
   * Refers to links using its source datapath id and port.
   *
   * You can configure the interval of query using keyword
   * interval, e.g.,
   * ./nox_core -i ptcp:6633 linkload=interval=10
   *
   * @author ykk
   * @date February 2010
   * @update October 2012
   */
  class linkload
    : public Component 
  {
  public:
    /** Definition switch and port
     */
    typedef vigil::network::switch_port switchport;
    typedef uint16_t PathID_t;
    typedef uint32_t MonitorID_t;
    typedef uint64_t Cookie_t;
    typedef boost::shared_ptr<hash_map<datapathid, Cookie_t> > CookiesPtr_t;
    typedef boost::shared_ptr<hash_map<datapathid, ofp_match*> > MatchesPtr_t;
    
    struct monitorInfo{
        PathID_t pid;
        datapathid dpid;
        ofp_match* match;
        Cookie_t cookie;
        timeval start_time; /* Time submitted */
        uint64_t duration;
        uint64_t frequency;
    };

    /** Hash map of switch/port to stats
     */
    hash_map<switchport, Port_stats> statmap;
    
    /** Hash map of cookie : flow statistics.
     */
    hash_map<MonitorID_t, Flow_stats> flowStatMap;
    
    hash_map<MonitorID_t, monitorInfo> monitoredFlows;
    
    /** Hash map of PathId : flow matches.
     */
    hash_map<PathID_t, CookiesPtr_t> pathCookies;
    
    hash_map<PathID_t, MatchesPtr_t> pathMatches;

    /** Load of switch/port (in bytes)
     */
    struct load
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

      load(uint64_t txLoad_, uint64_t rxLoad_, time_t interval_):
	txLoad(txLoad_), rxLoad(rxLoad_), interval(interval_)
      {}
    };

    struct ErrorsPerSecond
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

        ErrorsPerSecond(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
				uint64_t e, uint64_t f, uint64_t g, uint64_t h,
				time_t interval_) :
				rxDropped(a), txDropped(b), rxErrors(c), txErrors(d), rxFrameErr(
						e), rxOverErr(f), rxCrcErr(g), collisions(h), interval(
						interval_) {
		}

    };
    struct flow_load{
        uint64_t    p_count;
        uint64_t    b_count;
        time_t      interval;
        
        flow_load(uint64_t a, uint64_t b, time_t interval_):
            p_count(a), b_count(b), interval(interval_){}
    };

    /** Hash map of flow load.
     */
    hash_map<MonitorID_t, flow_load> flowLoadMap;
    
    /** Hash map of switch/port to load
     */
    hash_map<switchport, load> loadmap;


    /** Hash map of switch/port to errors
     *
     */
    hash_map<switchport, ErrorsPerSecond> errorMap;

    /** Interval to query for load
     */
    time_t load_interval;
    
    /** Interval to query for paths
     */
    time_t flow_interval;

    /** \brief Constructor of linkload.
     *
     * @param c context
     * @param node configuration (JSON object) 
     */
    linkload(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure linkload.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Periodic port stat probe function
     */
    void stat_probe();
    
    /** Periodic flow stat probe function
     */
    void flow_stat_probe();

    /** Get link load ratio.
     * @param dpid datapath id of switch
     * @param port port number
     * @param tx transmit load
     * @return ratio of link bandwidth used (-1 if invalid result)
     */
    float get_link_load_ratio(datapathid dpid, uint16_t port, bool tx=true);

    /** Dummy method
     *
     */
    void dummyLoad();

    /** \brief Get link load.
     * @param dpid datapath id of switch
     * @param port port number
     * @return ratio of link bandwidth used (interval = 0 for invalid result)
     */
    load get_link_load(datapathid dpid, uint16_t port, bool& found);

    /** \brief Get port errors.
     * @param dpid datapath id of switch
     * @param port port number
     * @return errors of the port mediated in the interval of time
     */
    ErrorsPerSecond get_port_errors(datapathid dpid, uint16_t port, bool& found);
    
    /** \brief Get flow statistics
     * @param monitor ID of flow being monitored
     * @param pointer to error code of the operation
     * @return flow statistics in the interval of time
     */
    flow_load get_flow_load(MonitorID_t mid, int& found);
    
    /** \brief Start to monitor a path in a certain datapath
     * @param pathID of the path to monitor 
     * @param dpid datapath id of the node in which the path will be monitored
     * @param duration of the monitor
     * @param frequency of the polls to the switch, not implemented still
     * @param error that will be returned
     * @return the monitor ID to be used as reference for future calls
     */
    uint32_t create_monitor_flow(PathID_t pathID, datapathid dpid, uint64_t duration, uint64_t frequency, int& error);
    
    /** \brief Returns a string with all monitorids separated by ':'
     * @return the string of monitor IDs
     */
    std::string get_path_mon_ids();
    
    /** \brief Remove a specific monitor ID
     * @param mid monitor ID to be removed
     * @return the result code of the method
     */
    int remove_monitor_flow(uint32_t mid);

    /** \brief Handle port stats in
     * @param e port stats in event
     * @return CONTINUE
     */
    Disposition handle_port_stats(const Event& e);
    
    /** \brief Handle flow stats in
     * @param e flow stats in event
     * @return CONTINUE
     */
    Disposition handle_flow_stats(const Event& e);
    
    /** \brief Handle datapath leave (remove ports)
     * @param e datapath leave event
     * @return CONTINUE
     */
    Disposition handle_dp_leave(const Event& e);

    /** \brief Handle port status change
     * @param e port status event
     * @return CONTINUE
     */
    Disposition handle_port_event(const Event& e);

    /** \brief Handle path event
     * @param e path event
     * @return CONTINUE
     */ 
    Disposition handle_path_event(const Event& e);
    /** \brief Start linkload.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of linkload.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    linkload*& component);

  private:
    /** \brief Reference to datapath memory
     */
    datapathmem* dpmem;
    
    /** Iterator for probing
     */
    hash_map<uint64_t,Datapath_join_event>::const_iterator dpi;

    /** \brief Send port stats request for switch and port
     * @param dpid switch to send port stats request to
     * @param port port to request stats for
     */
    void send_stat_req(const datapathid& dpid, uint16_t port=OFPP_ALL);
    
    
    /** \brief UoR: Send flow stats request to switch.
     *
     * @param dpid switch to send flow stat to
     * @param path id identifying the flow
     */
     void send_flow_stat_req(const datapathid& dpid, ofp_match* om);

    /** \brief Get next time to send probe
     *
     * @return time for next probe
     */
    timeval get_next_time();
    
    /** \brief Get next time to send probe for paths
     *
     * @return time for next probe
     */    
    timeval get_next_monitor_time();
    
    /** \brief Calculate the cookie as the hash of a Flow build with the match's information
     * @param match ofp_match from which the Flow will be build
     * @return the cookie claculated as the hash
     */
    uint64_t get_cookie_for_match(ofp_match* match);
    
    /** Calculate the monitor ID as a fnv_hash from two input values datapath id and cookie
     * @param dpid datapath id
     * @param cookie input Cookie 
     * @return monitor ID
     */
    MonitorID_t calculate_monitorID(datapathid dpid, Cookie_t cookie);
    
    /** \brief Memory for OpenFlow packet
     */
    boost::shared_array<uint8_t> of_raw;
    /** \brief Update load map
     * @param dpid switch to send port stats request to
     * @param port port to request stats for
     * @param oldstat old port stats
     * @param newstat new port stats
     */
    void updateLoad(const datapathid& dpid, uint16_t port,
		    const Port_stats& oldstat,
		    const Port_stats& newstat);

    /** \brief Update Flow load map
     * @param cookie identifier
     * @param oldstat old flow stats
     * @param newstat new flow stats
     */		    
    void updateFlowLoad(MonitorID_t mid, const Flow_stats& oldstat, 
                                         const Flow_stats& newstat);
                                         

    /** \brief Update ErrorPerSecond map
     * @param dpid switch to send port stats request to
     * @param port port to request stats for
     * @param oldstat old port stats
     * @param newstat new port stats
     */
    void updateErrors(const datapathid& dpid, uint16_t port,
    			    const Port_stats& oldstat,
    			    const Port_stats& newstat);
		    
  };
}

#endif
