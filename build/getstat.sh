#!/usr/bin/env bash
set -euo pipefail

# Usage from starlink-htsim/build:
#   ./getstat.sh results/run_prefix [../src/parse_output]
#
# It reads PREFIX.binlog and creates:
#   PREFIX.ascii.txt
#   PREFIX.events.csv
#   PREFIX.queue.csv
#   PREFIX.xcp_sink.txt
#   PREFIX.tcp_sink.txt
#   PREFIX.queue_ascii.txt
#   PREFIX.tcp_sink_summary.txt
#   PREFIX.queue_summary.txt

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_PARSE_OUTPUT="${SCRIPT_DIR}/../src/parse_output"

PREFIX="${1:-results/run}"
PARSE_OUTPUT="${2:-${PARSE_OUTPUT:-${DEFAULT_PARSE_OUTPUT}}}"
BINLOG="${PREFIX}.binlog"

if [[ ! -f "${BINLOG}" ]]; then
    echo "Missing binary log: ${BINLOG}" >&2
    exit 1
fi

if [[ ! -x "${PARSE_OUTPUT}" ]]; then
    echo "Missing executable parse_output: ${PARSE_OUTPUT}" >&2
    echo "Run ./build.sh first, or pass the parser path as the second argument." >&2
    exit 1
fi

"${PARSE_OUTPUT}" "${BINLOG}" -ascii > "${PREFIX}.ascii.txt"
"${PARSE_OUTPUT}" "${BINLOG}" -csv > "${PREFIX}.events.csv"
"${PARSE_OUTPUT}" "${BINLOG}" -queue-csv > "${PREFIX}.queue.csv"

# These files may be empty for ping/routing-only experiments; keep stable names.
grep 'XCP_SINK' "${PREFIX}.ascii.txt" > "${PREFIX}.xcp_sink.txt" || true
grep 'TCP_SINK' "${PREFIX}.ascii.txt" > "${PREFIX}.tcp_sink.txt" || true
grep 'QUEUE' "${PREFIX}.ascii.txt" > "${PREFIX}.queue_ascii.txt" || true

"${PARSE_OUTPUT}" "${BINLOG}" -tcp > "${PREFIX}.tcp_sink_summary.txt" || true
"${PARSE_OUTPUT}" "${BINLOG}" -queue > "${PREFIX}.queue_summary.txt" || true

echo "Parsed ${BINLOG}"
echo "  ${PREFIX}.routes.csv and ${PREFIX}.visibility.csv are produced by starlink_exp"
echo "  ${PREFIX}.events.csv and ${PREFIX}.queue.csv are produced by parse_output"
