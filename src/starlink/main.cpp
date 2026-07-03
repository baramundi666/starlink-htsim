// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

#include "constellation.h"
#include "ping.h"

int main() {
    EventList eventlist;

    const simtime_picosec END_TIME = timeFromSec(1);
    eventlist.setEndtime(END_TIME);

    Logfile logfile("starlink1.log", eventlist);
    logfile.setStartTime(0);

    Link::_logfile = &logfile;
    Link::_queue_logger_sampling_interval = timeFromMs(10);

    Constellation constellation(
        eventlist,
        speedFromMbps((uint64_t)10000), memFromPkt(20), // uplinks
        speedFromMbps((uint64_t)10000), memFromPkt(20), // downlinks
        speedFromMbps((uint64_t)10000), memFromPkt(20)  // ISLs
    );

    City london(51.5, 0.0, constellation);
    City newyork(40.76, -73.98, constellation);

    london.update_uplinks(0);
    newyork.update_uplinks(0);

    Route* rt_out = london.find_route(newyork, 0);
    Route* rt_back = newyork.find_route(london, 0);

    PingClient pingclient(eventlist, timeFromSec(1), 1500);
    PingServer pingserver;

    rt_out->push_back(static_cast<PacketSink*>(&pingserver));
    rt_back->push_back(static_cast<PacketSink*>(&pingclient));

    pingclient.connect(*rt_out, *rt_back, pingserver,
                       eventlist.now(), timeFromSec(10));

    simtime_picosec last_route_update = eventlist.now();

    while (eventlist.doNextEvent()) {

        simtime_picosec now = eventlist.now();

        // HARD STOP (critical fix)
        if (now >= END_TIME)
            break;

        // periodic route update
        if (timeAsMs(now - last_route_update) > 100) {

            london.update_uplinks(now);
            newyork.update_uplinks(now);

            rt_out->decr_refcount();
            rt_back->decr_refcount();

            rt_out = london.find_route(newyork, now);
            rt_back = newyork.find_route(london, now);

            rt_out->push_back(static_cast<PacketSink*>(&pingserver));
            rt_back->push_back(static_cast<PacketSink*>(&pingclient));

            pingclient.update_route(*rt_out, *rt_back);

            last_route_update = now;
        }
    }

    return 0;
}