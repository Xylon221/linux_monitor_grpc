#!/bin/bash
set -e
cd "$(dirname "$0")"

PROTO_DIR="../proto"
OUT_DIR="./pb"

mkdir -p "$OUT_DIR"
touch "$OUT_DIR/__init__.py"

python3 -m grpc_tools.protoc \
    --proto_path="$PROTO_DIR" \
    --python_out="$OUT_DIR" \
    --grpc_python_out="$OUT_DIR" \
    "$PROTO_DIR"/*.proto

echo "Proto files generated in $OUT_DIR"
