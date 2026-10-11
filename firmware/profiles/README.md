# Control profile

The hello-world profile in `src/usb_descriptors.c` is one physical joystick button, with no axes and seven report-padding bits. USB identity uses explicitly supplied VID/PID and the MCU UID. The profile is an example, not a registered production device model.

New devices define their own reports and capabilities here or in their application sources. Shared descriptor helpers stay in firmware-common. Lite/Pro mechanical editions may use the same profile.
