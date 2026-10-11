# Starter KiCad board

This is the reserved location for the device template's starting KiCad board. No schematic, PCB, selected components, hardware revision, or manufacturing-ready design is provided yet.

## Intended starting point

Pair a real STM32G0B1 board target with the hello-world firmware. Provide USB-C for standalone HID plus CDC serial and two parallel CAN connectors for linear pass-through. The module baseline uses external 12 V carried on dedicated supply conductors alongside CAN, local regulation, and protected power selection without backfeeding USB. Connector/cable/pinout qualification and exact component choices remain pending.

## Files to provide during implementation

- Editable KiCad project, schematic, and PCB sources under a named board revision.
- Electrical BOM and connector/pinout documentation.
- Power limits, protection behavior, and a debug/recovery connection.
- Matching firmware board target and flashing instructions.
- ERC/DRC results and physical USB/CAN, power, and recovery validation.

Keep the starting board independent of mechanical Lite/Pro editions so they can share it when compatible. This template is a module starting point; five-port gateway hardware remains a separate undecided design.
