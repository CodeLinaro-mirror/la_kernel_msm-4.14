Date: 3/5/2020
Author: Aaron J. Lee (jehun.lee@newracom.com)

Wireshark code: WFA HaLow PF Sniffer

Platform: Raspberry Pi 3 Model B+
OS: Raspbian Stretch with Pixel (Linux Kernel Version: 4.14.70)

1. Unzip NewraPeek_V0-1.5.tar.gz
tar zxf NewraPeek_v0-1.5.tar.gz

2. (Optional) Install libraries
sudo apt-get update
sudo apt-get install cmake libgcrypt-dev flex bison build-essential qtbase5-dev apt-file qt5-default qttools5-dev qttools5-dev-tools qtmultimedia5-dev libqt5svg5-dev
sudo apt-get install autogen autoconf libtool-bin libglib2.0-dev libgtk2.0-dev libpcap-dev
sudo apt-get install lua5.1 liblua5.1-0-dev

3. Install FW
sudo cp uni_s1g.bin /lib/firmware

4. Install NewraPeek
sudo dpkg -i *.deb

5.1 Local start (via VNC viewer)
cd NewraPeek_v0-1.5/
sed -i 's/gksudo .\/wireshark/sudo .\/wireshark/g' ./run_wts.py
./run_wts.py US 161

5.2 Remote start (via MobaXterm SSH session)
cd NewraPeek_v0-1.5/
sed -i 's/sudo .\/wireshark/gksudo .\/wireshark/g' ./run_wts.py
./run_wts.py US 161
choose wlan0 and then Capture->Start (Ctrl-E)

