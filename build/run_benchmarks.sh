#!/usr/bin/env bash
set -euo pipefail

# Benchmark runner for starlink-htsim/build.
# It intentionally does NOT use -DXCP_STATIC_NETWORK.
#
# Usage from starlink-htsim/build:
#   ./run_benchmarks.sh [results_dir]
#
# Optional env vars:
#   BIN=/path/to/starlink_exp
#   PARSE_OUTPUT=/path/to/parse_output
#   SKIP_MAKE=1
#   DEEP_CLEAN=1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
SRC_DIR="${PROJECT_ROOT}/src"

cd "${BUILD_DIR}"

RESULTS_DIR="${1:-results}"
BIN="${BIN:-${BUILD_DIR}/starlink_exp}"
PARSE_OUTPUT="${PARSE_OUTPUT:-${SRC_DIR}/parse_output}"
GETSTAT="${GETSTAT:-${BUILD_DIR}/getstat.sh}"
mkdir -p "${RESULTS_DIR}"

if [[ "${SKIP_MAKE:-0}" != "1" ]]; then
    if [[ "${DEEP_CLEAN:-0}" == "1" ]]; then
        make deep-clean
    fi
    "${BUILD_DIR}/build.sh"
fi

run_case() {
    local name="$1"
    shift
    local prefix="${RESULTS_DIR}/${name}"
    echo "==> ${name}"
    mkdir -p "$(dirname "${prefix}")"
    "${BIN}" --out "${prefix}" "$@" > "${prefix}.stdout.txt" 2> "${prefix}.stderr.txt"
    if [[ -f "${prefix}.binlog" ]]; then
        "${GETSTAT}" "${prefix}" "${PARSE_OUTPUT}" > "${prefix}.parse_stdout.txt" 2> "${prefix}.parse_stderr.txt" || true
    fi
}

# A. Minimal 2-satellite sanity benchmark.
# Uses adjacent FCC slots from a 66-slot plane, not two satellites spread over 180 degrees.
run_case "A_sanity_2sat_adjacent" \
    --planes 1 --sats-per-plane 2 --orbital-slots 66 --sat-selection adjacent \
    --src 0,0 --dst 0,5 \
    --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
    --routing-only

# B. Small scale growth benchmarks.
for sats in 4 8 16; do
    run_case "B_1plane_${sats}sat_adjacent" \
        --planes 1 --sats-per-plane "${sats}" --orbital-slots 66 --sat-selection adjacent \
        --src 0,0 --dst 0,10 \
        --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
        --routing-only
done

run_case "B_2planes_16sat" \
    --planes 2 --sats-per-plane 16 --orbital-slots 66 --sat-selection adjacent \
    --src 0,0 --dst 20,20 \
    --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
    --routing-only

run_case "B_4planes_16sat" \
    --planes 4 --sats-per-plane 16 --orbital-slots 66 --sat-selection adjacent \
    --src 0,0 --dst 30,30 \
    --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
    --routing-only

# C. Article-like partial-deployment RTT benchmarks: New York -> Seattle.
# Four hours are used for paper-style long-window plots; route update every 1s keeps runtime reasonable.
for planes in 6 12 24; do
    run_case "C_NY_Seattle_${planes}planes" \
        --planes "${planes}" --sats-per-plane 66 --orbital-slots 66 --sat-selection spread \
        --src 40.76,-73.98 --dst 47.61,-122.33 \
        --duration 14400 --route-update-ms 1000 --ping-interval-ms 1000 \
        --routing-only
done

# D. London -> New York with packet traffic enabled, useful for queue/binlog parsing.
# This version uses ISLs and no explicit ocean ground relays, because relay grids are not implemented yet.
for planes in 6 12 24; do
    run_case "D_London_NY_${planes}planes_ping" \
        --planes "${planes}" --sats-per-plane 66 --orbital-slots 66 --sat-selection spread \
        --src 51.5,0.0 --dst 40.76,-73.98 \
        --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
        --queue-sample-ms 10 --uplink-mbps 10000 --downlink-mbps 10000 --isl-mbps 10000
done

# E. Link-rate sensitivity. Keep topology fixed and vary ISL capacity.
for isl_mbps in 1000 5000 10000; do
    run_case "E_NY_Seattle_24planes_isl${isl_mbps}" \
        --planes 24 --sats-per-plane 66 --orbital-slots 66 --sat-selection spread \
        --src 40.76,-73.98 --dst 47.61,-122.33 \
        --duration 600 --route-update-ms 1000 --ping-interval-ms 1000 \
        --queue-sample-ms 10 --uplink-mbps 10000 --downlink-mbps 10000 --isl-mbps "${isl_mbps}"
done

echo "Done. Results in ${BUILD_DIR}/${RESULTS_DIR}/"
