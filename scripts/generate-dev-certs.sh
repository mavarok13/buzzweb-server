#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CERTS_DIR="${ROOT_DIR}/certs"
CERT_FILE="${CERTS_DIR}/dev-cert.pem"
KEY_FILE="${CERTS_DIR}/dev-key.pem"

mkdir -p "${CERTS_DIR}"

if [[ -f "${CERT_FILE}" && -f "${KEY_FILE}" ]]; then
    echo "Dev certificate already exists: ${CERT_FILE}"
    echo "Dev private key already exists: ${KEY_FILE}"
    exit 0
fi

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "${KEY_FILE}" \
    -out "${CERT_FILE}" \
    -days 365 \
    -subj "/CN=localhost"

echo "Created dev certificate: ${CERT_FILE}"
echo "Created dev private key: ${KEY_FILE}"
