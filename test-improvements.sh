#!/bin/bash
# Simple test script for architecture improvements

echo "=== Testing Architecture Improvements ===" >&2
echo "" >&2

# Run with commands via heredoc
/app/KernelSim <<EOF
run 100 100
metrics
economy
quit
EOF

echo "" >&2
echo "=== Test Complete ===" >&2
