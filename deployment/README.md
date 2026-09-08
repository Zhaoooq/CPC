# CPC_1 boot deployment

This directory contains the Raspberry Pi boot splash and desktop autostart files.

- `plymouth/cpc-gxy/`: black Plymouth screen with the supplied white GXY logo.
- `autostart/cpc-1.desktop`: launches `/home/pi/Desktop/CPC/CPC_1` after desktop autologin.
- `session/`: user session startup files that omit LXPanel and wf-panel-pi.
- `eeprom/boot.conf`: disables the Raspberry Pi 5 network-install and HDMI diagnostic screen.
- `disable-system-panels.sh`: removes panel launch lines from the system LXDE/labwc sessions.
- `polkit/49-cpc-network.rules`: grants the dedicated `cpc-network` group only the
  NetworkManager permissions needed to edit and activate connection profiles.
- `install-network-permissions.sh`: idempotently creates that group, adds the CPC
  desktop user, backs up an existing rule once, and installs the PolicyKit rule.
- `../start-cpc-1.sh`: launcher used by the desktop autostart entry.
- `install.sh` also disables the duplicate `wayvnc` service (RealVNC remains enabled)
  and removes the unnecessary `NetworkManager-wait-online` boot delay.

The target machine uses LightDM desktop autologin for user `pi`. The panel is configured
separately in the user's `wf-panel-pi.ini` and LXPanel profile so either desktop backend
automatically hides the top panel.

Install the restricted network permission separately, then reboot so the desktop
session receives its new group membership:

```bash
cd /home/pi/Desktop/CPC
sudo deployment/install-network-permissions.sh pi
sudo reboot
```

Do not launch `CPC_1` with `sudo`. The rule does not grant unrestricted sudo or
arbitrary shell execution; it is limited to NetworkManager profile modification
and activation. The CPC application itself only targets `eth0`, or an Ethernet
fallback when `eth0` does not exist, and never selects Wi-Fi, Bluetooth, or VPN.
