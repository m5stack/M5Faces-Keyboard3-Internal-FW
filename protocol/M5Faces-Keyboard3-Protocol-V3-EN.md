# M5Faces-Keyboard3 Control Protocol (Firmware v03)

---

## 1. Hardware Interface

| Item | Description |
| :--- | :--- |
| I2C slave address (7-bit) | `0x08` |
| Communication speed | Standard 100 kbps / Fast 400 kbps |
| IRQ pin state | Low: new data available; High: idle |

### 1.1 IRQ Timing

When new key data is generated, `IRQ` is pulled low. After the host finishes reading the data, `IRQ` returns high.

```text
Key pressed                 Host read complete
   |                              |
---+ IRQ low (0) -----------------+ IRQ high (1)
```

---

## 2. Operating Modes

The module enters **Normal mode** by default after power-on.  
The host can switch operating modes using command `0xF0`.

| Mode | Value | Description |
| :--- | :--- | :--- |
| Normal | `0x00` | The firmware maps keys; read **1 byte of ASCII data** per IRQ |
| Direct | `0x01` | Outputs raw key matrix data; read a **fixed 10-byte packet** per IRQ |

> **Note:**
> The firmware automatically clears the matrix state history whenever the mode is changed.

---

## 3. Register Address Map

| Register address | Readable | Writable | Description |
| :--- | :--: | :--: | :--- |
| `0xD0` | Yes | No | Device type ID |
| `0xE0~0xEB` | Yes | No | 96-bit UID, ordered by ascending chip UID storage address, 12 bytes total |
| `0xF0` | Yes | Yes | Operating mode (Normal / Direct) |
| `0xF1` | Yes | Yes | LED mode |
| `0xFE` | Yes | No | Firmware version |
| `0xFF` | Yes | Yes | I2C address register |

---

## 4. Write Operations (Host to Module)

The write format is `[register address] [data]`, with a fixed total length of **2 bytes**.

| Register address | Data | Function |
| :--- | :--- | :--- |
| `0xF0` | `0x00` | Switch to Normal mode |
| `0xF0` | `0x01` | Switch to Direct mode |
| `0xF1` | `0x00`-`0xFF` | Set the LED mode (see Section 8) |
| `0xFF` | A value within the valid address range | Change the module's I2C address; takes effect immediately (see Section 4.1 for the range) |

### 4.1 I2C Address

After a new address is written to `0xFF`, the configuration takes effect immediately. The operation takes approximately `20 ms`, and the setting persists after power-off.  

Ensure that the address value is within the valid I2C address range allowed by the system.

---

## 5. Read Operations (Module to Host)

### 5.1 Read Procedure

1. The host first writes a **1-byte register address**.
2. The host then initiates an **I2C read**.
3. The module returns data sequentially from that register address.

### 5.2 Readable Register Blocks

#### Block A: Device Type ID

| Start address | Readable bytes | Returned data |
| :--- | :---: | :--- |
| `0xD0` | 1 | `[device_id]` |

#### Block B: UID

| Start address | Maximum readable bytes | Returned data |
| :--- | :---: | :--- |
| `0xE0~0xEB` | `0xEC - start address` | Remaining UID data starting at the corresponding offset |

The UID is read and cached when the device powers on. The host can start at any address from `0xE0` through `0xEB` and read up to the number of bytes remaining through `0xEB`.

#### Block C: Operating Mode + LED Mode

| Start address | Readable bytes | Returned data |
| :--- | :---: | :--- |
| `0xF0` | 2 | `[operation_mode, led_mode]` |
| `0xF1` | 1 | `[led_mode]` |

#### Block D: Firmware Version + I2C Address

| Start address | Readable bytes | Returned data |
| :--- | :---: | :--- |
| `0xFE` | 2 | `[firmware_version, i2c_addr_reg]` |
| `0xFF` | 1 | `[i2c_addr_reg]` |

---

## 6. Normal Mode

In **Normal mode**, the firmware maps keys, and the host reads the **ASCII data** corresponding to each key.

### 6.1 Modifier Priority

If multiple modifier keys are active at the same time, the output is determined by the following priority, from highest to lowest:

```text
ALT > SYM > FN > aA > default
```

The corresponding mappings are:

| Modifier state | Mapping column used |
| :--- | :--- |
| No modifier | `KeyMap[n][0]` (lowercase / default) |
| `aA` | `KeyMap[n][1]` (uppercase) |
| `SYM` | `KeyMap[n][2]` (symbols) |
| `FN` | `KeyMap[n][3]` (extended functions) |
| `ALT` | `KeyMap[n][4]` (144-178) |

### 6.2 Special Handling for the `Enter` Key

The `Enter` key does not use `KeyMap`. Instead, it always outputs:

- First read: `0x0D` (`\r`)
- Second read: `0x0A` (`\n`)

In other words, pressing `Enter` generates **one IRQ**, and the host must perform two consecutive reads. The interrupt is cleared only after both reads are complete.

> **Note:**
> Although Normal mode is generally described as reading 1 byte of ASCII data per IRQ, the `Enter` key is a special case and requires two consecutive reads.

---

## 7. Direct Mode

In **Direct mode**, the module directly outputs raw key matrix data.

### 7.1 Trigger Condition

Whenever the state of any key changes, `IRQ` is pulled low.  
The host must read a **fixed 10-byte packet** each time.

### 7.2 Packet Format

| Offset | Content |
| :--- | :--- |
| Byte 0 | Data length, fixed at `0x0A` (10) |
| Byte 1-2 | Matrix row 0 (`OUTPUT_MODE_1`) |
| Byte 3-4 | Matrix row 1 (`OUTPUT_MODE_2`) |
| Byte 5-6 | Matrix row 2 (`OUTPUT_MODE_3`) |
| Byte 7-8 | Modifier key row |
| Byte 9 | Checksum |

### 7.3 Format of Each 2-Byte Group

```text
High byte (Byte N):
  Bit 7   : Change flag (1 = current data differs from previous data)
  Bit 6-4 : Row index (0-3)
  Bit 1-0 : GPIOA IDR bits 9-8 (PA9, PA8)

Low byte (Byte N+1):
  Bit 7-0 : GPIOA IDR bits 7-0 (PA7-PA0)
```

### 7.4 Combining the 10-Bit Raw Value

```c
raw = ((high_byte & 0x03) << 8) | low_byte;
```

> **Logic levels:**
>
> - `0` = Pressed
> - `1` = Released
>
> This is because the key inputs use pull-up resistors.

### 7.5 Modifier Key Row

| Bit | Key |
| :--- | :--- |
| 0 | `aA` |
| 1 | `ALT` |
| 2 | `Enter` |
| 3 | `SYM` |
| 4 | `FN` |
| 5-7 | Reserved, always `1` |

---

## 8. LED Effect Control (Command `0xF1`)

### 8.1 Preset LED Effect Modes (`0x00`-`0x08`)

| Value | Trigger | LED effect |
| :--- | :--- | :--- |
| `0` | No modifier | Both LEDs off |
| `1` | Single-click `aA` | Left LED steady on |
| `2` | Double-click `aA` to lock | Left LED flashes slowly (500 ms) |
| `3` | `ALT` active | Left LED flashes quickly (150 ms) |
| `4` | Single-click `FN` | Right LED steady on |
| `5` | Double-click `FN` to lock | Right LED flashes slowly (500 ms) |
| `6` | Double-click `SYM` to lock | Right LED flashes quickly (150 ms) |
| `7` | Single-click `SYM` | Left and right LEDs alternate slowly (500 ms) |
| `8` | External setting | Left and right LEDs alternate quickly (200 ms) |

### 8.2 Manual Control Mode (Bit 7 = 1)

When Bit 7 of the parameter is `1`, manual bit control mode is enabled.

#### Bit Definitions

| Bit | Function |
| :--- | :--- |
| 7 | Must be `1` |
| 4 | `1` = Left LED on |
| 5 | `1` = Right LED on |
| Other bits | Reserved |

#### Common Examples

| Value sent | Effect |
| :--- | :--- |
| `0x80` | Both LEDs off |
| `0x90` | Left on, right off |
| `0xA0` | Left off, right on |
| `0xB0` | Both LEDs on |

---

## 9. Modifier Key Logic (Normal Mode)

### 9.1 State Transitions for `aA` / `FN` / `SYM`

These three keys use the same state machine:

```text
State 0 (Off)
  | Single-click
  v
State 1 (One-shot) -- Press a regular key --> Automatically returns to State 0
  | Single-click again
  v
State 2 (Locked)
  | Single-click again
  v
State 0
```

In simple terms:

- **Single-click once**: applies only to the next regular key press
- **Quick double-click**: enters the locked state
- **Single-click while locked**: turns it off

> `ALT` does not use this logic.  
> `ALT` is a momentary key: it is active while pressed and becomes inactive immediately when released. It does not support double-click locking.

### 9.2 Modifier Mutual Exclusion

When any modifier key becomes active, all other modifier keys are cleared.  
Only one modifier key can be active at a time.

---

## 10. Key Mapping Table (Normal Mode)

| Index | Default | aA | SYM | FN | ALT |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 0 | `q` | `Q` | `#` | `~` | 144 |
| 1 | `w` | `W` | `1` | `^` | 145 |
| 2 | `e` | `E` | `2` | `&` | 146 |
| 3 | `r` | `R` | `3` | `` ` `` | 147 |
| 4 | `t` | `T` | `(` | `<` | 148 |
| 5 | `y` | `Y` | `)` | `>` | 149 |
| 6 | `u` | `U` | `_` | `{` | 150 |
| 7 | `i` | `I` | `-` | `}` | 151 |
| 8 | `o` | `O` | `+` | `[` | 152 |
| 9 | `p` | `P` | `@` | `]` | 153 |
| 10 | `a` | `A` | `*` | `\|` | 154 |
| 11 | `s` | `S` | `4` | `=` | 155 |
| 12 | `d` | `D` | `5` | `\` | 156 |
| 13 | `f` | `F` | `6` | `%` | 157 |
| 14 | `g` | `G` | `/` | 180 | 158 |
| 15 | `h` | `H` | `:` | 181 | 159 |
| 16 | `j` | `J` | `;` | 182 | 160 |
| 17 | `k` | `K` | `'` | 183 | 161 |
| 18 | `l` | `L` | `"` | 184 | 162 |
| 19 | 8 | 8 | 127 | 8 | 163 |
| 20 | 255 | 255 | 255 | 255 | 255 |
| 21 | `z` | `Z` | `7` | 186 | 165 |
| 22 | `x` | `X` | `8` | 187 | 166 |
| 23 | `c` | `C` | `9` | 188 | 167 |
| 24 | `v` | `V` | `?` | 189 | 168 |
| 25 | `b` | `B` | `!` | 190 | 169 |
| 26 | `n` | `N` | `,` | 191 | 170 |
| 27 | `m` | `M` | `.` | 192 | 171 |
| 28 | `$` | `$` | 255 | 193 | 172 |
| 29 | 13 | 13 | 13 | 13 | 13 |
| 30 | 255 | 255 | 255 | 255 | 255 |
| 31 | `0` | `0` | `0` | `0` | 175 |
| 32 | ` ` | ` ` | ` ` | `S ` | 176 |
| 33 | 255 | 255 | 255 | 255 | 255 |
| 34 | 255 | 255 | 255 | 255 | 255 |

> 255: Invalid value; the host should ignore it.

---

## 11. Notes

- Normal mode is suitable when the host reads characters directly.
- Direct mode is suitable when the host processes the matrix scan results itself.
- To support the `Enter` key, the host must specifically handle its **two-read** behavior.
- For reliable interrupt handling, communicate strictly by reading after `IRQ` is pulled low and waiting for it to return high after the read is complete.
