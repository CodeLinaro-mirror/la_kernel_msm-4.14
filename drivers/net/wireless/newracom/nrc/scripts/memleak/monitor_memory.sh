#!/bin/bash
# NRC Driver Memory Monitor
# Usage: ./monitor_memory.sh [interval_seconds]
# Note: Requires root privileges for debugfs access

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script requires root privileges for debugfs access."
    echo "Re-running with sudo..."
    exec sudo "$0" "$@"
fi

INTERVAL=${1:-2}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=== NRC Driver Memory Monitor ==="
echo "Monitoring interval: ${INTERVAL} seconds"
echo ""
echo "Press Ctrl+C to stop"
echo ""

# Track core module state to detect when it gets loaded
core_module_was_loaded=false
prev_kmalloc512=0

# Function to enable SKB debug
enable_skb_debug() {
    local SKB_DEBUG_FILE="/sys/kernel/debug/nrc_core/skb_debug"

    if [ -f "$SKB_DEBUG_FILE" ]; then
        current_status=$(cat "$SKB_DEBUG_FILE" 2>/dev/null)
        if [ "$current_status" != "Y" ]; then
            echo -e "${BLUE}Enabling SKB debug for detailed statistics...${NC}"
            echo "Y" > "$SKB_DEBUG_FILE" 2>/dev/null
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}✓ SKB debug enabled${NC}"
                echo ""
                return 0
            else
                echo -e "${YELLOW}⚠ Failed to enable SKB debug${NC}"
                echo ""
                return 1
            fi
        fi
    fi
    return 1
}

while true; do
    clear
    echo -e "${GREEN}=== NRC Driver Memory Monitor ===${NC}"
    echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""

    # kmalloc-512 상태
    echo -e "${YELLOW}=== kmalloc-512 Cache ===${NC}"
    kmalloc512_line=$(grep "^kmalloc-512" /proc/slabinfo)
    if [ -n "$kmalloc512_line" ]; then
        kmalloc512_active=$(echo $kmalloc512_line | awk '{print $3}')
        kmalloc512_total=$(echo $kmalloc512_line | awk '{print $4}')
        kmalloc512_size=$(echo $kmalloc512_line | awk '{print $5}')
        kmalloc512_mem=$((kmalloc512_active * kmalloc512_size / 1024))

        echo "Active objects: $kmalloc512_active / $kmalloc512_total"
        echo "Object size: $kmalloc512_size bytes"
        echo "Memory used: $kmalloc512_mem KB"

        # 증가 감지
        if [ $prev_kmalloc512 -gt 0 ]; then
            diff=$((kmalloc512_active - prev_kmalloc512))
            if [ $diff -gt 0 ]; then
                echo -e "${RED}▲ Increased by $diff objects${NC}"
            elif [ $diff -lt 0 ]; then
                echo -e "${GREEN}▼ Decreased by ${diff#-} objects${NC}"
            else
                echo -e "${GREEN}= No change${NC}"
            fi
        fi
        prev_kmalloc512=$kmalloc512_active
    fi
    echo ""

    # 다른 kmalloc 캐시들
    echo -e "${YELLOW}=== Other kmalloc Caches ===${NC}"
    grep "^kmalloc-" /proc/slabinfo | grep -v kmalloc-512 | \
        awk '{printf "%-20s: %8d active / %8d total (%d bytes) = %8d KB\n",
              $1, $3, $4, $5, $3*$5/1024}' | sort -k5 -rn | head -10
    echo ""

    # SKB 버퍼
    echo -e "${YELLOW}=== SKB Buffers ===${NC}"
    grep "skbuff" /proc/slabinfo | \
        awk '{printf "%-30s: %8d active / %8d total = %8d KB\n",
              $1, $3, $4, $3*$5/1024}'
    echo ""

    # 전체 SLAB 메모리
    echo -e "${YELLOW}=== Total SLAB Memory ===${NC}"
    total_kb=$(awk 'NR>2 {sum+=$3*$5} END {print int(sum/1024)}' /proc/slabinfo)
    total_mb=$(echo "scale=2; $total_kb / 1024" | bc)
    echo "Total SLAB: ${total_kb} KB (${total_mb} MB)"
    echo ""

    # NRC 관련 캐시 (있다면)
    nrc_caches=$(grep "^nrc" /proc/slabinfo 2>/dev/null)
    if [ -n "$nrc_caches" ]; then
        echo -e "${YELLOW}=== NRC Custom Caches ===${NC}"
        echo "$nrc_caches" | \
            awk '{printf "%-30s: %8d active / %8d total = %8d KB\n",
                  $1, $3, $4, $3*$5/1024}'
        echo ""
    fi

    # 모듈이 로드되어 있는지 확인
    echo -e "${YELLOW}=== NRC Modules Status ===${NC}"
    wlan_loaded=false
    mcp_loaded=false
    core_loaded=false

    if lsmod | grep -q "^nrc_wlan"; then
        echo -e "${GREEN}✓${NC} nrc_wlan loaded"
        wlan_loaded=true
    else
        echo -e "${RED}✗${NC} nrc_wlan not loaded"
    fi

    if lsmod | grep -q "^nrc_mcp"; then
        echo -e "${GREEN}✓${NC} nrc_mcp loaded"
        mcp_loaded=true
    else
        echo -e "${RED}✗${NC} nrc_mcp not loaded"
    fi

    if lsmod | grep -q "^nrc_core"; then
        echo -e "${GREEN}✓${NC} nrc_core loaded"
        core_loaded=true
    else
        echo -e "${RED}✗${NC} nrc_core not loaded"
    fi
    if lsmod | grep -q "^nrc_spi"; then
        echo -e "${GREEN}✓${NC} nrc_spi loaded"
    else
        echo -e "${RED}✗${NC} nrc_spi not loaded"
    fi
    echo ""

    # Auto-enable SKB debug when core module is newly loaded
    if [ "$core_loaded" = true ] && [ "$core_module_was_loaded" = false ]; then
        echo -e "${BLUE}>>> Core module detected! Auto-enabling SKB debug...${NC}"
        enable_skb_debug
        core_module_was_loaded=true
    elif [ "$core_loaded" = false ]; then
        core_module_was_loaded=false
    fi

    # SKB Statistics (debugfs)
    # Use centralized core debugfs
    SKB_STATS_FILE="/sys/kernel/debug/nrc_core/skb_stats"
    SKB_STATS_SOURCE="Core"

    if [ -f "$SKB_STATS_FILE" ]; then
        echo -e "${YELLOW}=== Driver SKB Statistics (${SKB_STATS_SOURCE}) ===${NC}"
        cat "$SKB_STATS_FILE" | while IFS= read -r line; do
            # Highlight leak values
            if echo "$line" | grep -q "Leak="; then
                leak=$(echo "$line" | sed -n 's/.*Leak=\([0-9]*\).*/\1/p')
                if [ -n "$leak" ] && [ "$leak" -gt 0 ]; then
                    echo -e "${RED}$line${NC}"
                else
                    echo "$line"
                fi
            elif echo "$line" | grep -qE "^[A-Z].*[0-9]"; then
                # Type lines with leak > 0
                leak=$(echo "$line" | awk '{print $4}')
                if [ -n "$leak" ] && [ "$leak" -gt 0 ] 2>/dev/null; then
                    echo -e "${RED}$line${NC}"
                else
                    echo "$line"
                fi
            else
                echo "$line"
            fi
        done
        echo ""
    else
        echo -e "${RED}✗ nrc_core debugfs not available${NC}"
        echo "  Expected: /sys/kernel/debug/nrc_core/skb_stats"
        echo ""
    fi

    sleep $INTERVAL
done
