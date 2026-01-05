#!/bin/bash
# NRC Driver Memory Comparison Tool
# Compares memory usage before/after module load/unload

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script requires root privileges. Restarting with sudo..."
    exec sudo -E "$0" "$@"
fi

BEFORE="/tmp/nrc_slabinfo_before.txt"
AFTER="/tmp/nrc_slabinfo_after.txt"
FINAL="/tmp/nrc_slabinfo_final.txt"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== NRC Driver Memory Comparison Tool ===${NC}"
echo -e "${GREEN}Running as root (UID: $EUID)${NC}"
echo ""

# Step 1: Before snapshot
echo -e "${YELLOW}[Step 1/4] Taking BEFORE snapshot...${NC}"
cat /proc/slabinfo > $BEFORE
echo "  Saved to $BEFORE"
echo ""

# Step 2: Load modules
echo -e "${YELLOW}[Step 2/4] Loading modules with start_wlan.py...${NC}"
./start_wlan.py 0 0 US
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to load modules${NC}"
    exit 1
fi
echo "  Modules loaded successfully"
echo ""

# Wait 10 seconds
echo -e "${YELLOW}Waiting 10 seconds for system stabilization...${NC}"
sleep 10

# Run iperf test
echo -e "${YELLOW}Running iperf.sh test...${NC}"
./iperf.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to run iperf test${NC}"
    exit 1
fi
echo "  iperf test completed successfully"
echo ""

# After snapshot (while modules still loaded)
echo -e "${YELLOW}Taking AFTER snapshot...${NC}"
cat /proc/slabinfo > $AFTER
echo "  Saved to $AFTER"
echo ""

# Step 3: Compare
echo -e "${BLUE}=== Memory Differences (AFTER - BEFORE) ===${NC}"
echo ""
printf "%-30s | %10s | %15s\n" "Cache Name" "Objects Δ" "Memory Δ (KB)"
echo "--------------------------------------------------------------------"

join -1 1 -2 1 \
  <(awk 'NR>2 {print $1, $3, $5}' $BEFORE | sort) \
  <(awk 'NR>2 {print $1, $3, $5}' $AFTER | sort) | \
awk -v red="$RED" -v green="$GREEN" -v nc="$NC" '{
  obj_diff = $5 - $2
  size_diff = (obj_diff * $3) / 1024
  if (obj_diff != 0) {
    color = (obj_diff > 0) ? red : green
    sign = (obj_diff > 0) ? "+" : ""
    printf "%-30s | %s%+10d%s | %s%+14.2f%s KB\n",
           $1, color, obj_diff, nc, color, size_diff, nc
  }
}' | sort -t'|' -k3 -rn

echo ""

# Highlight important caches
echo -e "${BLUE}=== Key Memory Changes ===${NC}"
join -1 1 -2 1 \
  <(awk 'NR>2 {print $1, $3, $5}' $BEFORE | sort) \
  <(awk 'NR>2 {print $1, $3, $5}' $AFTER | sort) | \
grep -E "kmalloc-512|skbuff|nrc" | \
awk -v red="$RED" -v yellow="$YELLOW" -v nc="$NC" '{
  obj_diff = $5 - $2
  size_diff = (obj_diff * $3) / 1024
  if (obj_diff > 0) {
    printf "%s%-30s: +%d objects (+%.2f KB)%s\n",
           yellow, $1, obj_diff, size_diff, nc
  }
}'

echo ""

# Step 4: Unload and final check
echo -e "${YELLOW}[Step 4/4] Stopping modules with stop_modular.py...${NC}"
./stop_modular.py
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to stop modules${NC}"
    exit 1
fi
echo "  Modules stopped successfully"
echo ""

# Final snapshot
echo -e "${YELLOW}Taking FINAL snapshot...${NC}"
cat /proc/slabinfo > $FINAL
echo "  Saved to $FINAL"
echo ""

# Step 5: Leak detection
echo -e "${BLUE}=== Leak Detection (FINAL - BEFORE) ===${NC}"
echo ""
echo -e "Objects that ${RED}INCREASED${NC} after module unload indicate potential memory leaks:"
echo ""
printf "%-30s | %10s | %15s\n" "Cache Name" "Objects Δ" "Memory Δ (KB)"
echo "--------------------------------------------------------------------"

leak_found=0
join -1 1 -2 1 \
  <(awk 'NR>2 {print $1, $3, $5}' $BEFORE | sort) \
  <(awk 'NR>2 {print $1, $3, $5}' $FINAL | sort) | \
awk -v red="$RED" -v green="$GREEN" -v nc="$NC" -v leak_found="$leak_found" '{
  obj_diff = $5 - $2
  size_diff = (obj_diff * $3) / 1024
  if (obj_diff > 0) {
    printf "%s%-30s | %+10d | %+14.2f KB%s\n",
           red, $1, obj_diff, size_diff, nc
    leak_found = 1
  }
}
END {
  exit !leak_found
}'

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ No obvious memory leaks detected${NC}"
else
    echo ""
    echo -e "${RED}⚠ Potential memory leaks detected!${NC}"
    echo ""
    echo "Recommendations:"
    echo "  1. Check dmesg for memory-related errors"
    echo "  2. Enable KMEMLEAK: echo scan > /sys/kernel/debug/kmemleak"
    echo "  3. Review SKB allocations and frees"
    echo "  4. Check for uncancelled work queues and timers"
fi

echo ""

# Summary
echo -e "${BLUE}=== Summary ===${NC}"
echo ""

# kmalloc-512 specific
echo "kmalloc-512 Cache:"
before_512=$(grep "^kmalloc-512" $BEFORE | awk '{print $3}')
after_512=$(grep "^kmalloc-512" $AFTER | awk '{print $3}')
final_512=$(grep "^kmalloc-512" $FINAL | awk '{print $3}')

echo "  Before load:  $before_512 objects"
echo "  After load:   $after_512 objects (Δ $((after_512 - before_512)))"
echo "  After unload: $final_512 objects (Δ $((final_512 - before_512)))"

if [ $((final_512 - before_512)) -gt 0 ]; then
    echo -e "  ${RED}⚠ kmalloc-512 increased by $((final_512 - before_512)) objects${NC}"
else
    echo -e "  ${GREEN}✓ kmalloc-512 returned to baseline${NC}"
fi

echo ""
echo "Detailed reports saved:"
echo "  Before: $BEFORE"
echo "  After:  $AFTER"
echo "  Final:  $FINAL"
