# Firmware Lifecycle and Boot Sequences

This document describes the firmware lifecycle management, boot sequences, and state transitions in the NRC driver architecture.

## Table of Contents

- [Overview](#overview)
- [EIRQ Status Register](#eirq-status-register)
- [Firmware States](#firmware-states)
- [Boot Sequences](#boot-sequences)
  - [Initial Boot](#initial-boot)
  - [Module Reload](#module-reload)
  - [Power Save Wake](#power-save-wake)
  - [WDT Recovery](#wdt-recovery)
- [State Transition Diagram](#state-transition-diagram)
- [Firmware Download Protocol](#firmware-download-protocol)
- [Implementation Details](#implementation-details)

---

## Overview

The NRC driver manages firmware lifecycle through hardware status monitoring (`EIRQ_STATUS`) and software state tracking (`NRC_FW_STATE`). The firmware can be in different operational states depending on boot progress, power management, and error recovery scenarios.

**Key Principle**:
- Hardware state (`EIRQ_STATUS`) is the source of truth
- Software state (`NRC_FW_STATE`) mirrors hardware state
- FW ready verification happens **after** WIM_CMD_START, not before

---

## EIRQ Status Register

The SPI status register `EIRQ_STATUS` indicates device state bits:

| Value | Name | Meaning |
|-------|------|---------|
| `0x00` | `EIRQ_STATUS_DEVICE_ROM` | Device in ROM/bootloader mode |
| `0x01` | `EIRQ_STATUS_TXQUE_EIRQ` | TX queue interrupt pending |
| `0x02` | `EIRQ_STATUS_RXQUE_EIRQ` | RX queue interrupt pending |
| `0x04` | `EIRQ_STATUS_DEVICE_READY` | Device ready bit (not used for FW state) |
| `0x08` | `EIRQ_STATUS_DEVICE_SLEEP` | Device sleep bit (not used for FW state) |

**Important**: The driver does **NOT** rely on `EIRQ_STATUS` for firmware state detection. The actual firmware state is determined by:

1. **`sw_id` register** - Primary method for FW running detection:
   - `sw_id = 0x01020716` (SW_MAGIC_FOR_BOOT): Bootloader mode, FW not loaded
   - `sw_id = (chip_id << 16) | version`: FW loaded and running (e.g., `0x73940001`)

2. **`msg[3]` TARGET_NOTI** - For PS wake/sleep events and WDT recovery

**Why EIRQ_STATUS alone is insufficient**:
- `EIRQ_STATUS = 0x00` can mean: initial power-on, FW download in progress, WDT reset, or FW just downloaded
- `EIRQ_STATUS_DEVICE_READY` and `EIRQ_STATUS_DEVICE_SLEEP` bits exist in hardware but are only used for debug logging and `spi_poll_status()` internal branching

### Target Notifications

Firmware sends notifications via `msg[3]` register:

| Value | Name | Trigger | Action Required |
|-------|------|---------|----------------|
| `0x11` | `TARGET_NOTI_PS_READY` | Power save ready | PS entry confirmed |
| `0x7D` | `TARGET_NOTI_WDT_EXPIRED` | Watchdog timeout | Wait for recovery |
| `0x9D` | `TARGET_NOTI_FW_READY_FROM_WDT` | FW recovered from WDT | Resume operations |
| `0xAB` | `TARGET_NOTI_W_DISABLE_ASSERTED` | W_DISABLE signal asserted | Handle connection loss |
| `0xBE` | `TARGET_NOTI_BEACON_UPDATED` | Beacon updated | Backend layer handles |
| `0xDC` | `TARGET_NOTI_REQUEST_FW_DOWNLOAD` | FW needs download (WDT/wake) | Download FW |
| `0xEA` | `TARGET_NOTI_TWT_SERVICE` | TWT service notification | Resume TWT |
| `0xEB` | `TARGET_NOTI_TWT_QUIET` | TWT quiet notification | Enter TWT quiet |
| `0xEC` | `TARGET_NOTI_FW_READY_FROM_PS` | FW woke from power save | Resume operations |
| `0xED` | `TARGET_NOTI_FAILED_TO_ENTER_PS` | Failed to enter power save | Retry or abort PS |
| `0xEF` | `TARGET_NOTI_FW_ENTER_TO_PS` | Firmware entering power save | Backend layer handles |

---

## Firmware States

The driver uses **software state tracking** (`hdev->fw.state`) for firmware lifecycle management:

```c
#define NRC_FW_NONE     (0)    /* No firmware loaded */
#define NRC_FW_LOADING  (1)    /* FW download in progress */
#define NRC_FW_ACTIVE   (2)    /* Firmware running */
#define NRC_FW_FAILED   (-1)   /* Firmware load failed */
```

**Note**: `hdev->fw.started` (atomic flag) tracks whether WIM_CMD_START has completed successfully. This is separate from the FW state to handle sleep/wake scenarios properly.

**Legacy `enum NRC_FW_STATE`**: Exists in code for EIRQ_STATUS mapping but is **not used** for actual FW state decisions. The driver relies on `sw_id` register and TARGET_NOTI instead.

### State Descriptions

#### `NRC_FW_NONE` (Value: 0)
- **Meaning**: Initial state, no firmware loaded
- **Verification**: `sw_id = 0x01020716` (SW_MAGIC_FOR_BOOT) confirms bootloader mode
- **Actions**:
  - Download firmware binary to device RAM
  - Initialize slot/credit structures
  - Start RX/IRQ threads

#### `NRC_FW_LOADING` (Value: 1)
- **Meaning**: FW download in progress
- **Set by**: `fw_download()` when `auto_verify=true`
- **Actions**:
  - Continue FW fragment transfer
  - Wait for download completion and `sw_id` verification

#### `NRC_FW_ACTIVE` (Value: 2)
- **Meaning**: Firmware downloaded and verified (`sw_id` check passed)
- **Set by**: `fw_download()` after `fw_wait_ready()` succeeds
- **Verification**: `sw_id = (chip_id << 16) | sw_version` (e.g., `0x73940001`)
- **Critical Path**:
  - **WIM TX**: Only WIM_CMD_START allowed (if not already started)
  - **WIM RX**: All WIM responses allowed
- **Actions**:
  - Check `hdev->fw.started` flag
  - If started==0: Send WIM_CMD_START, set started=1
  - If started==1: Skip WIM_CMD_START (already initialized)

#### `NRC_FW_FAILED` (Value: -1)
- **Meaning**: Firmware load failed
- **Set by**: `fw_download()` when `fw_wait_ready()` times out
- **Actions**: Error recovery required

---

## Boot Sequences

### Initial Boot

**Scenario**: First driver load after power-on

**Initial Conditions**:
```
EIRQ_STATUS = 0x00 (DEVICE_ROM)
sw_id       = 0x01020716 (SW_MAGIC_FOR_BOOT)
fw.state    = NRC_FW_NONE
```

**Sequence**:

```
1. Module Init
   ├─ insmod nrc_spi.ko
   ├─ insmod nrc_core.ko
   └─ insmod nrc_wlan.ko

2. Frontend Init (nrc_wlan)
   ├─ nrc_hal_init_with_network_device()
   ├─ nrc_wlan_sync_params() → share params with HAL
   └─ nrc_nw_start()

3. FW State Sync (nrc_nw_start)
   ├─ Check hdev->fw.loaded (module reload case)
   │  └─ If true → skip FW download
   ├─ Read HW state: nrc_hif_ops_fw_is_boot()
   │  └─ Check if target is in bootloader mode
   └─ Decision: bootloader mode → need FW download

4. Board Data (optional)
   └─ nrc_check_bd() → load board file

5. FW Download & Auto-Boot
   ├─ nrc_fw_load()
   │  └─ fw_download_to_ram()      [internal]
   │     ├─ Set state: NRC_FW_NONE → NRC_FW_LOADING
   │     ├─ fw_download() → binary transfer to device RAM
   │     │  └─ FW receives binary and auto-boots immediately
   │     │
   │     ├─ fw_wait_ready() → verify FW auto-boot completed
   │     │  ├─ Retry up to 300 times with 10ms interval (max 3 sec)
   │     │  ├─ nrc_hif_ops_fw_is_loaded() → read sys.sw_id register
   │     │  │  ├─ Initial: 0x01020716 (SW_MAGIC_FOR_BOOT - bootloader)
   │     │  │  └─ After boot: 0x73940001 (chip_id << 16 | version)
   │     │  ├─ Check: (sys.sw_id & 0xFFFF) == NRC_SW_ID ✓
   │     │  ├─ Check: (sys.sw_id >> 16) == chip_id ✓
   │     │  └─ Check: sys.status & 0x1 ✓
   │     │
   │     ├─ Set state: NRC_FW_LOADING → NRC_FW_ACTIVE
   │     └─ [INFO] "FW download and verification completed successfully"
   │
   └─ FW auto-booted, sw_id verified, state = NRC_FW_ACTIVE

6. HAL Start
   ├─ nrc_hal_start()
   │  ├─ Start RX thread
   │  ├─ Enable IRQ
   │  └─ [INFO] "HAL start: RX thread and IRQ"
   └─ **CRITICAL**: WIM communication path enabled!

7. FW Start (send WIM_CMD_START, set started flag)
   ├─ nrc_fw_start()
   │  ├─ **Check started flag**
   │  │  ├─ if (NRC_FW_IS_STARTED(hdev)) → Skip WIM_CMD_START
   │  │  │  └─ Already initialized (e.g., after sleep/wake)
   │  │  └─ else → First time initialization
   │  │
   │  ├─ **WIM TX Path Check** (wim.c:230)
   │  │  ├─ if (fw.state == NRC_FW_LOADING) → Block all WIM commands
   │  │  └─ if (fw.state == NRC_FW_ACTIVE && cmd == WIM_CMD_START) → **ALLOW** ✓
   │  │
   │  ├─ Send WIM_CMD_START (driver info, parameters)
   │  │  └─ nrc_wim_request(WIM_CMD_START, timeout=70sec)
   │  │
   │  ├─ **WIM RX Path Check** (nrc-hal-callback.c:856)
   │  │  ├─ if (fw.state == NRC_FW_LOADING) → Drop WIM packet
   │  │  └─ if (fw.state == NRC_FW_ACTIVE) → **Process** ✓
   │  │
   │  ├─ **Set started flag** ✓
   │  │  └─ atomic_set(&hdev->fw.started, 1)  [nrc-fw.c internal only]
   │  │
   │  ├─ Receive WIM_CMD_START response
   │  │  └─ fw_on_ready(skb_resp)
   │  │     ├─ Parse FW version, capabilities
   │  │     ├─ Parse TX/RX head sizes
   │  │     └─ Parse MAC addresses
   │  │
   │  └─ **State remains: NRC_FW_ACTIVE** (don't change)
   │     └─ FW running = NRC_FW_ACTIVE + started==true
   │
   └─ [INFO] "FW started successfully (state=ACTIVE, started=YES)"

8. Complete
   └─ Driver operational, fw.state = NRC_FW_ACTIVE, started = YES
```

**Timeline**:
```
  Host                           Firmware
  ────                           ────────
  FW download ───────────────→   Receive binary
                                 Auto-boot immediately
                                 Complete initialization
                                 Set sw_id = 0x73940001
                                 Set status = 0x1 (ready)

  fw_wait_ready() ─────────────→
  (polling via fw_is_loaded)
                 ←──────────────  sw_id = 0x73940001 ✓

  State: NRC_FW_NONE → NRC_FW_LOADING → NRC_FW_ACTIVE

  HAL start (RX/IRQ)             (FW ready, waiting for CMD_START)

  WIM_CMD_START ─────────────→   Process CMD_START
  Set started=true ✓
                                 Send capabilities
                 ←──────────────  WIM response (ready info)

  fw_on_ready()
  Parse FW info

  State: NRC_FW_ACTIVE (keep), started=YES
  FW running = NRC_FW_ACTIVE + started
```

---

### Module Reload

**Scenario**: `rmmod nrc_wlan.ko; insmod nrc_wlan.ko` (SPI/Core remain loaded)

**Initial Conditions**:
```
sw_id       = 0x73940001 (valid FW)
fw.loaded   = true
fw.state    = NRC_FW_ACTIVE
```

**Sequence**:

```
1. Frontend Init
   └─ nrc_nw_start()

2. Check fw.loaded flag
   ├─ hdev->fw.loaded == true
   └─ [INFO] "Firmware already loaded, skipping firmware download"

3. Skip FW Download
   └─ goto skip_fw_download

4. HAL Start (reuse existing FW)
   └─ nrc_hal_start()

5. FW Start
   └─ nrc_fw_start() → send WIM_CMD_START

6. Complete
   └─ Driver operational, reused existing FW
```

**Key Point**: No FW download needed, reuse running firmware via `fw.loaded` flag.

---

### Power Save Wake

**Scenario**: Deep sleep → beacon wake or host wake

#### Case 1: XIP Mode (fw_name = NULL)

**Initial Conditions**:
```
msg[3]      = 0xDC (REQUEST_FW_DOWNLOAD)
fw.state    = NRC_FW_ACTIVE
ps.state    = SLEEP
```

**Sequence**:
```
1. IRQ 0xDC Received
   ├─ msg[3] = 0xDC (REQUEST_FW_DOWNLOAD)
   └─ Trigger: NRC_BACKEND_EVT_TARGET_NOTI_REQUEST_FW_DOWNLOAD

2. HAL Handler (nrc_hal_handle_request_fw_download)
   ├─ Check fw_name = NULL → XIP mode
   ├─ PS state: SLEEP → WAKING
   └─ Skip FW download (FW in flash)

3. Wait for FW Ready
   └─ IRQ 0xEC (FW_READY_FROM_PS)
      ├─ Call: nrc_hif_reset_slot_credit()
      └─ PS state: WAKING → AWAKE
```

#### Case 2: RAM Mode (fw_name != NULL)

**Initial Conditions**:
```
msg[3]      = 0xDC (REQUEST_FW_DOWNLOAD)
fw.state    = NRC_FW_ACTIVE
ps.state    = SLEEP
```

**Sequence**:
```
1. IRQ 0xDC Received
   └─ Same as above

2. HAL Handler
   ├─ Check fw_name != NULL → RAM mode
   ├─ PS state: SLEEP → WAKING
   ├─ FW download required (RAM lost in sleep)
   └─ Call: nrc_fw_reload()
      ├─ Set sw state: NRC_FW_LOADING
      ├─ Download FW binary (RAM only)
      └─ Set sw state: NRC_FW_ACTIVE

3. No WIM_CMD_START in 0xDC handler
   └─ Why? FW already started, just reloaded

4. Wait for FW Ready (passive)
   └─ IRQ 0xEC (FW_READY_FROM_PS)
      └─ PS state: WAKING → AWAKE
```

**Key Difference**:
- XIP: FW in flash, no download
- RAM: FW lost, must re-download

---

### WDT Recovery

**Scenario**: Watchdog timeout → FW crash → auto-recovery

**Timeline**:

```
1. WDT Timeout
   ├─ IRQ 0x7D (WDT_EXPIRED)
   │  └─ Log: "WDT EXPIRED"
   └─ FW reboots automatically

2. FW Requests Download
   ├─ IRQ 0xDC (REQUEST_FW_DOWNLOAD)
   │  └─ msg[3] = 0xDC
   └─ Trigger: nrc_hal_handle_request_fw_download()

3. HAL Download Handler
   ├─ Set sw state: NRC_FW_LOADING
   ├─ nrc_fw_reload()
   │  └─ Download FW binary (RAM only)
   ├─ Set sw state: NRC_FW_ACTIVE
   └─ nrc_ps_handle_fw_ready()

4. FW Recovery Complete
   └─ IRQ 0x9D (FW_READY_FROM_WDT)
      ├─ Cleanup WIM responses
      └─ Notify frontends
```

**Important**: WDT recovery does NOT send WIM_CMD_START because FW auto-restarts.

---

## State Transition Diagram

```
                    ┌──────────────────────┐
                    │   Module Load/Init   │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  Check sw_id reg     │
                    │  (FW running check)  │
                    └──────────┬───────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
        ┌───────▼───────┐     │      ┌───────▼────────┐
        │ sw_id=MAGIC   │     │      │ sw_id=valid    │
        │ (bootloader)  │     │      │ (FW running)   │
        └───────┬───────┘     │      └───────┬────────┘
                │             │              │
                │             │              │
        ┌───────▼────────┐    │      ┌───────▼────────┐
        │ SW: NRC_FW_NONE│    │      │ SW: NRC_FW_    │
        └───────┬────────┘    │      │     ACTIVE     │
                │             │      └───────┬────────┘
                │             │              │
    ┌───────────▼──────────┐  │      ┌───────▼───────────┐
    │  Need FW Download    │  │      │  Reuse Existing   │
    │  1. nrc_fw_load()    │  │      │  Skip Download    │
    │  2. fw_wait_ready()  │  │      │  Set started=1    │
    │  3. HAL start        │  │      │  goto skip_fw_    │
    │  4. WIM_CMD_START    │  │      │     download      │
    │  5. Set started=1    │  │      └─────────┬─────────┘
    └───────────┬──────────┘  │                │
                │             │                │
                ▼             │                │
        ┌──────────────┐      │      ┌─────────▼─────────┐
        │ SW: NONE →   │      │      │  SW: ACTIVE       │
        │  LOADING →   │      │      │  started=YES      │
        │  ACTIVE      │      │      └───────────────────┘
        │ started=YES  │      │
        └──────────────┘      │
                              │
                              │
                      ┌───────▼─────────────┐
                      │  Deep Sleep (PS)    │
                      │  SW: ACTIVE (keep)  │
                      └───────┬─────────────┘
                              │
                      ┌───────▼─────────────┐
                      │  IRQ 0xDC received  │
                      │  (wake request)     │
                      └───────┬─────────────┘
                              │
                ┌─────────────┼─────────────┐
                │                           │
        ┌───────▼────────┐          ┌───────▼────────┐
        │ fw_name=NULL   │          │ fw_name!=NULL  │
        │  (XIP mode)    │          │  (RAM mode)    │
        └───────┬────────┘          └───────┬────────┘
                │                           │
                │                           │
        ┌───────▼────────┐          ┌───────▼────────┐
        │ Skip Download  │          │ FW Download    │
        │ FW in flash    │          │ SW: LOADING →  │
        └───────┬────────┘          │     ACTIVE     │
                │                   └───────┬────────┘
                └─────────────┬─────────────┘
                              │
                      ┌───────▼─────────────┐
                      │  IRQ 0xEC received  │
                      │  (FW_READY_FROM_PS) │
                      └───────┬─────────────┘
                              │
                      ┌───────▼────────┐
                      │  SW: ACTIVE    │
                      │  PS: AWAKE     │
                      └────────────────┘
```

---

## Firmware Download Protocol

FW binary is transferred in chunks as WIM packets (`HIF_WIM_SUB_REQUEST`), with ACK verification after each chunk.

### Transfer Flow

```
Host                              FW (Target)
  │                                  │
  │── Chunk N (WIM_SUB_REQUEST) ────>│
  │                                  │ rxque_isr() triggered
  │                                  │ Process chunk (on_fota)
  │                                  │ Allocate new RX buffer
  │                                  │ put_msg(HIF_CSPI_RX, ++report)
  │<── rxq_status[1] updated ────────│
  │                                  │
  │ spi_wait_ack() exits             │
  │── Chunk N+1 ────────────────────>│
  │        ...                       │
  │── Chunk Last (eof=1) ───────────>│
  │                                  │ on_fota(): CRC verify, set ready
  │                                  │ FW stops buffer management
```

### Key Functions

**Host Side:**

| Function | Location | Role |
|----------|----------|------|
| `nrc_fw_send_frag()` | nrc-fw.c | Build WIM packet and send via SPI |
| `nrc_fw_check_next_frag()` | nrc-fw.c | Wait ACK, update next chunk info |
| `spi_wait_ack()` | nrc-spi-hif-ops.c | Poll `rxq_status[1]` for ACK |

**FW Side:**

| Function | Location | Role |
|----------|----------|------|
| `rxque_isr()` | hal_cspi.c | Handle Host→FW data, allocate new buffer |
| `put_msg(HIF_CSPI_RX, ...)` | hal_cspi.c | Update RX slot count (ACK to Host) |
| `on_fota()` | umac_wim_manager.c | Process FW chunk, write to flash |

### Register Mapping

FW and Host share status via SPI registers:

```
FW Side                              Host Side
─────────────────────────────────────────────────────────────
put_msg(HIF_CSPI_RX, value)    →    status.rxq_status[1]
  RegHIF_DEVICE_MESSAGE0[31:16]      (RX buffer slot count)

put_msg(HIF_CSPI_TX, value)    →    status.msg[0][15:0]
  RegHIF_DEVICE_MESSAGE0[15:0]       (TX slot head)
```

### ACK Mechanism

**Host polling (`spi_wait_ack`):**
```c
do {
    c_spi_read_regs(spi, C_SPI_EIRQ_MODE, &status, sizeof(status));
} while ((status.rxq_status[1] & RXQ_SLOT_COUNT) < 1);
```

**FW response (`rxque_isr`):**
```c
// After processing received packet, allocate new RX buffer
packet = system_memory_pool_alloc(1);
if (packet) {
    util_sysbuf_queue_push(&priv->hwque[HIF_CSPI_RX], packet);
    drv_cspi_set(HIF_CSPI_RX, address, N_RX_CHUNK, true, false);
    put_msg(HIF_CSPI_RX, ++priv->report[HIF_CSPI_RX]);  // ← ACK!
}
```

- `rxq_status[1]`: FW's available RX buffer slot count (general purpose, not FW-download specific)
- `RXQ_SLOT_COUNT`: `0x7F` (lower 7-bit mask)
- **Exit condition**: FW increments slot count to >= 1
- **Caution**: No timeout → infinite loop if FW doesn't respond

### Known Issue: Last Chunks Hang

**Symptom:**
```
[  155.523722] nrc-hal core: Info 502/505 chunks sent
Network error: Software caused connection abort
```

**Root Cause:**
1. FW receives last chunks and writes to flash
2. `on_fota()` verifies CRC on `frag->eof == 1`
3. On success: `util_fota_set_ready(true)` - FW prepares for reboot
4. FW stops `rxque_isr()` buffer management
5. `put_msg(HIF_CSPI_RX, ...)` no longer called
6. Host's `spi_wait_ack()` hangs indefinitely (no timeout)

---

## Implementation Details

### FW Ready Verification

**Location**: `backend/nrc_spi/nrc-spi-hif-ops.c:spi_check_ready()`

**Checks**:
1. `sys.status & 0x1` - Boot status bit
2. `sys.sw_id & 0xFFFF == NRC_SW_ID` - SW version match
3. `(sys.sw_id >> 16) == sys.chip_id` - Chip ID consistency

**Example**:
```c
sys.chip_id = 0x7394
sys.sw_id   = 0x73940001
            = (0x7394 << 16) | 0x0001
            = chip_id in upper 16 bits, version in lower 16 bits

Check 1: status = 0x1 ✓
Check 2: 0x0001 == NRC_SW_ID (1) ✓
Check 3: 0x7394 == 0x7394 ✓
→ FW READY!
```

### Critical Timing

**FW Auto-Boot and sw_id Update**:

1. **FW Boot Timeline**:
   ```
   FW binary download → Auto-boot immediately → Complete initialization →
   Update sw_id register (0x01020716 → 0x73940001) →
   Wait for WIM_CMD_START
   ```

2. **Host Actions**:
   ```
   Download FW → Poll sw_id (until 0x73940001) →
   Start RX/IRQ → Send WIM_CMD_START → Parse response → ACTIVE
   ```

3. **Key Points**:
   - FW auto-boots **immediately** after download
   - `sw_id` verification happens **before** HAL start via `fw_wait_ready()`
   - WIM_CMD_START is for **exchanging capabilities**, not triggering boot
   - SW state transition NRC_FW_LOADING → NRC_FW_ACTIVE happens **after** `fw_wait_ready()` succeeds

4. **NRC_FW_LOADING State Critical Path**:
   ```
   SW State = NRC_FW_LOADING means:
   - FW download in progress OR waiting for sw_id verification
   - After verification: state changes to NRC_FW_ACTIVE
   - HAL started (RX/IRQ enabled)
   - Waiting for WIM_CMD_START response

   During NRC_FW_LOADING:
   - WIM TX: Blocked (FW not verified yet)
   - WIM RX: Blocked
   ```

### Key Functions

| Function | Purpose | Use Case |
|----------|---------|----------|
| `nrc_fw_load()` | Initial FW load (RAM + optional XIP fusing) | Driver initialization |
| `nrc_fw_reload()` | Lightweight FW reload (RAM only) | PS wake |
| `nrc_fw_fusing()` | Flash programming (DL + FW + BL) | Manufacturing/recovery |
| `nrc_fw_start()` | Send WIM_CMD_START, set started flag | After FW loaded |
| `nrc_hal_start()` | Start RX/IRQ threads | Infrastructure |
| `nrc_hif_ops_fw_is_boot()` | Check if target in bootloader mode (sw_id) | Check before download |
| `nrc_hif_ops_fw_is_loaded()` | Verify sw_id register (FW loaded) | Verification after download |

### FW Loading Functions Detail

```
nrc_fw_load()           - Initial boot
├─ fw_download_to_ram() - Download FW binary to RAM
│   └─ fw_download(to_xip=false, auto_verify=true)
└─ fw_xip_fusing()      - Optional XIP flash update
    └─ fw_download(to_xip=true, auto_verify=false)

nrc_fw_reload()         - PS wake (lightweight)
└─ fw_download_to_ram() - Download FW binary to RAM only

nrc_fw_fusing()         - Flash programming mode
├─ fw_download(DL)      - Download loader
├─ fw_download(FW)      - Firmware to flash
└─ fw_download(BL)      - Bootloader to flash
```

### FW Started Flag Macros

The `hdev->fw.started` atomic flag tracks whether WIM_CMD_START has completed successfully.
This is separate from the FW state machine to handle sleep/wake scenarios properly.

| Macro | Purpose | Location |
|-------|---------|----------|
| `NRC_FW_IS_STARTED(hdev)` | Check if WIM_CMD_START completed | `nrc-hif.h` |
| `NRC_FW_SET_STARTED(hdev)` | Mark WIM_CMD_START as completed | `nrc-hif.h` |
| `NRC_FW_CLEAR_STARTED(hdev)` | Clear on WIM_CMD_STOP | `nrc-hif.h` |

**Usage**:
```c
/* Set when WIM_CMD_START succeeds (nrc-fw.c) */
fw_on_ready(skb_resp);
NRC_FW_SET_STARTED(hdev);

/* Clear when WIM_CMD_STOP is sent (nrc-mac80211.c) */
nrc_hal_ops_wim_request(NULL, WIM_CMD_STOP, 0, false, NULL);
NRC_FW_CLEAR_STARTED(hdev);

/* Check before using FW features */
if (NRC_FW_IS_STARTED(hdev)) {
    /* FW is ready for WLAN operations */
}
```

### State Management Rules

1. **Primary State**: SW state (`hdev->fw.state`) tracks FW loading progress
2. **FW Detection**: Use `sw_id` register, NOT `EIRQ_STATUS`, to check if FW is running
3. **SW State Values**: `NRC_FW_NONE` → `NRC_FW_LOADING` → `NRC_FW_ACTIVE` (or `NRC_FW_FAILED`)
4. **Atomic Transitions**: Use `atomic_set(&hdev->fw.state, ...)` for state changes
5. **Error Recovery**: Set SW state to `NRC_FW_FAILED` on failure
6. **Module Reload**: Check `hdev->fw.loaded` flag first, reuse FW if true
7. **PS Wake**: TARGET_NOTI (0xDC, 0xEC) via `msg[3]` register

---

## Common Issues

### Issue 1: Hang after FW download

**Symptom**:
```
FW download completed successfully
(hang - no further progress)
```

**Cause**: Checking FW ready before starting RX/IRQ threads

**Solution**:
- Start RX/IRQ first (`nrc_hal_start()`)
- Send WIM_CMD_START (`nrc_fw_start()`)
- FW verification via `nrc_hif_ops_fw_is_loaded()`

### Issue 2: Wrong sw_id value

**Symptom**:
```
sw_id = 0x01020716 (SW_MAGIC_FOR_BOOT)
Expected: 0x73940001
```

**Cause**: FW not fully initialized (download in progress or failed)

**Solution**: 
- Wait for `fw_wait_ready()` to complete
- Check if `fw.state == NRC_FW_ACTIVE` before sending WIM commands

### Issue 3: Module reload re-downloads FW

**Symptom**: Unnecessary FW download on `insmod nrc_wlan.ko`

**Cause**: Not checking `hdev->fw.loaded` flag before download

**Solution**: Check `hdev->fw.loaded` first, skip download if true

---

