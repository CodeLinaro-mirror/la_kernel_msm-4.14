#!/bin/bash

#===============================================================================
# NRC Power Save Monitor Script
#
# Monitors power save events from debugfs and displays with timestamps
# Accumulates history in memory to show more than debugfs buffer (16 events)
#===============================================================================

# Configuration
DEBUGFS_PATH="/sys/kernel/debug/nrc_core/ps_timing"
REFRESH_INTERVAL=2  # seconds
HISTORY_SIZE=50     # Number of events to keep in display
MAX_ACCUMULATED_EVENTS=1000  # Maximum events to keep in memory

# History storage file (temporary)
HISTORY_FILE="/tmp/nrc_ps_history_$$.txt"

# Colors
COLOR_RESET="\033[0m"
COLOR_HEADER="\033[1;36m"  # Cyan bold
COLOR_SLEEP="\033[1;33m"   # Yellow bold
COLOR_WAKE="\033[1;32m"    # Green bold
COLOR_TIMESTAMP="\033[0;90m"  # Gray
COLOR_ERROR="\033[1;31m"   # Red bold
COLOR_INFO="\033[0;36m"    # Cyan

# PS Reason names (from nrc-ps-common.h)
declare -A PS_REASON_NAMES
PS_REASON_NAMES[0]="MAC_PS_ENABLED"
PS_REASON_NAMES[1]="MAC_PS_DISABLED"
PS_REASON_NAMES[2]="MAC_IDLE_ENTER"
PS_REASON_NAMES[3]="MAC_IDLE_EXIT"
PS_REASON_NAMES[4]="TARGET_FW_READY"
PS_REASON_NAMES[5]="TARGET_FAILED_ENTER_PS"
PS_REASON_NAMES[10]="DRV_TX_WAKEUP"
PS_REASON_NAMES[11]="DRV_RX_WAKEUP"
PS_REASON_NAMES[12]="DRV_BSS_CONFIG"
PS_REASON_NAMES[13]="DRV_STA_ADD"
PS_REASON_NAMES[14]="DRV_STA_REMOVE"
PS_REASON_NAMES[15]="DRV_SCAN_START"
PS_REASON_NAMES[16]="DRV_SCAN_ABORT"
PS_REASON_NAMES[17]="DRV_ROC_START"
PS_REASON_NAMES[18]="DRV_ROC_CANCEL"
PS_REASON_NAMES[19]="DRV_APF_CONFIG"
PS_REASON_NAMES[20]="DRV_POST_INIT"
PS_REASON_NAMES[21]="DRV_DYNAMIC_PS"
PS_REASON_NAMES[30]="HAL_CALLBACK"
PS_REASON_NAMES[31]="HAL_PS_DYNAMIC"
PS_REASON_NAMES[32]="HAL_TX_TIMEOUT"
PS_REASON_NAMES[33]="HAL_TX_WAKEUP"
PS_REASON_NAMES[34]="HAL_SHUTDOWN"
PS_REASON_NAMES[40]="USER_NETLINK_CMD"
PS_REASON_NAMES[41]="USER_DEBUG_WAKE"
PS_REASON_NAMES[42]="USER_DEBUG_SLEEP"
PS_REASON_NAMES[50]="SYS_SUSPEND"

# Function to get reason name
get_reason_name() {
    local reason=$1
    echo "${PS_REASON_NAMES[$reason]:-UNKNOWN($reason)}"
}

# Function to format timestamp for display
format_timestamp() {
    local ts=$1
    local sec=${ts%.*}
    local nsec=${ts#*.}
    
    # Extract subsecond part (milliseconds)
    local ms=""
    if [ "$sec" != "$ts" ]; then
        # Convert nanoseconds to milliseconds (first 3 digits after decimal)
        ms=$(echo "$nsec" | cut -c1-3)
        [ -z "$ms" ] && ms="000"
    fi

    # Convert to human-readable format with milliseconds
    local datetime=$(date -d "@$sec" "+%Y-%m-%d %H:%M:%S" 2>/dev/null || echo "$ts")
    
    # Return datetime with milliseconds if available
    if [ -n "$ms" ]; then
        echo "${datetime}.${ms}"
    else
        echo "${datetime}"
    fi
}

# Function to calculate time difference in milliseconds
calc_time_diff() {
    local ts1=$1  # earlier
    local ts2=$2  # later

    local diff=$(echo "$ts2 - $ts1" | bc -l 2>/dev/null || echo "0")
    
    # Convert to integer milliseconds (no sub-millisecond precision)
    local diff_ms=$(echo "$diff * 1000 / 1" | bc 2>/dev/null || echo "0")
    
    # Format as seconds.milliseconds
    local seconds=$((diff_ms / 1000))
    local ms=$((diff_ms % 1000))
    
    printf "%d.%03d" "$seconds" "$ms"
}

# Check if debugfs file exists
check_debugfs() {
    if [ ! -f "$DEBUGFS_PATH" ]; then
        return 1
    fi
    return 0
}

# Wait for module to load
wait_for_module() {
    local max_wait=60  # Maximum 60 seconds
    local waited=0
    
    echo -e "${COLOR_HEADER}Waiting for nrc_core module to load...${COLOR_RESET}"
    
    while [ $waited -lt $max_wait ]; do
        if check_debugfs; then
            echo -e "${COLOR_HEADER}Module detected! Starting monitoring...${COLOR_RESET}"
            sleep 1
            return 0
        fi
        
        # Show progress
        printf "\rWaiting... %ds (Ctrl+C to cancel)" $waited
        sleep 1
        waited=$((waited + 1))
    done
    
    echo ""
    echo -e "${COLOR_ERROR}Timeout: Module not loaded after ${max_wait}s${COLOR_RESET}"
    echo "Please ensure:"
    echo "  1. nrc_core.ko module is loaded (check with: lsmod | grep nrc_core)"
    echo "  2. CONFIG_DEBUG_FS is enabled in kernel"
    echo "  3. debugfs is mounted at /sys/kernel/debug"
    return 1
}

# Display header
display_header() {
    local total_events=$1
    local accumulated_events=$2
    
    clear
    echo -e "${COLOR_HEADER}╔═════════════════════════════════════════════════════════════════════════════════════════════════════════╗${COLOR_RESET}"
    echo -e "${COLOR_HEADER}║                       NRC Power Save Monitor - Real-time Event Log                                     ║${COLOR_RESET}"
    echo -e "${COLOR_HEADER}╠═════════════════════════════════════════════════════════════════════════════════════════════════════════╣${COLOR_RESET}"
    echo -e "${COLOR_HEADER}║ Refresh: ${REFRESH_INTERVAL}s | Path: ${DEBUGFS_PATH}                                   ║${COLOR_RESET}"
    echo -e "${COLOR_HEADER}║ Debugfs Events: ${total_events} | Accumulated: ${accumulated_events} | Displaying: Last ${HISTORY_SIZE}                                  ║${COLOR_RESET}"
    echo -e "${COLOR_HEADER}╚═════════════════════════════════════════════════════════════════════════════════════════════════════════╝${COLOR_RESET}"
    echo ""
    printf "%-16s  %-8s  %-12s  %-20s  %-28s  %s\n" \
        "TIMESTAMP" "TYPE" "STATE" "MODE" "REASON" "TIMEOUT/DURATION"
    echo "────────────────  ────────  ────────────  ────────────────────  ────────────────────────────  ──────────────────"
}

# Main monitoring loop
monitor_ps() {
    local prev_sleep_ts=""
    local prev_wake_ts=""
    local first_display=true
    local seen_file="/tmp/nrc_ps_seen_$$.txt"
    
    # Initialize history file
    > "$HISTORY_FILE"
    > "$seen_file"

    while true; do
        # Check if module is still loaded
        if ! check_debugfs; then
            if [ "$first_display" = false ]; then
                # Module was loaded but now unloaded
                echo -e "\n${COLOR_ERROR}Warning: Module unloaded or debugfs unavailable${COLOR_RESET}"
                echo -e "${COLOR_HEADER}Waiting for module to reload...${COLOR_RESET}"
            fi
            sleep $REFRESH_INTERVAL
            continue
        fi

        # Read debugfs file
        local content=$(cat "$DEBUGFS_PATH" 2>/dev/null)
        if [ -z "$content" ]; then
            sleep $REFRESH_INTERVAL
            continue
        fi

        # Parse header to get total event count from debugfs
        local current_count=$(echo "$content" | grep "total events:" | sed 's/.*total events: \([0-9]*\).*/\1/')

        # Process and accumulate new events
        local new_events=0
        local temp_new="/tmp/nrc_ps_new_$$.txt"
        > "$temp_new"
        
        echo "$content" | grep -v "^#" | while IFS='|' read ts type state mode reason timeout; do
            [ -z "$ts" ] && continue
            
            # Check if we've already seen this timestamp
            if ! grep -q "^${ts}$" "$seen_file" 2>/dev/null; then
                # New event - add to history file
                echo "$ts|$type|$state|$mode|$reason|$timeout" >> "$HISTORY_FILE"
                echo "$ts" >> "$seen_file"
                echo "1" >> "$temp_new"
            fi
        done
        
        # Count new events
        new_events=$(wc -l < "$temp_new" 2>/dev/null || echo 0)
        rm -f "$temp_new"

        # Limit history file size
        local line_count=$(wc -l < "$HISTORY_FILE" 2>/dev/null || echo 0)
        if [ "$line_count" -gt "$MAX_ACCUMULATED_EVENTS" ]; then
            # Keep only the most recent events
            tail -n "$MAX_ACCUMULATED_EVENTS" "$HISTORY_FILE" > "${HISTORY_FILE}.tmp"
            mv "${HISTORY_FILE}.tmp" "$HISTORY_FILE"
            
            # Also trim seen file to match
            tail -n "$MAX_ACCUMULATED_EVENTS" "$seen_file" > "${seen_file}.tmp"
            mv "${seen_file}.tmp" "$seen_file"
        fi

        # Count accumulated events
        local accumulated_count=$(wc -l < "$HISTORY_FILE" 2>/dev/null || echo 0)

        # Redisplay if new events or first time
        if [ "$new_events" -gt 0 ] || [ "$first_display" = true ]; then
            display_header "$current_count" "$accumulated_count"
            first_display=false

            # Display last N events from accumulated history
            tail -n "$HISTORY_SIZE" "$HISTORY_FILE" | while IFS='|' read ts type state mode reason timeout; do
                [ -z "$ts" ] && continue

                # Format timestamp with milliseconds
                local formatted_ts=$(format_timestamp "$ts")
                local time_part=$(echo "$formatted_ts" | awk '{print $2}')

                # Get reason name
                local reason_name=$(get_reason_name "$reason")

                # Color and format based on type
                if [ "$type" = "S" ]; then
                    # Sleep event - calculate awake duration if we have previous wake
                    local duration_info=""
                    # Handle special timeout values
                    local timeout_display="$timeout ms"
                    if [ "$timeout" = "18446744073709551615" ] || [ "$timeout" = "0" ]; then
                        timeout_display="infinite"
                    fi
                    
                    if [ -n "$prev_wake_ts" ]; then
                        local duration=$(calc_time_diff "$prev_wake_ts" "$ts")
                        # Parse seconds and milliseconds
                        local sec_part="${duration%.*}"
                        local ms_part="${duration#*.}"
                        
                        # Format as "Xs YYYms"
                        if [ "$sec_part" -gt 0 ]; then
                            duration_info="${sec_part}s ${ms_part}ms (awake)"
                        else
                            duration_info="${ms_part}ms (awake)"
                        fi
                        timeout_display="${timeout_display}, ${duration_info}"
                    fi
                    
                    printf "${COLOR_SLEEP}%-16s  %-8s  %-12s  %-20s  %-28s  %s${COLOR_RESET}\n" \
                        "$time_part" "SLEEP" "$state" "$mode" "$reason_name" "$timeout_display"
                    prev_sleep_ts="$ts"
                    prev_wake_ts=""
                elif [ "$type" = "W" ]; then
                    # Wake event - calculate sleep duration if we have previous sleep
                    local duration_info=""
                    if [ -n "$prev_sleep_ts" ]; then
                        local duration=$(calc_time_diff "$prev_sleep_ts" "$ts")
                        # Parse seconds and milliseconds
                        local sec_part="${duration%.*}"
                        local ms_part="${duration#*.}"
                        
                        # Format as "Xs YYYms"
                        if [ "$sec_part" -gt 0 ]; then
                            duration_info="${sec_part}s ${ms_part}ms"
                        else
                            duration_info="${ms_part}ms"
                        fi
                        duration_info="(slept ${duration_info})"
                    fi

                    printf "${COLOR_WAKE}%-16s  %-8s  %-12s  %-20s  %-28s  %s${COLOR_RESET}\n" \
                        "$time_part" "WAKE" "$state" "$mode" "$reason_name" "$duration_info"
                    prev_wake_ts="$ts"
                    prev_sleep_ts=""
                fi
            done

            echo ""
            echo -e "${COLOR_TIMESTAMP}Last updated: $(date '+%Y-%m-%d %H:%M:%S')${COLOR_RESET}"
            echo -e "${COLOR_INFO}Total accumulated events: ${accumulated_count} (max: ${MAX_ACCUMULATED_EVENTS})${COLOR_RESET}"
        fi

        sleep $REFRESH_INTERVAL
    done
    
    # Cleanup seen file
    rm -f "$seen_file"
}

# Cleanup on exit
cleanup() {
    echo -e "\n${COLOR_HEADER}Monitoring stopped.${COLOR_RESET}"
    
    # Remove temporary history file
    if [ -f "$HISTORY_FILE" ]; then
        rm -f "$HISTORY_FILE"
        echo -e "${COLOR_INFO}History file cleaned up.${COLOR_RESET}"
    fi
    
    exit 0
}

trap cleanup SIGINT SIGTERM

# Main
main() {
    echo -e "${COLOR_HEADER}═══════════════════════════════════════════════════════════${COLOR_RESET}"
    echo -e "${COLOR_HEADER}       NRC Power Save Monitor - Starting...${COLOR_RESET}"
    echo -e "${COLOR_HEADER}═══════════════════════════════════════════════════════════${COLOR_RESET}"
    echo ""

    # Check if already loaded
    if check_debugfs; then
        echo -e "${COLOR_HEADER}✓ nrc_core module already loaded${COLOR_RESET}"
        sleep 1
    else
        # Wait for module to load
        if ! wait_for_module; then
            exit 1
        fi
    fi

    monitor_ps
}

main
