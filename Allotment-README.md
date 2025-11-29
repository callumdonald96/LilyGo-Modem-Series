# 🌦️ Modular Allotment Weather Station — Project Plan

This document outlines the design, build, and expansion plan for a modular allotment weather station.  
It defines the project phases from proof-of-concept to a fully autonomous smart irrigation system, along with component requirements for each subsystem.

---

## 📘 Milestone Summary

| Phase | Focus | Key Additions | Outcome |
|:------|:------|:--------------|:---------|
| [0 — Proof of Concept](#phase-0--proof-of-concept-lab-bench) | Bench validation | Microcontroller + initial sensors → AWS → Grafana | End-to-end data flow established |
| [1 — Outdoor Basic Station with Modular Wiring](#phase-1--outdoor-basic-station-with-modular-wiring) | Weatherproof box, wiring plan, pole mount | Stable outdoor baseline |
| [2 — Solar & Power Management](#phase-2--solar--power-management) | Solar panel, battery, usage metrics | Autonomous, self-powered station |
| [3 — Extended Environmental Sensors](#phase-3--extended-environmental-sensors) | Additional sensors | Expanded data set without loss of stability |
| [4 — Data Platform Enhancements](#phase-4--data-platform-enhancements) | Dashboards, storage, alerting | Operational insights refined |
| [5 — Reservoir & Irrigation Monitoring](#phase-5--reservoir--irrigation-monitoring) | Water-butt monitoring | Water availability tracked |
| [6 — Automated Irrigation Control](#phase-6--automated-irrigation-control) | Closed-loop irrigation | Automated watering logic |
| [7 — Refinement & Expansion](#phase-7--refinement--expansion) | PCB, docs, OTA | Reproducible kit design |

---

## Phase 0 — Proof of Concept (Lab Bench)

**Goal:** Validate full data flow from sensor → AWS → Grafana.  
**Target completion:** **End of November 2025**

**Deliverables**
- Purchase and select suitable **microcontroller** (SIM-enabled, solar-compatible, MicroPython-supported)
- Purchase and select **initial sensors** (e.g. temperature, humidity, pressure)
- Establish **mobile network connectivity** (via SIM card) and data transfer from controller → AWS Lambda → Grafana Cloud
- Create **basic Grafana dashboard** displaying sensor data

**Success criteria**
- Microcontroller operates on test bench, producing full end-to-end data flow visible in Grafana Cloud

---

## Phase 1 — Outdoor Basic Station with Modular Wiring

**Goal:** Build and deploy the first outdoor station with stable, waterproof housing and modular wiring.  

**Deliverables**
- Design and install **weatherproof housing** (e.g. Spelsberg IC35 or equivalent)
- Create **mounting solution** (pole, clamps, positioning)
- Define and document **wiring plan** — shielded, modular, noise-resistant
- Source or design a **Stevenson screen** or equivalent sensor shield
- Select and test a **battery system** capable of sufficient runtime
- Verify full end-to-end data flow from outdoor station → Grafana Cloud

**Success criteria**
- Outdoor deployment running continuously  
- No water ingress or signal interference  
- End-to-end data confirmed in Grafana Cloud  

---

## Phase 2 — Solar & Power Management

**Goal:** Transition from replaceable batteries to a fully autonomous solar-powered system.  

**Deliverables**
- Integrate **solar panel** and **charge controller** to manage power input  
- Install **battery monitoring** to measure voltage, charge level, and usage  
- Update firmware to include power metrics and low-battery alerts  
- Configure Grafana alerts to notify when intervention may be needed  

**Success criteria**
- Station operates without battery replacement  
- Grafana metrics clearly show power usage and battery health  
- Alerting gives adequate warning before critical battery levels  

---

## Phase 3 — Extended Environmental Sensors

**Goal:** Add additional sensors while maintaining stability and power efficiency.  

**Success criteria**
- Deployment of extra sensors (e.g. soil, rainfall, wind, polytunnel)  
- Full end-to-end data flow maintained  
- No significant degradation of battery life, power stability, or SIM data usage  

---

## Phase 4 — Data Platform Enhancements

**Goal:** Strengthen data visualisation and alerting capabilities.  

**Success criteria**
- Grafana dashboards refined for readability  
- Data retention and alerting improved  
- Reliable visibility into long-term trends and key thresholds  

---

## Phase 5 — Reservoir & Irrigation Monitoring

**Goal:** Design and implement the water storage monitoring system.  

**Success criteria**
- Water-butt level measurable (e.g. ultrasonic or pressure sensor)  
- Soil moisture signals integrated  
- Correlation between soil condition and available water clearly visible  

---

## Phase 6 — Automated Irrigation Control

**Goal:** Enable automatic irrigation using available water reserves.  

**Success criteria**
- Automated watering triggered based on soil moisture and water-butt level  
- Manual override and fail-safes in place  
- Stable and reliable operation without manual intervention  

---

## Phase 7 — Refinement & Expansion

**Goal:** Finalise the design for reproducibility and scalability.  

**Success criteria**
- Custom PCB and 3D-printed mounts developed  
- Documentation complete for replication  
- OTA updates functional across all deployed units  

---

## ⚙️ Materials & Component Requirements

### Microcontroller
- Must support a **SIM card** for mobile data (4G / LTE / NB-IoT)  
- Must be **solar-compatible** (run from and charge batteries via solar input)  
- Must have **multiple sensor ports** (I²C, SPI, ADC, UART)  
- **No soldering required** — plug-in or terminal-block style connectors preferred  
- Must support **MicroPython firmware**  
- Should have **broad sensor compatibility** across brands  

### Power System
- 10–20 W solar panel with 5 V or 12 V output  
- MPPT or TP4056 charge controller for Li-ion/LiFePO₄  
- Battery pack (2–3 × 18650 cells)  
- Voltage/current sensor (INA219/INA3221)  
- Voltage regulation to 5 V / 3.3 V  

### Environmental Sensors
| Function | Sensor Options | Notes |
|-----------|----------------|-------|
| Air temp/humidity/pressure | BME280 / SHT31 | I²C interface |
| Soil temp | DS18B20 | Waterproof probe |
| Soil moisture | Capacitive v2.0 | Analog or I²C |
| Rainfall | Tipping bucket or piezo | Debounced digital input |
| Wind speed/direction | Reed switch + resistor ladder | Analog/digital |
| Light/UV | BH1750 / VEML6075 | I²C |
| CO₂ / air quality | SCD30 / ENS160 | Optional, I²C |
| Polytunnel sensors | Duplicate BME280s inside/outside | Compare readings |

### Reservoir & Irrigation Components
- **Water-butt level sensor:** ultrasonic (HC-SR04 waterproof) or pressure sensor  
- **Flow sensor:** hall-effect type  
- **Pump/valve:** 12 V DC with MOSFET or relay isolation  
- **Tubing & connectors:** garden hose compatible  
- **Manual override switch & fuse** for safety  

### Enclosure & Mounting
- IP66 polycarbonate junction box (e.g. *Spelsberg IC35*)  
- Cable glands (PG7/PG9) and waterproof vent plug  
- Desiccant packs  
- Aluminium or galvanised pole (~2 m) with U-bolt clamps  
- Optional Stevenson-screen-style vented shield for sensors  