# segs_fix.py - TM1640 decode from Logic2 digital.csv and print lit GRID/SEG pins
import csv

FN = "data/digital.csv"

# Flip this if polarity is inverted on your hardware capture:
# - ACTIVE_LOW=False => segment ON when bit=1
# - ACTIVE_LOW=True  => segment ON when bit=0
ACTIVE_LOW = False

# For 3x4 digits, usually GRID1..GRID12 are used
MAX_GRIDS_TO_SHOW = 12

# Optional common 7-seg names. This is just labeling; pins are still SEG1..SEG8.
SEG_LABEL = {0: "a", 1: "b", 2: "c", 3: "d", 4: "e", 5: "f", 6: "g", 7: "dp"}

def pick_cols(fieldnames):
    lowers = [s.lower() for s in fieldnames]
    time_i = None
    for i, n in enumerate(lowers):
        if "time" in n:
            time_i = i
            break
    non_time = [fieldnames[i] for i in range(len(fieldnames)) if i != time_i]

    clk = None
    din = None
    for c in non_time:
        lc = c.lower()
        if clk is None and ("clk" in lc or "sclk" in lc):
            clk = c
        elif din is None and ("din" in lc or "dio" in lc or "data" in lc):
            din = c

    tcol = fieldnames[time_i] if time_i is not None else fieldnames[0]
    if clk and din:
        return tcol, clk, din

    # fallback: first two non-time cols
    return tcol, non_time[0], non_time[1]

def bits_set(byte):
    return [i for i in range(8) if (byte >> i) & 1]

def segs_on(byte, active_low):
    if not active_low:
        return [i for i in range(8) if (byte >> i) & 1]
    return [i for i in range(8) if ((byte >> i) & 1) == 0]

def fmt_grid(grid_idx, byte):
    on = segs_on(byte, ACTIVE_LOW)
    pins = [f"SEG{i+1}" for i in on]
    return f"GRID{grid_idx+1}: 0x{byte:02X}  " + (",".join(pins) if pins else "-")

with open(FN, newline="") as f:
    r = csv.DictReader(f)
    tcol, cclk, cdin = pick_cols(r.fieldnames)
    rows = [(float(x[tcol]), int(x[cclk]), int(x[cdin])) for x in r]

# --- low-level decode (START/END + bytes) ---
clk_lvl, din_lvl = rows[0][1], rows[0][2]
in_frame = False
skip_ack = False
bitcnt = 0
cur = 0
frame = []
t_start = 0.0
frames = []

for i in range(1, len(rows)):
    ti, new_clk, new_din = rows[i]

    # START/END: DIN edge while CLK high (per TM1640 timing)
    if new_din != din_lvl and clk_lvl == 1:
        if din_lvl == 1 and new_din == 0:  # START
            in_frame = True
            skip_ack = False
            bitcnt = 0
            cur = 0
            frame = []
            t_start = ti
        elif din_lvl == 0 and new_din == 1:  # END
            if in_frame:
                frames.append((t_start, ti, frame[:]))
            in_frame = False
            skip_ack = False
            bitcnt = 0
            cur = 0
            frame = []

    # sample on CLK rising edge, LSB first, skip ACK clock
    if in_frame and (clk_lvl == 0 and new_clk == 1):
        if skip_ack:
            skip_ack = False
        else:
            cur |= ((new_din & 1) << bitcnt)
            bitcnt += 1
            if bitcnt == 8:
                frame.append(cur)
                cur = 0
                bitcnt = 0
                skip_ack = True

    clk_lvl, din_lvl = new_clk, new_din

# --- high-level interpret (commands + RAM updates) ---
ram = [0x00] * 16
data_mode = "auto"      # 0x40 auto-inc, 0x44 fixed
pending_write = False

def print_lit(t0, t1):
    print(f"\n{t0:12.6f} .. {t1:12.6f}  RAM snapshot (ACTIVE_LOW={ACTIVE_LOW})")
    for g in range(min(MAX_GRIDS_TO_SHOW, 16)):
        b = ram[g]
        print("  " + fmt_grid(g, b))
    pairs = []
    for g in range(min(MAX_GRIDS_TO_SHOW, 16)):
        for s in segs_on(ram[g], ACTIVE_LOW):
            pairs.append(f"(GRID{g+1},SEG{s+1})")
    print("  pairs:", " ".join(pairs) if pairs else "(none)")

for (t0, t1, fr) in frames:
    if not fr:
        continue

    b0 = fr[0]

    # Data command frame (usually just 1 byte)
    if (b0 & 0xC0) == 0x40:
        data_mode = "fixed" if (b0 & 0x04) else "auto"
        continue

    # Address + data frame: EVERYTHING after the first byte is display data until END
    if (b0 & 0xF0) == 0xC0:
        addr = b0 & 0x0F
        for k, db in enumerate(fr[1:]):
            a = addr if data_mode == "fixed" else ((addr + k) & 0x0F)
            ram[a] = db
        pending_write = True
        continue

    # Display control frame: good point to print after a write burst
    if (b0 & 0xC0) == 0x80:
        if pending_write:
            print_lit(t0, t1)
            pending_write = False
        continue

# If the capture ends right after a write burst
if pending_write and frames:
    print_lit(frames[-1][0], frames[-1][1])
