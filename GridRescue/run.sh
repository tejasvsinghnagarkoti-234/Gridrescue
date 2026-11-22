#!/usr/bin/env sh
# Builds and starts the GridRescue WEB UI (default) or the console version (./run.sh console)
set -eu
cd "$(dirname "$0")"
SRC="GridManager.cpp GridJson.cpp Graph.cpp Algorithms.cpp FailureManager.cpp PowerManager.cpp PowerStation.cpp Substation.cpp TransmissionLine.cpp Consumer.cpp"
if [ "${1:-}" = "console" ]; then
  g++ -std=c++17 Main.cpp $SRC -o GridRescue
  exec ./GridRescue
fi
g++ -std=c++17 -O2 WebMain.cpp $SRC -o GridRescueWeb
PORT="${1:-8080}"
( sleep 1; (xdg-open "http://localhost:$PORT" || open "http://localhost:$PORT") >/dev/null 2>&1 || true ) &
exec ./GridRescueWeb "$PORT"
