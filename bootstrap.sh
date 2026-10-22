#!/usr/bin/env bash
# Protean bootstrap for Linux: fetches premake, generates makefiles and builds.
#
#   ./bootstrap.sh [debug|development|shipping] [--generate-only]
#
# Requires: g++ (C++17), make, SDL2 development headers (libsdl2-dev).
# premake 5.0.0+ is expected at build/premake5 and downloaded if missing.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG=development
GENERATE_ONLY=0
PREMAKE_VERSION=5.0.0
PREMAKE="$ROOT/build/premake5"

for arg in "$@"; do
	case "${arg,,}" in
		debug|development|shipping) CONFIG="${arg,,}" ;;
		--generate-only) GENERATE_ONLY=1 ;;
		*) echo "Unknown argument: $arg"; exit 1 ;;
	esac
done

if [ ! -x "$PREMAKE" ]; then
	echo "[Protean] Downloading premake $PREMAKE_VERSION..."
	curl -fsSL "https://github.com/premake/premake-core/releases/download/v$PREMAKE_VERSION/premake-$PREMAKE_VERSION-linux.tar.gz" | tar -xz -C "$ROOT/build" premake5
	chmod +x "$PREMAKE"
fi

echo "[Protean] Generating makefiles..."
(cd "$ROOT/build" && "$PREMAKE" gmake)

if [ "$GENERATE_ONLY" = 1 ]; then
	echo "[Protean] Makefiles generated: build/projects/Makefile"
	exit 0
fi

echo "[Protean] Building $CONFIG..."
make -C "$ROOT/build/projects" -j"$(nproc)" config="${CONFIG}_x64"
echo "[Protean] Done: binaries/linux-x64-$CONFIG/"
