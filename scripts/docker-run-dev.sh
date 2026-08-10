#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="${BUZZWEB_IMAGE_NAME:-buzzweb-server}"
PORT="${BUZZWEB_SERVER_PORT:-9291}"
CERTS_DIR="${ROOT_DIR}/certs"

if [[ ! -f "${CERTS_DIR}/dev-cert.pem" || ! -f "${CERTS_DIR}/dev-key.pem" ]]; then
    "${ROOT_DIR}/scripts/generate-dev-certs.sh"
fi

HOST_CERTS_DIR="${CERTS_DIR}"
if command -v cygpath >/dev/null 2>&1; then
    HOST_CERTS_DIR="$(cygpath -w "${CERTS_DIR}")"
fi

MSYS_NO_PATHCONV=1 docker run --rm \
    -p "${PORT}:${PORT}" \
    -e BUZZWEB_CERTIFICATE_FILE_PATH=/certs/dev-cert.pem \
    -e BUZZWEB_PRIVATE_KEY_PATH=/certs/dev-key.pem \
    -e BUZZWEB_SERVER_PORT="${PORT}" \
    -v "${HOST_CERTS_DIR}:/certs:ro" \
    "${IMAGE_NAME}"
