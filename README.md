# HFD Service

**H**uman **F**eedback **D**evice **Service** is a DBus activated service that manages human feedback devices such as leds and vibrators on mobile devices.

It replaces usensord and Unity8's own LED/light handler.

## Why?

We have a need for a more modular system running service now that we have different devices handling implementations differently.

Secondly, we could not use Unity8's own LED handler which only runs in userspace from where it cannot access sysfs devices in a secure manner.

We thus use a DBus service protected with AppArmor.
