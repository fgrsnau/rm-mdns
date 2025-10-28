# rm-mdns

This repository contains a tiny multicast DNS (mDNS) responder for the remarkable 2 e-ink tablet.

Instead of using the IP address of the remarkable you can connect to `remarkable.local`.
For example, to connect via SSH run `ssh root@remarkable.local`.

You can also specify a custom hostname by passing it as a argument. Note that it should end with a period to mark that fully qualified name, e.g., `rm-mdns my-custom-hostname.local.`.

## How to Build

To build a native version via nix you can run:

```shell
user@local$ git clone https://git.stha.de/stefan/rm-mdns.git
user@local$ cd rm-mdns
user@local$ nix-build -A native
```

To build a static binary that can run directly on the remarkable you can run:

```shell
user@local$ git clone https://git.stha.de/stefan/rm-mdns.git
user@local$ cd rm-mdns
user@local$ nix-build -A remarkable
```

For a non-nix build you can use meson directly.

```shell
user@local$ git clone https://git.stha.de/stefan/rm-mdns.git
user@local$ cd rm-mdns
user@local$ meson setup build
user@local$ meson compile -C build
```

Without nix you have to manually set up cross-compilation with the remarkable toolchain.
This is not covered by this README.

## How to Install

The following instruction will install the binary and the systemd unit to `/root`.
Afterwards the systemd unit is linked to the service directory and enabled with systemd.
Note that `/root` is on the system partition and is not encrypted only has a small amount of free space.
However, the statically linked `rm-mdns` binary is less than 50 KiB, so it does not consume much space.

```shell
user@local$ git clone https://git.stha.de/stefan/rm-mdns.git
user@local$ cd rm-mdns
user@local$ nix-build -A remarkable2
user@local$ scp result/bin/rm-mdns root@${IP_OF_REMARKABLE}:/root/rm-mdns
user@local$ scp result/bin/rm-mdns.service root@${IP_OF_REMARKABLE}:/root/rm-mdns.service
user@local$ ssh root@${IP_OF_REMARKABLE}
root@remarkable# ln -s /root/rm-mdns.service /etc/systemd/system
root@remarkable# systemctl enable --now rm-mdns
```

From now on you can now connect to your remarkable tablet with by running `ssh root@remarkable.local`.
To use a custom hostname change the `ExecStart` line in `rm-mdns.services`, e.g., `ExecStart=/root/rm-mdns my-custom-hostname.local.`.

If you are looking for prebuild binary, you can download them from <https://www.stha.de/shares/rm-mdns/>.

## How to Uninstall

```shell
user@local ssh root@${IP_OF_REMARAKBLE}
root@remarkable# systemctl disable --now rm-mdns
root@remarkable# rm -f /etc/systemd/system/rm-mdns /root/rm-mdns /root/rm-mdns.service
```

## Shortcomings

The mDNS responder is not fully standard compliant:

- It does not check if the claimed name is already in used.
- It does not handle the unicast flag in the mDNS query.
- Probably other non-compliant behaviour.

Apart from this, the responder will only answer for `A` record requests, so it will only work with IPv4.
Adding IPv6 support should be straight-forward, though.

## Credits

This project uses the [public domain `mdns` library][mdns] by Mattias Jansson and contributors.

[mdns]: https://github.com/mjansson/mdns
