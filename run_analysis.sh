#!/bin/bash
# Run simulation and analyze output inside Docker container

echo "=== GRAND STRATEGY KERNEL DEMOGRAPHIC ANALYSIS ==="
echo ""
echo "Running 1000-tick simulation with demographics enabled..."
echo ""

# Create temp file with commands
cat > /tmp/sim_commands.txt << 'EOF'
run 1000 100
metrics
economy
quit
EOF

# Run simulation and capture output
/app/KernelSim < /tmp/sim_commands.txt 2>&1 | tee /tmp/sim_output.txt

echo ""
echo "=== ANALYZING OUTPUT ==="
echo ""

# Extract key metrics using grep and awk
echo "📊 POPULATION METRICS:"
echo ""
grep -E "Tick [0-9]+.*Pop=" /tmp/sim_output.txt | head -1 | sed 's/.*Pop=\([0-9]*\).*/  Initial Population: \1/'
grep -E "Tick [0-9]+.*Pop=" /tmp/sim_output.txt | tail -1 | sed 's/.*Pop=\([0-9]*\).*/  Final Population:   \1/'

echo ""
echo "💰 ECONOMIC INDICATORS (Final):"
echo ""
grep -E "Tick [0-9]+.*Welfare=" /tmp/sim_output.txt | tail -1 | sed 's/.*Welfare=\([0-9.]*\).*/  Welfare:    \1/'
grep -E "Tick [0-9]+.*Ineq=" /tmp/sim_output.txt | tail -1 | sed 's/.*Ineq=\([0-9.]*\).*/  Inequality: \1/'
grep -E "Tick [0-9]+.*Hard=" /tmp/sim_output.txt | tail -1 | sed 's/.*Hard=\([0-9.]*\).*/  Hardship:   \1/'

echo ""
echo "📈 TRADE ACTIVITY (Final):"
echo ""
grep -E "Tick [0-9]+.*Trade=" /tmp/sim_output.txt | tail -1 | sed 's/.*Trade=\([0-9]*\).*/  Volume: \1/'

echo ""
echo "=== DETAILED OUTPUT SAMPLES ==="
echo ""
echo "First 3 tick samples:"
grep -E "Tick [0-9]+.*Pop=" /tmp/sim_output.txt | head -3
echo ""
echo "Last 3 tick samples:"
grep -E "Tick [0-9]+.*Pop=" /tmp/sim_output.txt | tail -3

echo ""
echo "=== FULL OUTPUT ==="
cat /tmp/sim_output.txt
