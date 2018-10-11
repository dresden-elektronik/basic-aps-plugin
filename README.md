Introduction
============

The [deCONZ](http://www.dresden-elektronik.de/funktechnik/products/software/pc/deconz?L=1) Basic APS Plugin demonstrates how to use the deCONZ C++ API to send and receive ZigBee APS frames.

The plugin runs a simple state machine which continuously searches the network for ZigBee Light Link devices (profile-id 0xC05E) wich provide a on/off cluster (cluster-id 0x0006). This is done by broadcasting a ZigBee ZDP match descriptor request.

If any responses are received they will be printed in the console.

Installation
============

##### Supported platforms
* Raspbian Jessie
* Raspbian Stretch

Raspbian Wheezy and Qt4 is no longer maintained.

### Install deCONZ

**Important** If you're updateing from a previous version always make sure to create an backup in the Phoscon App and read the changelog first.

1. Download deCONZ package

        wget http://www.dresden-elektronik.de/rpi/deconz/beta/deconz-latest-beta.deb

2. Install deCONZ package

        sudo dpkg -i deconz-latest-beta.deb

**Important** this step might print some errors *that's ok* and will be fixed in the next step.

3. Install missing dependencies

        sudo apt update
        sudo apt install -f

##### Install deCONZ development package

1. Download deCONZ development package

        wget http://www.dresden-elektronik.de/rpi/deconz-dev/deconz-dev-latest.deb

2. Install deCONZ development package

        sudo dpkg -i deconz-dev-latest.deb

3. Install missing dependencies

        sudo apt update
        sudo apt install -f

##### Get and compile the plugin
1. Checkout the repository

        git clone https://github.com/dresden-elektronik/basic-aps-plugin.git

2. Compile the plugin

        cd basic-aps-plugin
        qmake && make -j2

**Note** On Raspberry Pi 1 use `qmake && make`

3. Replace original plugin

        sudo cp ../libbasic_aps_plugin.so /usr/share/deCONZ/plugins

## Running

Start deCONZ from commandline with enabled debug output.

  `deCONZ --dbg-info=1`

### Console Output
    send match descriptor request (id: 181)
    ....
    received match descriptor response (id: 181) from 0x00212effff0059c5
             match descriptor endpoint: 0x0A

Headless support
----------------

The beta version contains a systemd script, which allows deCONZ to run without a X11 server.

**Note** The service does not yet support deCONZ updates via WebApp, therefore these must be installed manually. A further systemd script will handle updates in future versions.

1. Enable the service at boot time

```bash
$ sudo systemctl enable deconz
```

2. Disable deCONZ GUI Autostart

The dresden elektronik Raspbian sd-card image autostarts deCONZ GUI.

```bash
$ sudo systemctl disable deconz-gui
$ sudo systemctl stop deconz-gui
```

On older versions of deCONZ this can be done by removing the X11 Autostart file.

```bash
$ rm -f /home/pi/.config/autostart/deCONZ.desktop
```


Software requirements
---------------------
* Raspbian Jessie or Raspbian Stretch with Qt5

**Important** The serial port must be configured as follows to allow communication with the RaspBee.

    $ sudo raspi-config

    () Interfacting Options > Serial

        * Would you like a login shell accessible over serial?
          > No
        * Would you like the serial port hardware to be enabled?
          > Yes

After changing the settings reboot the Raspberry Pi.


Hardware requirements
---------------------

* Raspberry Pi 1, 2 or 3
* [RaspBee](http://www.dresden-elektronik.de/funktechnik/solutions/wireless-light-control/raspbee?L=1) ZigBee Shield for Raspberry Pi
* [ConBee](https://www.dresden-elektronik.de/funktechnik/solutions/wireless-light-control/conbee/?L=1) USB dongle for Raspberry Pi and PC

3rd party libraries
-------------------
The following libraries are used by the plugin:

* [SQLite](http://www.sqlite.org)
* [qt-json](https://github.com/lawand/droper/tree/master/qt-json)
* [colorspace](http://www.getreuer.info/home/colorspace)

License
=======
The plugin is available as open source and licensed under the BSD (3-Clause) license.
