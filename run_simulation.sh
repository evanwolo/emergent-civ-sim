#!/bin/bash
# Script to run inside the container

# Create command file
cat > /tmp/sim_commands.txt << 'EOF'
run 100 10
metrics
economy
quit
EOF

# Run the simulation
/app/KernelSim /tmp/sim_commands.txt
