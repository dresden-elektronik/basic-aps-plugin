Basic APS Plugin
================

This example plugin demonstrates how to use the deCONZ C++ API to send and receive ZigBee APS frames.

The plugin runs a simple state machine which continuously searches the network for ZigBee Light Link devices (profile-id 0xC05E) wich provide a on/off cluster (cluster-id 0x0006). This is done by broadcasting a ZigBee ZDP match descriptor request.

If any responses are received they will be printed in the console.

## Building

Currently the compilation of the plugin is only supported within the Raspbian distribution.


##### Install deCONZ and development package
1. Download deCONZ package

  `wget http://www.dresden-elektronik.de/rpi/deconz/deconz-latest.deb`

2. Install deCONZ package

  `sudo dpkg -i deconz-latest.deb`
  
3. Download deCONZ development package

  `wget http://www.dresden-elektronik.de/rpi/deconz-dev/deconz-dev-latest.deb`

4. Install deCONZ development package

  `sudo dpkg -i deconz-dev-latest.deb`

5. Install Qt4 development packages

  `sudo apt-get install qt4-qmake libqt4-dev`

##### Get and compile the plugin
1. Checkout the repository

  `git clone https://github.com/dresden-elektronik/basic-aps-plugin.git`
 
2. Compile the plugin

  `cd basic-aps-plugin`

  `qmake-qt4 && make`

3. Install the plugin

  `sudo cp libbasic_aps_plugin.so /usr/share/deCONZ/plugins`

## Running

Start deCONZ from commandline with enabled debug output.

  `deCONZ --dbg-info=1`

### Console Output
    send match descriptor request (id: 181)
    ....
    received match descriptor response (id: 181) from 0x00212effff0059c5
             match descriptor endpoint: 0x0A

## License
The plugin is available as open source and licensed under the BSD (3-Clause) license.

