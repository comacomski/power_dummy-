# power_dummy-

# Todo:
- make logic to set and change in time values for properties
- clean things
- expose debug knobs

`dummypower.c` - driver that works under QEMU and fully exercises the Linux power-supply subsystem **without any real hardware**.

---

# Goal

Create a kernel module that:
- pretends to be a charger
- exposes `/sys/class/power_supply/dummy_charger/…`
- reports fake voltage, current, status, etc.
- updates values on a timer (or sysfs knobs)
- requires **no real I²C/SPI/HW**
- runs under QEMU or any Linux machine

---

# DESIGN

# Simulate both layers:

## **1. Backend Layer (Fake Hardware Engine)**

Fake hardware model inside the driver:
- Some variables like:
    - `charging_enabled`
    - `voltage_now`
    - `current_now`
    - `temperature`
    - `status` (charging/discharging/full)   
- A timer and workqueue that periodically changes those values
- A function that simulates reading a register
- A function that simulates setting a register
    

This backend is _pure software_, just some integers and logic.

Example logic in words:

> Every second increase the battery voltage by 1000 µV until it reaches 4.2V, then switch status to FULL.

Or:

> If userspace writes `set_charge=1` to some debugfs file, the backend pretends charging is enabled.


### What to use to change values: IRQ, timer, deferred work: 
- Timer -softIRQ - to schedule_work - to queue workqueue [[timer]]
- Workqueue - to update values, lock mutexes, sleep, whatever logic is needed [[workqueue]]

---

## **2. Frontend Layer (Power-Supply Interface)**

This part behaves exactly like a real charger driver:

- Registers with `devm_power_supply_register()`
- Provides properties like:
    - `POWER_SUPPLY_PROP_STATUS`
    - `POWER_SUPPLY_PROP_VOLTAGE_NOW`
    - `POWER_SUPPLY_PROP_CURRENT_NOW`
- Implements `get_property()` and `set_property()`
- When the backend updates fake values, the driver:
    - calls `power_supply_changed(psy)` to notify userspace
        

This creates:
```
/sys/class/power_supply/dummy_charger/
    voltage_now
    current_now
    status
    online
    temp
    ...

```

Userspace tools like `upower`, `udev`, systemd, etc. will treat it as a real charger.

---

# Front-Back interaction

 Frontend will ask the fake backend:

> What is your current voltage?

and the backend returns whatever implemented software model says.

Or:

> Userspace wants to enable charging.

Frontend tells the backend:

> Set `charging_enabled = true`.

Then simulated backend logic adjusts the fake voltage/current over time.

---

#  FLOW of Operation

1. Module loads
    
2. Create a `dummy_chip` structure to hold simulated values
    
3. Set initial fake values:
    
    - voltage_now = 3700 mV
        
    - current_now = 500 mA
        
    - status = charging
        
4. Start a timer/workqueue that “ticks” every 1 second
    
5. On each tick:
    
    - voltage increases a bit
        
    - current slowly decreases
        
    - when voltage reaches 4200 mV, set status=full
        
    - notify userspace with `power_supply_changed()`
        
6. User runs:
`cat /sys/class/power_supply/dummy_charger/voltage_now`
→ Driver returns the fake value.
    

Everything works exactly like real hardware.

---

# Expose “debug knobs”

Add **debugfs** entries like:
```
/sys/kernel/debug/dummy_charger/set_voltage
/sys/kernel/debug/dummy_charger/trigger_fault
/sys/kernel/debug/dummy_charger/set_status

```
Userspace writes numbers to these files, and driver change the backend state.

This helps to test:

- notifications
- driver logic
- UI behavior
- Android battery service
- systemd power events
    
---

# Where does this run?

Anywhere:
- QEMU board
- Any Linux PC
- Virtual machine
- ARM QEMU with your test kernel
    
This dummy driver is architecture independent.
