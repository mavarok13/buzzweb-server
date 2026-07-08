#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="${BUZZWEB_IMAGE_NAME:-buzzweb-server}"

docker build -t "${IMAGE_NAME}" "${ROOT_DIR}"
