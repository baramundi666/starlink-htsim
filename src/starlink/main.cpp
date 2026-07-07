// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

#include "constellation.h"
#include "experiment_logger.h"
#include "ping.h"

#include <chrono>
#include <cstdlib>
#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

struct RouteState {
    string last_hash;
    simtime_picosec last_change;
    RouteState() : last_hash(""), last_change(0) {}
};

static void usage(const char* argv0) {
    cerr << "Usage: " << argv0 << " [options]\n"
         << "  --planes N                  populated orbital planes, default 8\n"
         << "  --sats-per-plane N          satellites per populated plane, default 66\n"
         << "  --orbital-slots N           FCC slots used for adjacent mode, default 66\n"
         << "  --sat-selection spread|adjacent  default spread\n"
         << "  --sat-offset N              first slot in adjacent mode, default 0\n"
         << "  --duration S                simulation duration in seconds, default 300\n"
         << "  --route-update-ms MS        route recalculation period, default 100\n"
         << "  --queue-sample-ms MS        queue logger sampling interval, default 10\n"
         << "  --ping-interval-ms MS       ping interval, default 1000\n"
         << "  --uplink-mbps N             default 10000\n"
         << "  --downlink-mbps N           default 10000\n"
         << "  --isl-mbps N                default 10000\n"
         << "  --queue-pkts N              queue size in packets, default 20\n"
         << "  --packet-size BYTES         ping packet size, default 1500\n"
         << "  --src LAT,LON               source city coordinates, default London\n"
         << "  --dst LAT,LON               destination coordinates, default New York\n"
         << "  --out PREFIX                output prefix, default results/run\n"
         << "  --routing-only              do not create ping traffic; log routing metrics only\n"
         << "  --verbose                   print extra progress to stderr\n";
}

static ExperimentConfig default_config() {
    ExperimentConfig cfg;
    cfg.num_planes = 8;
    cfg.sats_per_plane = 66;
    cfg.orbital_slots = 66;
    cfg.adjacent_sats = false;
    cfg.sat_offset = 0;
    cfg.duration_s = 300.0;
    cfg.route_update_ms = 100.0;
    cfg.queue_sample_ms = 10.0;
    cfg.ping_interval_ms = 1000.0;
    cfg.uplink_mbps = 10000;
    cfg.downlink_mbps = 10000;
    cfg.isl_mbps = 10000;
    cfg.queue_pkts = 20;
    cfg.packet_size_bytes = 1500;
    cfg.src_lat = 51.5;
    cfg.src_lon = 0.0;
    cfg.dst_lat = 40.76;
    cfg.dst_lon = -73.98;
    cfg.out_prefix = "results/run";
    cfg.routing_only = false;
    cfg.verbose = false;
    return cfg;
}

static bool parse_pair(const string& value, double& first, double& second) {
    size_t comma = value.find(',');
    if (comma == string::npos) return false;
    first = atof(value.substr(0, comma).c_str());
    second = atof(value.substr(comma + 1).c_str());
    return true;
}

static string require_value(int& i, int argc, char** argv) {
    if (i + 1 >= argc) {
        cerr << "Missing value for " << argv[i] << endl;
        exit(1);
    }
    return string(argv[++i]);
}

static ExperimentConfig parse_args(int argc, char** argv) {
    ExperimentConfig cfg = default_config();

    for (int i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            exit(0);
        } else if (arg == "--planes") {
            cfg.num_planes = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--sats-per-plane") {
            cfg.sats_per_plane = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--orbital-slots") {
            cfg.orbital_slots = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--sat-selection") {
            string mode = require_value(i, argc, argv);
            if (mode == "adjacent") cfg.adjacent_sats = true;
            else if (mode == "spread") cfg.adjacent_sats = false;
            else {
                cerr << "Unknown --sat-selection: " << mode << endl;
                exit(1);
            }
        } else if (arg == "--sat-offset") {
            cfg.sat_offset = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--duration") {
            cfg.duration_s = atof(require_value(i, argc, argv).c_str());
        } else if (arg == "--route-update-ms") {
            cfg.route_update_ms = atof(require_value(i, argc, argv).c_str());
        } else if (arg == "--queue-sample-ms") {
            cfg.queue_sample_ms = atof(require_value(i, argc, argv).c_str());
        } else if (arg == "--ping-interval-ms") {
            cfg.ping_interval_ms = atof(require_value(i, argc, argv).c_str());
        } else if (arg == "--uplink-mbps") {
            cfg.uplink_mbps = strtoull(require_value(i, argc, argv).c_str(), NULL, 10);
        } else if (arg == "--downlink-mbps") {
            cfg.downlink_mbps = strtoull(require_value(i, argc, argv).c_str(), NULL, 10);
        } else if (arg == "--isl-mbps") {
            cfg.isl_mbps = strtoull(require_value(i, argc, argv).c_str(), NULL, 10);
        } else if (arg == "--queue-pkts") {
            cfg.queue_pkts = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--packet-size") {
            cfg.packet_size_bytes = atoi(require_value(i, argc, argv).c_str());
        } else if (arg == "--src") {
            if (!parse_pair(require_value(i, argc, argv), cfg.src_lat, cfg.src_lon)) {
                cerr << "--src must have form LAT,LON" << endl;
                exit(1);
            }
        } else if (arg == "--dst") {
            if (!parse_pair(require_value(i, argc, argv), cfg.dst_lat, cfg.dst_lon)) {
                cerr << "--dst must have form LAT,LON" << endl;
                exit(1);
            }
        } else if (arg == "--out" || arg == "--prefix") {
            cfg.out_prefix = require_value(i, argc, argv);
        } else if (arg == "--routing-only") {
            cfg.routing_only = true;
        } else if (arg == "--verbose") {
            cfg.verbose = true;
        } else {
            cerr << "Unknown argument: " << arg << endl;
            usage(argv[0]);
            exit(1);
        }
    }

    if (cfg.route_update_ms <= 0 || cfg.duration_s <= 0) {
        cerr << "Duration and route update interval must be positive." << endl;
        exit(1);
    }

    return cfg;
}

static string hash_to_hex(unsigned long long h) {
    std::ostringstream out;
    out << std::hex << h;
    return out.str();
}

static RouteMetrics analyze_route(Route* route,
                                  Constellation& constellation,
                                  RouteState& state,
                                  simtime_picosec now,
                                  double dijkstra_cpu_ms) {
    RouteMetrics m;
    m.found = (route != NULL);
    m.rtt_ms = 0.0;
    m.one_way_latency_ms = 0.0;
    m.route_hash = "NO_ROUTE";
    m.changed = false;
    m.route_duration_s = 0.0;
    m.link_hops = 0;
    m.isl_hops = 0;
    m.ground_hops = 0;
    m.dijkstra_cpu_ms = dijkstra_cpu_ms;

    if (route) {
        simtime_picosec rtt = constellation.get_rtt(route);
        m.rtt_ms = timeAsMs(rtt);
        m.one_way_latency_ms = m.rtt_ms / 2.0;

        unsigned long long h = 1469598103934665603ULL;
        for (auto it = route->begin(); it != route->end(); ++it) {
            Link* link = dynamic_cast<Link*>(*it);
            if (link != NULL) {
                m.link_hops++;
                unsigned long long ptr = (unsigned long long)(uintptr_t)link;
                h ^= ptr;
                h *= 1099511628211ULL;
            }
        }
        m.route_hash = hash_to_hex(h);
        m.ground_hops = m.link_hops >= 2 ? 2 : m.link_hops;
        m.isl_hops = m.link_hops > 2 ? m.link_hops - 2 : 0;
    }

    m.changed = (m.route_hash != state.last_hash);
    if (m.changed) {
        m.route_duration_s = state.last_hash.empty() ? 0.0 : timeAsSec(now - state.last_change);
        state.last_hash = m.route_hash;
        state.last_change = now;
    } else {
        m.route_duration_s = timeAsSec(now - state.last_change);
    }

    return m;
}

static Route* timed_find_route(City& src, City& dst, simtime_picosec now, double& cpu_ms) {
    auto begin = std::chrono::high_resolution_clock::now();
    Route* route = src.find_route(dst, now);
    auto end = std::chrono::high_resolution_clock::now();
    cpu_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli> >(end - begin).count();
    return route;
}

static void log_visibility_pair(ExperimentLogger& metrics, simtime_picosec now, City& src, City& dst) {
    metrics.log_visibility(now, "src", src.active_uplink_count(), src.inactive_sat_count(), src.nearest_sat_distance());
    metrics.log_visibility(now, "dst", dst.active_uplink_count(), dst.inactive_sat_count(), dst.nearest_sat_distance());
}

static void log_route_pair(ExperimentLogger& metrics,
                           simtime_picosec now,
                           const string& direction,
                           const RouteMetrics& rm,
                           City& src,
                           City& dst,
                           const ExperimentConfig& cfg,
                           int num_sats) {
    metrics.log_route(now, direction, rm,
                      src.active_uplink_count(), dst.active_uplink_count(),
                      src.inactive_sat_count(), dst.inactive_sat_count(),
                      src.nearest_sat_distance(), dst.nearest_sat_distance(),
                      cfg, num_sats);
}

static void run_routing_only_scan(ExperimentLogger& metrics,
                                  Constellation& constellation,
                                  City& src,
                                  City& dst,
                                  const ExperimentConfig& cfg,
                                  simtime_picosec end_time) {
    RouteState out_state;
    RouteState back_state;
    simtime_picosec step = timeFromMs(cfg.route_update_ms);

    for (simtime_picosec now = 0; now <= end_time; now += step) {
        src.update_coordinates(now);
        dst.update_coordinates(now);
        src.update_uplinks(now);
        dst.update_uplinks(now);
        log_visibility_pair(metrics, now, src, dst);

        double cpu_out = 0.0;
        Route* out = timed_find_route(src, dst, now, cpu_out);
        RouteMetrics out_metrics = analyze_route(out, constellation, out_state, now, cpu_out);
        log_route_pair(metrics, now, "out", out_metrics, src, dst, cfg, constellation.num_sats());
        if (out) out->decr_refcount();

        double cpu_back = 0.0;
        Route* back = timed_find_route(dst, src, now, cpu_back);
        RouteMetrics back_metrics = analyze_route(back, constellation, back_state, now, cpu_back);
        log_route_pair(metrics, now, "back", back_metrics, dst, src, cfg, constellation.num_sats());
        if (back) back->decr_refcount();
    }
}

int main(int argc, char** argv) {
    ExperimentConfig cfg = parse_args(argc, argv);

    string mkdir_cmd = "mkdir -p \"" + cfg.out_prefix.substr(0, cfg.out_prefix.find_last_of('/')) + "\"";
    if (cfg.out_prefix.find('/') != string::npos) {
        system(mkdir_cmd.c_str());
    }

    EventList eventlist;
    const simtime_picosec END_TIME = timeFromSec(cfg.duration_s);
    eventlist.setEndtime(END_TIME);

    Logfile logfile((cfg.out_prefix + ".binlog").c_str(), eventlist);
    logfile.setStartTime(0);

    Link::_logfile = &logfile;
    Link::_queue_logger_sampling_interval = timeFromMs(cfg.queue_sample_ms);

    Constellation constellation(
        eventlist,
        speedFromMbps((uint64_t)cfg.uplink_mbps), memFromPkt(cfg.queue_pkts),
        speedFromMbps((uint64_t)cfg.downlink_mbps), memFromPkt(cfg.queue_pkts),
        speedFromMbps((uint64_t)cfg.isl_mbps), memFromPkt(cfg.queue_pkts),
        cfg.num_planes, cfg.sats_per_plane, cfg.orbital_slots,
        cfg.adjacent_sats, cfg.sat_offset
    );

    ExperimentLogger metrics(cfg.out_prefix);
    metrics.log_config(cfg, constellation.num_sats());

    City src(cfg.src_lat, cfg.src_lon, constellation);
    City dst(cfg.dst_lat, cfg.dst_lon, constellation);

    src.update_uplinks(0);
    dst.update_uplinks(0);
    log_visibility_pair(metrics, 0, src, dst);

    if (cfg.routing_only) {
        run_routing_only_scan(metrics, constellation, src, dst, cfg, END_TIME);
        metrics.log_summary("mode", "routing_only");
        return 0;
    }

    RouteState out_state;
    RouteState back_state;
    double cpu_out = 0.0;
    double cpu_back = 0.0;
    Route* rt_out = timed_find_route(src, dst, 0, cpu_out);
    Route* rt_back = timed_find_route(dst, src, 0, cpu_back);

    RouteMetrics out_metrics = analyze_route(rt_out, constellation, out_state, 0, cpu_out);
    RouteMetrics back_metrics = analyze_route(rt_back, constellation, back_state, 0, cpu_back);
    log_route_pair(metrics, 0, "out", out_metrics, src, dst, cfg, constellation.num_sats());
    log_route_pair(metrics, 0, "back", back_metrics, dst, src, cfg, constellation.num_sats());

    if (!rt_out || !rt_back) {
        if (cfg.verbose) {
            cerr << "Initial route unavailable. Falling back to routing-only scan." << endl;
        }
        if (rt_out) rt_out->decr_refcount();
        if (rt_back) rt_back->decr_refcount();
        run_routing_only_scan(metrics, constellation, src, dst, cfg, END_TIME);
        metrics.log_summary("mode", "routing_only_initial_route_unavailable");
        return 0;
    }

    PingClient pingclient(eventlist, timeFromMs(cfg.ping_interval_ms), cfg.packet_size_bytes);
    PingServer pingserver;

    rt_out->push_back(static_cast<PacketSink*>(&pingserver));
    rt_back->push_back(static_cast<PacketSink*>(&pingclient));

    pingclient.connect(*rt_out, *rt_back, pingserver, eventlist.now(), END_TIME);

    simtime_picosec last_route_update = eventlist.now();
    simtime_picosec route_update_interval = timeFromMs(cfg.route_update_ms);

    while (eventlist.doNextEvent()) {
        simtime_picosec now = eventlist.now();

        if (now >= END_TIME) break;

        if (now - last_route_update >= route_update_interval) {
            src.update_uplinks(now);
            dst.update_uplinks(now);
            log_visibility_pair(metrics, now, src, dst);

            double new_cpu_out = 0.0;
            double new_cpu_back = 0.0;
            Route* new_rt_out = timed_find_route(src, dst, now, new_cpu_out);
            Route* new_rt_back = timed_find_route(dst, src, now, new_cpu_back);

            RouteMetrics new_out_metrics = analyze_route(new_rt_out, constellation, out_state, now, new_cpu_out);
            RouteMetrics new_back_metrics = analyze_route(new_rt_back, constellation, back_state, now, new_cpu_back);
            log_route_pair(metrics, now, "out", new_out_metrics, src, dst, cfg, constellation.num_sats());
            log_route_pair(metrics, now, "back", new_back_metrics, dst, src, cfg, constellation.num_sats());

            if (new_rt_out && new_rt_back) {
                new_rt_out->push_back(static_cast<PacketSink*>(&pingserver));
                new_rt_back->push_back(static_cast<PacketSink*>(&pingclient));
                pingclient.update_route(*new_rt_out, *new_rt_back);

                rt_out->decr_refcount();
                rt_back->decr_refcount();
                rt_out = new_rt_out;
                rt_back = new_rt_back;
            } else {
                if (new_rt_out) new_rt_out->decr_refcount();
                if (new_rt_back) new_rt_back->decr_refcount();
            }

            last_route_update = now;
        }
    }

    if (rt_out) rt_out->decr_refcount();
    if (rt_back) rt_back->decr_refcount();
    metrics.log_summary("mode", "packet_ping");
    return 0;
}
