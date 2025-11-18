#!/bin/bash
# Wrapper script to run simulation inside container

cd /app

# Run the simulation by passing commands directly
/app/KernelSim <<EOF
run 100 10
metrics
economy
quit
EOF
