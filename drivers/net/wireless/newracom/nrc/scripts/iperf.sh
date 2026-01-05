#!/bin/bash

# WiFi iperf test script with real-time output and reverse mode

INTERFACE="wlan0"
IPERF_PORT=5201
TEST_DURATION=10
UDP_BANDWIDTH="10M"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Get my IP address
get_my_ip() {
    ip addr show $INTERFACE | grep 'inet ' | awk '{print $2}' | cut -d/ -f1 | head -1
}

# Check if peer is routed through wireless interface
is_wireless_peer() {
    local peer="$1"
    # Get the route device for this peer
    local route_dev=$(ip route get "$peer" 2>/dev/null | grep -o 'dev [^ ]*' | awk '{print $2}')
    [[ "$route_dev" == "$INTERFACE" ]]
}

# Discover peers (wireless LAN only)
discover_peers() {
    local my_ip=$(get_my_ip)

    # First try: Get peers from ARP table for the specific wireless interface
    local arp_peers=$(ip neigh show dev $INTERFACE | grep -v FAILED | grep -v INCOMPLETE | awk '{print $1}' | grep -v "$my_ip" | sort -u)

    # Filter peers by routing table (ensure they route through wireless interface)
    local peers=""
    for peer in $arp_peers; do
        if is_wireless_peer "$peer"; then
            peers="$peers $peer"
        fi
    done

    # Second try: Active ping sweep through wireless interface only
    if [[ -z "$peers" ]]; then
        log_info "No peers in ARP cache, performing ping sweep on $INTERFACE..."
        local subnet=$(echo "$my_ip" | cut -d. -f1-3)
        local found_peers=""

        for i in {2..254}; do
            local target="$subnet.$i"
            if [[ "$target" != "$my_ip" ]]; then
                # Use -I to force ping through wireless interface only
                if ping -I $INTERFACE -c 1 -W 1 "$target" >/dev/null 2>&1; then
                    # Verify the peer is routed through wireless interface
                    if is_wireless_peer "$target"; then
                        found_peers="$found_peers $target"
                    fi
                fi
            fi
        done
        peers=$found_peers
    fi

    echo "$peers"
}

# Check if port is open
check_port() {
    local peer="$1"
    local port="$2"
    timeout 3 bash -c "</dev/tcp/$peer/$port" 2>/dev/null
}

# Run iperf test with real-time output
test_peer() {
    local peer="$1"
    local mode="$2"
    
    log_info "Testing $peer in $mode mode"
    
    case $mode in
        "client")
            # TCP TX test (normal direction)
            echo -e "${CYAN}=== TCP TX Test (Client -> Server) ===${NC}"
            log_info "iperf3 -c $peer -p $IPERF_PORT -t $TEST_DURATION"
            iperf3 -c "$peer" -p $IPERF_PORT -t $TEST_DURATION
            echo
            
            sleep 2
            
            # TCP RX test (reverse direction)
            echo -e "${CYAN}=== TCP RX Test (Server -> Client) ===${NC}"
            log_info "iperf3 -c $peer -p $IPERF_PORT -t $TEST_DURATION -R"
            iperf3 -c "$peer" -p $IPERF_PORT -t $TEST_DURATION -R
            echo
            
            sleep 2
            
            # UDP TX test
            echo -e "${CYAN}=== UDP TX Test (Client -> Server) ===${NC}"
            log_info "iperf3 -c $peer -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION"
            iperf3 -c "$peer" -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION
            echo
            
            sleep 2
            
            # UDP RX test
            echo -e "${CYAN}=== UDP RX Test (Server -> Client) ===${NC}"
            log_info "iperf3 -c $peer -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION -R"
            iperf3 -c "$peer" -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION -R
            echo
            ;;
        "server")
            log_info "Starting iperf3 server on port $IPERF_PORT"
            log_warning "Run these commands on peer devices:"
            echo "  TCP TX: iperf3 -c $(get_my_ip) -p $IPERF_PORT -t $TEST_DURATION"
            echo "  TCP RX: iperf3 -c $(get_my_ip) -p $IPERF_PORT -t $TEST_DURATION -R"
            echo "  UDP TX: iperf3 -c $(get_my_ip) -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION"
            echo "  UDP RX: iperf3 -c $(get_my_ip) -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION -R"
            echo
            iperf3 -s -p $IPERF_PORT
            ;;
        "tx-only")
            # Only TX tests
            echo -e "${CYAN}=== TCP TX Test ===${NC}"
            iperf3 -c "$peer" -p $IPERF_PORT -t $TEST_DURATION
            echo
            
            sleep 2
            
            echo -e "${CYAN}=== UDP TX Test ===${NC}"
            iperf3 -c "$peer" -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION
            echo
            ;;
        "rx-only")
            # Only RX tests
            echo -e "${CYAN}=== TCP RX Test ===${NC}"
            iperf3 -c "$peer" -p $IPERF_PORT -t $TEST_DURATION -R
            echo
            
            sleep 2
            
            echo -e "${CYAN}=== UDP RX Test ===${NC}"
            iperf3 -c "$peer" -p $IPERF_PORT -u -b $UDP_BANDWIDTH -t $TEST_DURATION -R
            echo
            ;;
    esac
}

# Diagnostics
run_diagnostics() {
    local peer="$1"

    log_info "Network diagnostics for $peer (via $INTERFACE)"

    echo "Wireless Interface Info:"
    ip link show $INTERFACE | head -2
    echo ""

    echo "Ping test (via $INTERFACE):"
    ping -I $INTERFACE -c 5 "$peer" | tail -1
    echo ""

    echo "Neighbor info (on $INTERFACE):"
    ip neigh show dev $INTERFACE | grep "$peer"
    if [ $? -ne 0 ]; then
        echo "  WARNING: $peer not found on $INTERFACE interface"
        echo "  This peer may be on a different interface (e.g., eth0)"
    fi
    echo ""

    echo "Route to $peer:"
    ip route get "$peer"
    echo ""

    echo "Port $IPERF_PORT check:"
    if check_port "$peer" $IPERF_PORT; then
        echo "  Port $IPERF_PORT: OPEN"
    else
        echo "  Port $IPERF_PORT: CLOSED"
    fi

    echo
}

# Print usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h          Show help"
    echo "  -l          List peers only"
    echo "  -c PEER     Test as client to PEER (TX+RX)"
    echo "  -tx PEER    TX tests only to PEER"
    echo "  -rx PEER    RX tests only to PEER"
    echo "  -s          Start server mode"
    echo "  -d PEER     Run diagnostics for PEER"
    echo "  -a          Auto test all peers (TX+RX)"
    echo "  -t SEC      Test duration (default: 10)"
    echo "  -b BW       UDP bandwidth (default: 10M)"
    echo ""
    echo "Examples:"
    echo "  $0 -l                    # List peers"
    echo "  $0 -c 192.168.200.35     # Full test (TX+RX)"
    echo "  $0 -tx 192.168.200.35    # TX tests only"
    echo "  $0 -rx 192.168.200.35    # RX tests only"
    echo "  $0 -s                    # Server mode"
    echo "  $0 -a                    # Auto test all peers"
    echo "  $0 -a -t 30 -b 50M       # 30s tests, 50Mbps UDP"
}

# Main
main() {
    echo "=== WiFi iperf Test Script ==="
    echo "Interface: $INTERFACE (port $IPERF_PORT)"
    echo "Test duration: ${TEST_DURATION}s, UDP bandwidth: $UDP_BANDWIDTH"

    # Check if wireless interface exists
    if ! ip link show $INTERFACE &>/dev/null; then
        log_error "Wireless interface $INTERFACE not found"
        log_info "Available interfaces: $(ip -o link show | awk -F': ' '{print $2}' | tr '\n' ' ')"
        exit 1
    fi

    # Check if wireless interface is UP
    if ! ip link show $INTERFACE | grep -q "state UP"; then
        log_warning "Wireless interface $INTERFACE is not UP"
        log_info "Try: sudo ip link set $INTERFACE up"
    fi

    echo

    local my_ip=$(get_my_ip)
    if [[ -z "$my_ip" ]]; then
        log_error "No IP address found for $INTERFACE"
        log_info "Check if $INTERFACE is connected to a wireless network"
        exit 1
    fi
    
    local mode=""
    local target_peer=""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            "-h"|"--help")
                usage
                exit 0
                ;;
            "-l"|"--list")
                mode="list"
                shift
                ;;
            "-c"|"--client")
                mode="client"
                target_peer="$2"
                shift 2
                ;;
            "-tx"|"--tx-only")
                mode="tx-only"
                target_peer="$2"
                shift 2
                ;;
            "-rx"|"--rx-only")
                mode="rx-only"
                target_peer="$2"
                shift 2
                ;;
            "-s"|"--server")
                mode="server"
                shift
                ;;
            "-d"|"--diag")
                mode="diag"
                target_peer="$2"
                shift 2
                ;;
            "-a"|"--auto")
                mode="auto"
                shift
                ;;
            "-t"|"--time")
                TEST_DURATION="$2"
                shift 2
                ;;
            "-b"|"--bandwidth")
                UDP_BANDWIDTH="$2"
                shift 2
                ;;
            *)
                if [[ -z "$mode" ]]; then
                    mode="auto"
                fi
                break
                ;;
        esac
    done
    
    # Default to auto if no mode specified
    if [[ -z "$mode" ]]; then
        mode="auto"
    fi
    
    case "$mode" in
        "list")
            log_info "My IP: $my_ip"
            local peers=$(discover_peers)
            if [[ -n "$peers" ]]; then
                log_success "Found peers: $peers"
                for peer in $peers; do
                    echo "  - $peer"
                done
            else
                log_warning "No peers found"
            fi
            ;;
        "client"|"tx-only"|"rx-only")
            if [[ -z "$target_peer" ]]; then
                log_error "Please specify peer IP"
                exit 1
            fi
            test_peer "$target_peer" "$mode"
            ;;
        "server")
            test_peer "" "server"
            ;;
        "diag")
            if [[ -z "$target_peer" ]]; then
                log_error "Please specify peer IP"
                exit 1
            fi
            run_diagnostics "$target_peer"
            ;;
        "auto")
            log_info "My IP: $my_ip"
            local peers=$(discover_peers)
            if [[ -z "$peers" ]]; then
                log_warning "No peers found"
                exit 1
            fi
            
            log_success "Found peers: $peers"
            echo
            
            for peer in $peers; do
                echo "======================================"
                echo "Testing $peer"
                echo "======================================"
                
                if check_port "$peer" $IPERF_PORT; then
                    log_success "iperf3 server detected on $peer:$IPERF_PORT"
                    test_peer "$peer" "client"
                else
                    log_warning "No iperf server on $peer:$IPERF_PORT"
                    log_info "Start server first: $0 -s"
                fi
                echo
            done
            
            log_info "Auto test completed for all peers"
            ;;
    esac
}

main "$@"
