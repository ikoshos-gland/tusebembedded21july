# sEMG Hand Prosthesis - Hardware Design Guide

## 1. System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                          Power Supply Section                        │
│  ┌─────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐      │
│  │ 7.4V    │───▶│ 5V Buck  │───▶│ 3.3V LDO │───▶│ 1.8V LDO │      │
│  │ Battery │    │ (Servos) │    │ (Digital)│    │ (Analog) │      │
│  └─────────┘    └──────────┘    └──────────┘    └──────────┘      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                         Analog Front-End (AFE)                       │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐     │
│  │ EMG      │───▶│ Inst.Amp │───▶│ Filters  │───▶│ ADS1299  │     │
│  │ Electrodes│    │ (G=1000) │    │ 20-500Hz │    │ 24-bit   │     │
│  │ (4 diff) │    │          │    │          │    │ ADC      │     │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘     │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                          Digital Section                             │
│  ┌──────────┐    ┌──────────────────┐    ┌──────────┐             │
│  │ ADS1299  │───▶│ STM32H7S3L8      │───▶│ Servo    │             │
│  │ (SPI)    │    │ NUCLEO Board     │    │ Drivers  │             │
│  └──────────┘    │                  │    │ (6x PWM) │             │
│                  │                  │    └──────────┘             │
│  ┌──────────┐    │                  │    ┌──────────┐             │
│  │ LIS3DH   │───▶│                  │───▶│ Status   │             │
│  │ (I2C)    │    │                  │    │ LEDs     │             │
│  └──────────┘    └──────────────────┘    └──────────┘             │
└─────────────────────────────────────────────────────────────────────┘
```

## 2. EMG Analog Front-End Design

### 2.1 Electrode Interface Circuit (Per Channel)

```
                    ┌─────────────────────────────────┐
                    │      Instrumentation Amplifier   │
    EMG+ ───────R1──┤IN+                              │
             1kΩ    │         INA333                  │
                    │                            OUT──┼───▶ To Filter
    EMG- ───────R2──┤IN-                              │
             1kΩ    │                                 │
                    │         G = 1 + (100kΩ/RG)      │
                    └──────RG──────┘                  │
                         100Ω (G=1001)                │
                                                      │
    REF ──────────────────────────────────────────────┘

Protection Circuit:
    EMG+ ──┬── D1 ──┬── VDD_PROTECT (3.6V)
           │        │
           ├── D2 ──┴── GND
           │
           R1 (1kΩ)
```

### 2.2 Active Bandpass Filter (20-500 Hz)

```
High-Pass Stage (20 Hz):                Low-Pass Stage (500 Hz):
                                        
IN ──┬── C1 ──┬── (+)Op-Amp1 ──┬──────┬── C3 ──┬── (+)Op-Amp2 ──┬── OUT
     │  10nF  │               │      │  1nF   │               │
     │        R2              │      R4        R6              │
     │        82kΩ            │      10kΩ      10kΩ            │
     │        │               │      │         │               │
     └────────┴───────────────┘      └─────────┴───────────────┘
              GND                              GND

Component Values:
- Op-Amps: TLV2372 (Rail-to-rail, low noise)
- fc_high = 1/(2π × R2 × C1) = 19.4 Hz
- fc_low = 1/(2π × R4 × C3) = 15.9 kHz (then divided by gain)
```

### 2.3 ADS1299 Connection Schematic

```
STM32H7 SPI Master              ADS1299 (EMG ADC)
┌─────────────┐                 ┌─────────────────┐
│             │                 │                 │
│ PA5 (SCK) ──┼────────────────▶│ SCLK           │
│ PA6 (MISO)◀─┼─────────────────┤ DOUT           │
│ PA7 (MOSI)──┼────────────────▶│ DIN            │
│ PA4 (CS) ───┼────────────────▶│ CS             │
│ PB0 (DRDY)◀─┼─────────────────┤ DRDY           │
│ PB1 (START)─┼────────────────▶│ START          │
│ PB2 (RESET)─┼────────────────▶│ RESET          │
│             │                 │                 │
│             │                 │ IN1P ◀──────────┤ CH1+
│             │                 │ IN1N ◀──────────┤ CH1-
│             │                 │ IN2P ◀──────────┤ CH2+
│             │                 │ IN2N ◀──────────┤ CH2-
│             │                 │ IN3P ◀──────────┤ CH3+
│             │                 │ IN3N ◀──────────┤ CH3-
│             │                 │ IN4P ◀──────────┤ CH4+
│             │                 │ IN4N ◀──────────┤ CH4-
│             │                 │                 │
└─────────────┘                 └─────────────────┘

Power Supply:
- AVDD: 3.3V (analog, separate LDO)
- AVSS: AGND
- DVDD: 3.3V (digital)
- DGND: Digital ground
```

### 2.4 Right Leg Drive (RLD) Circuit

```
                 ┌─────────────────┐
                 │   Common Mode    │
All IN- ─────────┤   Amplifier     │
                 │                  │
                 │   Gain = -39     ├──── RLD Electrode
                 │                  │
                 └─────────────────┘
                 
Improves CMRR by actively driving the body to cancel common-mode interference
```

## 3. Digital Interface Design

### 3.1 LIS3DH Accelerometer Connection

```
STM32H7 I2C1                    LIS3DH
┌─────────────┐                 ┌─────────────────┐
│             │                 │                 │
│ PB8 (SCL) ──┼──── 4.7kΩ ──VDD─┤ SCL            │
│ PB9 (SDA) ──┼──── 4.7kΩ ──VDD─┤ SDA            │
│ PC0 (INT1)◀─┼─────────────────┤ INT1           │
│             │                 │                 │
│             │                 │ VDD ◀───────────┤ 3.3V
│             │                 │ GND ◀───────────┤ GND
│             │                 │ CS  ◀───────────┤ VDD (I2C mode)
│             │                 │                 │
└─────────────┘                 └─────────────────┘

I2C Address: 0x18 (SA0 = GND) or 0x19 (SA0 = VDD)
```

### 3.2 Servo Control Interface

```
STM32H7 TIM1 PWM                Servo Connectors
┌─────────────┐                 ┌─────────────────┐
│             │                 │ Servo 1 (Thumb) │
│ PE9 (CH1) ──┼─────────────────┤ Signal (Orange) │
│             │                 │ VCC (Red) ◀─────┤ 5V
│             │                 │ GND (Brown) ◀───┤ GND
│             │                 └─────────────────┘
│ PE11 (CH2)──┼────────────────▶ Servo 2 (Index)
│ PE13 (CH3)──┼────────────────▶ Servo 3 (Middle)
│ PE14 (CH4)──┼────────────────▶ Servo 4 (Ring)
│ PA8 (CH1N)──┼────────────────▶ Servo 5 (Pinky)
│ PA9 (CH2N)──┼────────────────▶ Servo 6 (Wrist)
│             │
└─────────────┘

PWM Settings:
- Frequency: 50 Hz (20ms period)
- Pulse width: 1-2ms (0-180°)
- Timer clock: 200 MHz
- Prescaler: 199 (1 MHz timer)
- Period: 19999 (20ms)
```

## 4. Power Supply Design

### 4.1 Power Architecture

```
7.4V Battery ──┬── Protection ──┬── 5V Buck ──┬── Servos (6×500mA peak)
(2S LiPo)      │   Circuit      │  (TPS54331) │
               │                │             └── 3.3V LDO ──┬── Digital
               │                │                (AMS1117)   │
               │                └── Charge Port              └── 1.8V LDO ── Analog Ref
               │                    (USB-C PD)                   (TPS79318)
               │
               └── Battery Monitor (INA219)
```

### 4.2 5V Buck Converter (Servo Supply)

```
VIN (7.4V) ──┬── L1 ──┬── SW (TPS54331) ──┬── L2 ──┬── VOUT (5V/3A)
             │  10µH  │                   │  22µH  │
             C1       │                   D1       C3
             10µF     │                   │        47µF
             │        └── FB ──R1──┬──R2──┘        │
             GND              10kΩ │ 2.2kΩ         GND
                                   │
                                   └── Vref (0.8V)

Additional Components:
- Input capacitor: 10µF ceramic + 100µF electrolytic
- Output capacitor: 47µF ceramic + 220µF electrolytic
- Schottky diode: SS34 (3A, 40V)
- Inductor: 22µH, 3A saturated, low DCR
```

### 4.3 Analog Power Supply (Low Noise)

```
3.3V Digital ──┬── Ferrite ──┬── π-Filter ──┬── 1.8V LDO ──┬── AVDD
               │   Bead       │              │  (TPS79318)  │
               │              C1    L1   C2  │              C3
               │              10µF 10µH 10µF │              10µF
               │              │     │    │   │              │
               └──────────────┴─────┴────┴───┴──────────────┴── AGND

Noise Performance:
- Output noise: <30µVRMS (10Hz-100kHz)
- PSRR: >70dB @ 1kHz
- Load regulation: <0.1%
```

## 5. PCB Layout Guidelines

### 5.1 Layer Stack-up (4-layer)

```
┌─────────────────────────┐
│ Top: Components, Signal │ 1.6mm FR4
├─────────────────────────┤ 0.2mm prepreg
│ Layer 2: Ground Plane   │ 0.5mm core
├─────────────────────────┤ 0.2mm prepreg
│ Layer 3: Power Planes   │ 0.5mm core
├─────────────────────────┤ 0.2mm prepreg
│ Bottom: Signal, Ground  │
└─────────────────────────┘
```

### 5.2 Critical Layout Rules

1. **Analog Section**
   - Separate analog and digital grounds, connect at single point
   - Keep EMG traces short and differential
   - Guard rings around sensitive analog circuits
   - No digital signals under analog section

2. **Power Section**
   - Wide traces for high current (>2mm for servo power)
   - Thermal vias under regulators
   - Input/output capacitors close to regulators
   - Star grounding for power returns

3. **Digital Section**
   - Length-match SPI traces (±5mm)
   - 50Ω impedance control for high-speed signals
   - Decoupling capacitors within 5mm of each IC
   - Crystal oscillator away from switching regulators

### 5.3 EMI/EMC Considerations

```
Shielding Plan:
┌─────────────────────────────────────┐
│ Metal Enclosure (Connected to GND)  │
│ ┌─────────────────────────────────┐ │
│ │   Analog Section (Shielded)     │ │
│ │ ┌─────────────┐ ┌─────────────┐ │ │
│ │ │ EMG Input   │ │ ADS1299     │ │ │
│ │ │ Connectors  │ │ ADC         │ │ │
│ │ └─────────────┘ └─────────────┘ │ │
│ └─────────────────────────────────┘ │
│                                     │
│ ┌─────────────────────────────────┐ │
│ │   Digital Section               │ │
│ │ ┌─────────────┐ ┌─────────────┐ │ │
│ │ │ STM32H7     │ │ Power       │ │ │
│ │ │ MCU         │ │ Supplies    │ │ │
│ │ └─────────────┘ └─────────────┘ │ │
│ └─────────────────────────────────┘ │
└─────────────────────────────────────┘
```

## 6. Component Selection

### 6.1 Critical Components BOM

| Component | Part Number | Specifications | Quantity | Notes |
|-----------|------------|----------------|----------|-------|
| **Analog Front-End** |
| EMG ADC | ADS1299-4 | 4-ch, 24-bit, SPI | 1 | Medical grade |
| Inst. Amp | INA333 | Low noise, G=1-1000 | 4 | CMRR >100dB |
| Op-Amp | TLV2372 | Rail-to-rail, low noise | 8 | Dual package |
| **Digital** |
| MCU Board | NUCLEO-H7S3L8 | STM32H7, 280MHz | 1 | Development board |
| Accelerometer | LIS3DH | 3-axis, I2C, low power | 1 | ±2g range |
| **Power** |
| Buck Converter | TPS54331 | 3A, 28V input | 1 | 90% efficiency |
| LDO 3.3V | AMS1117-3.3 | 1A, low dropout | 1 | Digital supply |
| LDO 1.8V | TPS79318 | 200mA, low noise | 1 | Analog reference |
| Battery Monitor | INA219 | I2C, current/voltage | 1 | Optional |
| **Connectors** |
| EMG Connector | 3.5mm TRRS | 4-pole audio jack | 4 | Shielded |
| Servo Connector | JST-XH 3P | 2.5mm pitch | 6 | Locking |
| Debug Port | 10-pin Cortex | SWD + UART | 1 | Standard ARM |

### 6.2 Passive Components

| Type | Value | Package | Tolerance | Quantity | Application |
|------|-------|---------|-----------|----------|-------------|
| **Resistors** |
| SMD | 1kΩ | 0603 | 1% | 20 | Pull-ups, protection |
| SMD | 10kΩ | 0603 | 1% | 30 | Biasing, dividers |
| SMD | 100Ω | 0603 | 0.1% | 4 | Gain setting |
| **Capacitors** |
| Ceramic | 100nF | 0603 | X7R | 50 | Decoupling |
| Ceramic | 10µF | 0805 | X5R | 20 | Bulk decoupling |
| Tantalum | 47µF | Case-B | 20% | 4 | Power filtering |
| **Inductors** |
| Power | 22µH | 1210 | 20% | 2 | Buck converter |
| Ferrite | 600Ω@100MHz | 0805 | - | 10 | EMI suppression |

## 7. Mechanical Integration

### 7.1 Electrode Placement Map

```
Forearm Cross-Section (Looking from elbow toward hand):

        Dorsal (Top)
           CH3
    ┌──────┴──────┐
CH4 │             │ CH2    Legend:
    │   Forearm   │        CH1: Flexor Carpi Radialis
    │             │        CH2: Flexor Digitorum
    └──────┬──────┘        CH3: Extensor Digitorum
           CH1             CH4: Extensor Carpi Ulnaris
       Volar (Palm)        REF: Elbow (bony prominence)
```

### 7.2 Enclosure Design Requirements

1. **Main Electronics Box** (120×80×30mm)
   - Aluminum for EMI shielding
   - Separate compartments for analog/digital
   - Ventilation slots for thermal management
   - IP54 rating for sweat resistance

2. **Electrode Module** (30×20×10mm each)
   - Snap-on dry electrodes
   - Flexible cable with strain relief
   - Quick-disconnect connectors

3. **Servo Pack** (150×100×50mm)
   - Modular mounting system
   - Cable management channels
   - Access panels for maintenance

## 8. Safety Considerations

### 8.1 Electrical Safety

1. **Isolation**
   - Patient isolation: >5kV
   - Leakage current: <10µA
   - Defibrillator protection

2. **Protection Circuits**
   - TVS diodes on all patient connections
   - Current limiting resistors (1kΩ minimum)
   - Isolated power supplies for analog section

### 8.2 Mechanical Safety

1. **Servo Limits**
   - Software end-stops
   - Mechanical stops as backup
   - Torque limiting in software
   - Emergency stop button

2. **Thermal Management**
   - Temperature monitoring
   - Thermal shutdown at 60°C
   - Heat sinks on power components

## 9. Testing & Validation

### 9.1 Electrical Tests

| Test | Specification | Method |
|------|--------------|--------|
| Input Noise | <2µVRMS | Short inputs, measure |
| CMRR | >100dB @ 50Hz | Apply common signal |
| Bandwidth | 20-500Hz ±3dB | Sweep generator |
| ADC Linearity | <0.1% FSR | Precision source |
| Power Efficiency | >85% | Load testing |

### 9.2 EMC Testing

1. **Emissions** (CISPR 11 Class B)
   - Radiated: 30MHz-1GHz
   - Conducted: 150kHz-30MHz

2. **Immunity** (IEC 61000-4)
   - ESD: ±8kV contact
   - RF: 80MHz-2.7GHz, 3V/m
   - Power line transients

## 10. Manufacturing Files

### Required Outputs:
1. **PCB Files**
   - Gerbers (RS-274X format)
   - Pick & place files
   - 3D model (STEP format)
   - Assembly drawings

2. **Mechanical Files**
   - Enclosure drawings (DXF/DWG)
   - 3D print files (STL)
   - Assembly instructions

3. **Documentation**
   - Schematic PDF
   - BOM with suppliers
   - Test procedures
   - Calibration guide