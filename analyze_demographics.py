#!/usr/bin/env python3
"""
Analyze demographic patterns from Grand Strategy Kernel simulation.
Run a 1000-tick simulation and extract demographic metrics.
"""

import subprocess
import re
import json
from collections import defaultdict

def run_simulation(ticks=1000, log_interval=100):
    """Run the simulation via Docker and capture output."""
    
    # Create input commands
    commands = f"""run {ticks} {log_interval}
metrics
economy
quit
"""
    
    # Run Docker container with commands
    try:
        result = subprocess.run(
            ['docker', 'run', '--rm', '-i', 'grand-strategy-kernel:latest', 'KernelSim'],
            input=commands,
            capture_output=True,
            text=True,
            timeout=120
        )
        return result.stdout
    except subprocess.TimeoutExpired:
        print("Simulation timed out after 120 seconds")
        return None
    except Exception as e:
        print(f"Error running simulation: {e}")
        return None

def parse_metrics_output(output):
    """Extract metrics from simulation output."""
    
    metrics = {
        'ticks': [],
        'population': [],
        'welfare': [],
        'inequality': [],
        'hardship': [],
        'polarization': [],
        'trade_volume': []
    }
    
    # Pattern for log output lines
    # Tick 100: Pop=49523, Pol=0.234, Welfare=0.95, Ineq=0.28, Trade=23456
    pattern = r'Tick (\d+).*?Pop=(\d+).*?Pol=([\d.]+).*?Welfare=([\d.]+).*?Ineq=([\d.]+).*?(?:Trade=(\d+))?'
    
    for line in output.split('\n'):
        match = re.search(pattern, line)
        if match:
            tick, pop, pol, welfare, ineq = match.groups()[:5]
            trade = match.group(6) if len(match.groups()) > 5 else '0'
            
            metrics['ticks'].append(int(tick))
            metrics['population'].append(int(pop))
            metrics['polarization'].append(float(pol))
            metrics['welfare'].append(float(welfare))
            metrics['inequality'].append(float(ineq))
            metrics['trade_volume'].append(int(trade) if trade else 0)
    
    return metrics

def analyze_demographics(metrics):
    """Analyze demographic patterns."""
    
    if not metrics['ticks']:
        print("No metrics found in output")
        return
    
    print("\n" + "="*60)
    print("DEMOGRAPHIC ANALYSIS")
    print("="*60)
    
    # Population dynamics
    initial_pop = metrics['population'][0]
    final_pop = metrics['population'][-1]
    pop_change = final_pop - initial_pop
    pop_change_pct = (pop_change / initial_pop) * 100
    
    print(f"\n📊 POPULATION DYNAMICS")
    print(f"   Initial: {initial_pop:,}")
    print(f"   Final:   {final_pop:,}")
    print(f"   Change:  {pop_change:+,} ({pop_change_pct:+.1f}%)")
    
    # Calculate growth rate
    ticks = metrics['ticks'][-1] - metrics['ticks'][0]
    annual_growth = (pop_change_pct / ticks) * 10  # Assuming 10 ticks per year
    print(f"   Annual Growth Rate: {annual_growth:+.2f}%")
    
    # Economic patterns
    avg_welfare = sum(metrics['welfare']) / len(metrics['welfare'])
    avg_inequality = sum(metrics['inequality']) / len(metrics['inequality'])
    avg_hardship = sum(metrics['hardship']) / len(metrics['hardship']) if metrics['hardship'] else 0
    
    print(f"\n💰 ECONOMIC INDICATORS")
    print(f"   Avg Welfare:    {avg_welfare:.3f}")
    print(f"   Avg Inequality: {avg_inequality:.3f}")
    print(f"   Avg Hardship:   {avg_hardship:.3f}")
    
    # Trade activity
    if metrics['trade_volume'] and any(metrics['trade_volume']):
        avg_trade = sum(metrics['trade_volume']) / len(metrics['trade_volume'])
        print(f"   Avg Trade Volume: {avg_trade:,.0f}")
    
    # Social polarization
    avg_polarization = sum(metrics['polarization']) / len(metrics['polarization'])
    print(f"\n🗣️  SOCIAL COHESION")
    print(f"   Avg Polarization: {avg_polarization:.3f}")
    
    # Trends
    print(f"\n📈 TRENDS (First → Last)")
    welfare_trend = metrics['welfare'][-1] - metrics['welfare'][0]
    ineq_trend = metrics['inequality'][-1] - metrics['inequality'][0]
    
    print(f"   Welfare:    {metrics['welfare'][0]:.3f} → {metrics['welfare'][-1]:.3f} ({welfare_trend:+.3f})")
    print(f"   Inequality: {metrics['inequality'][0]:.3f} → {metrics['inequality'][-1]:.3f} ({ineq_trend:+.3f})")
    
    print("\n" + "="*60)

def main():
    print("Starting Grand Strategy Kernel simulation...")
    print("Running 1000 ticks with demographic enhancements enabled...")
    
    output = run_simulation(ticks=1000, log_interval=100)
    
    if not output:
        print("Failed to get simulation output")
        return
    
    # Save raw output
    with open('simulation_output.txt', 'w') as f:
        f.write(output)
    print("\n✓ Raw output saved to simulation_output.txt")
    
    # Parse and analyze
    metrics = parse_metrics_output(output)
    analyze_demographics(metrics)
    
    # Save metrics as JSON
    with open('simulation_metrics.json', 'w') as f:
        json.dump(metrics, f, indent=2)
    print("\n✓ Metrics saved to simulation_metrics.json")

if __name__ == '__main__':
    main()
