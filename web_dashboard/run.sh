#!/bin/bash
set -e
cd "$(dirname "$0")"

pip install -q -r requirements.txt
bash gen_proto.sh
echo "Starting web dashboard at http://localhost:8000"
python3 main.py
