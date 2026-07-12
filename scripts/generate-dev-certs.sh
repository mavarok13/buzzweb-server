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

if [[ $# -gt 0 ]]; then
    CERT_NAMES=("$@")
else
    CERT_NAMES=(localhost 127.0.0.1 192.168.0.131)
fi

if command -v mkcert >/dev/null 2>&1; then
    echo "Using mkcert to create trusted local development certificate."
    TRUST_STORES="${TRUST_STORES:-system}" mkcert -install
    mkcert \
        -cert-file "${CERT_FILE}" \
        -key-file "${KEY_FILE}" \
        "${CERT_NAMES[@]}"
else
    echo "mkcert was not found. Falling back to openssl self-signed certificate."
    echo "Browsers/devices will not trust this certificate unless you trust it manually."

    SAN=""
    for name in "${CERT_NAMES[@]}"; do
        if [[ "${name}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            entry="IP:${name}"
        else
            entry="DNS:${name}"
        fi

        if [[ -z "${SAN}" ]]; then
            SAN="${entry}"
        else
            SAN="${SAN},${entry}"
        fi
    done

    openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout "${KEY_FILE}" \
        -out "${CERT_FILE}" \
        -days 365 \
        -subj "/CN=localhost" \
        -addext "subjectAltName=${SAN}"
fi

echo "Created dev certificate: ${CERT_FILE}"
echo "Created dev private key: ${KEY_FILE}"