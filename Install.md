# Install DAQ system

This document is the installation log for Ubuntu 18.04 PC.  For Ubuntu 16 or 20, The procedures are almost same as the following.  Main difference is package name for **"apt install"** command.  Sometimes, the apt command suggest the right package name.  And also you can google to find the right package name.  This installation is based on the information on [DAQ-Middleware](https://daqmw.kek.jp/) and [ROOT](https://root.cern.ch/) web pages.
At this moment, the system was tested on Ubuntu 18.04, Ubuntu 16.04, Ubuntu 20.04, and CentOS 8.  Ubuntu is the one of the famous distributions in general, CentOS is also very common distribution for nuclear, particle, high-energy physics people.

## Install software
### From package manager
### For Ubuntu18
On the terminal, type following.  
***sudo apt install omniorb omniidl omniorb-nameserver libomniorb4-dev libxalan-c-dev libtool-bin uuid-dev autogen libboost-all-dev bc libxml2-utils libxml2-dev xinetd emacs git cmake-qt-gui doxygen automake swig dpkg-dev g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev python libssl-dev apache2\* libapache2-mod-wsgi***  
### For Ubuntu20
On the terminal, type following.  
***sudo apt install omniorb omniidl omniorb-nameserver libomniorb4-dev libxalan-c-dev libtool-bin uuid-dev autogen libboost-all-dev bc libxml2-utils libxml2-dev xinetd emacs git cmake-qt-gui doxygen automake swig dpkg-dev g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev python libssl-dev apache2 libapache2-mod-wsgi python2-dev libcurl4-openssl-dev nlohmann-json3-dev***  
### For Ubuntu22
On the terminal, type following.  
***sudo apt install omniorb omniidl omniorb-nameserver libomniorb4-dev libxalan-c-dev libtool-bin uuid-dev autogen libboost-all-dev bc libxml2-utils libxml2-dev xinetd emacs git cmake-qt-gui doxygen automake swig dpkg-dev g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev python-is-python3 libssl-dev apache2 libapache2-mod-wsgi-py3 python3-dev libcurl4-openssl-dev nlohmann-json3-dev xinet***  

These packages are listed by [Dependencies page of ROOT](https://root.cern/install/dependencies/) and [DAQ-Middleware intallation for Raspberry Pi](https://daqmw.kek.jp/raspberrypi/DAQ-MWonRasp4b-rep.txt).  Almost all of DAQ-Middleware page is written in Japanese, please try to use Chrome or some web browsers with translation. There are some unnecessary packages also.  If you want, you can remove (e.g. emacs).  

### From source, manual instalation
Install following software
* OpenRTM  
The version 1.2.2 (The latest version at December 2020) is used.  You can find the install script for Ubuntu at [Release note](https://www.openrtm.org/openrtm/en/download/openrtm-aist-cpp/openrtm-aist-cpp_1_2_2_release).  
From source, you need to do following.  
***wget https://github.com/OpenRTM/OpenRTM-aist/releases/download/v1.2.2/OpenRTM-aist-1.2.2.tar.gz***  
***tar zxvf OpenRTM-aist-1.2.2.tar.gz***  
***cd OpenRTM-aist-1.2.2***  
***./configure***  
***make -j12*** (12 means using 12 threads to compile)  
***sudo make install***

* DAQ-Middleware  
For CentOS, you can find the source code from [GitHub](https://github.com/h-sendai/DAQ-Middleware-CentOS8).  For Ubuntu, You can download from [KEK](https://daqmw.kek.jp/src/), DAQ-Middleware-1.4.4.tar.gz is the latest version in December 2020.  We use 1.4.4.  
***wget https://daqmw.kek.jp/src/DAQ-Middleware-1.4.4.tar.gz***  
***tar zxvf DAQ-Middleware-1.4.4.tar.gz***  
***cd DAQ-Middleware-1.4.4***  
***make*** (If you compile with option -j, you will get a trouble.).  
***sudo make install***  

* ROOT  
ROOT is used to plot. You can install by [some ways](https://root.cern/install/).  Here, I describe the installation from source.  Also I use not release version.  In the case of you want the stable version, please download the source code as same as the [ROOT installation page](https://root.cern/install/build_from_source/).  Also I install ROOT at /opt/ROOT like a old style CERN software.  The version used is 6.22.06.  Usually ROOT is already installed by your team.  If installed, you can skip.  But if you will get some troubles from monitoring, back to here.  
***wget https://root.cern/download/root_v6.22.06.source.tar.gz***  
***mkdir build_root***  
***cd build_root***  
***cmake -DCMAKE_INSTALL_PREFIX=/opt/ROOT ../root-6.22.06***  
***make -j12*** (If you will get the error messages, try other stable version of ROOT.)  
***sudo mkdir -p /opt/ROOT***  
***sudo make install***  
After installation, add following line into ~/.bashrc.    
**source /opt/ROOT/bin/thisroot.sh**  (Not command.)  
***source ~/.bashrc*** (This should be on terminal, command.)  

* CAEN driveres and libraries  
The installed files are at ELIADE server 8  
**/home/eliade/DAQ/InstallFiles/CAEN**  
The following is the file list.  The versions used are in the file name.  script.sh install all drivers and libraries.  Note: A3818 driver version 1.6.4 does not suppot the kernel of Ubuntu20. A3818Drv-1.6.4.hack.tgz supports Ubuntu20.  It is quick hack version done by Soiciro.  If you will find the newer version, it is better to use.  
**A3818Drv-1.6.4.hack.tgz   
CAENComm-1.4.1-build20210305.tgz  
CAENDigitizer-2.16.3.tgz  
CAENDPPLib_1.7.2_build170707.tar  
CAENUpgrader-1.7.1-build20210402.tgz  
CAENUSBdrvB-1.5.4.tgz  
CAENVMELib-3.2.0-build20210409.tgz  
CoMPASS-1.6.0.tar.gz  
script.sh**

* CAEN digitizer handler  
We use https://github.com/aogaki/TDigiTES  
Download or clone it.  The discription will be in the later.

## Preparing the running env  
* Check the output directory, usually ELIADE team uses under /eliadedisks.  

* Edit system library path.  Usually, /usr/local/lib is not included in default.   
**/etc/ld.so.conf.d/daqmw.conf**  
Adding following lines.  If ROOT is installed another place, adding the proper place.  
**/usr/local/lib**  
**/opt/ROOT/lib**  
And type ldconfig command.  
***sudo ldconfig***  

* Adding network component controll (If you don't need the network transparency, you don't need to do.)  
Editting service setting  
***sudo vim /etc/services***  
Adding following  
**bootComps       50000/tcp                       # boot Comps**  
Adding bootComps into the system  
***sudo cp /usr/share/daqmw/etc/remote-boot/bootComps-xinetd.sample /etc/xinetd.d/bootComps***  
In the bootComps file, Change the user name.  Default is daq.  
**/etc/xinetd.d/bootComps**  
**daq -> eliade** (should be smne as user making components described bellow)  

* Set up Apache2 HTTP server  
Enable headers and CORS. This is needed for remote monitoring and controll.  
First, enable the header.  
***sudo a2enmod headers***  
Next, Editting the config file **/etc/apache2/apache2.conf**.  Adding following line in the **<Directory /var/www/>** section.  
**Header set Access-Control-Allow-Origin "*"**  

* Edit and set up daqmw web script  
In Ubuntu, the default setting of web pages are at **/var/www/html** not **/var/www**.  Place the scripts at the right place.  
***sudo cp -a /var/www/daqmw /var/www/html/***  
Edit config files of HTTP server  
**/etc/apache2/conf.d/daq.conf**  
Change the location of script files (www -> www/html) as same as folloing two lines.   
**WSGIScriptAlias /daqmw/scripts "/var/www/html/daqmw/scripts"  
WSGIPythonPath  /var/www/html/daqmw/scripts**  
Also add above 2 lines into following config files.  
**/etc/apache2/sites-available/000-default.conf  
/etc/apache2/sites-enabled/000-default.conf**  
Restart the HTTP server  
***sudo systemctl restart apache2***  

* Place the controller web page  
The page is written in TypeScript.  The source code is https://github.com/aogaki/DAQController  
The compiled version is at DAQController.  Under the DAQController, there is the assets directory.  
Editting the setting file apiSettings.json at assets.  Change the operator address as IP address of DAQ PC.  After editing, cp the file to proper place.    
***sudo cp -a DAQController /var/www/html/controller***

## Preparation
DELILA expects TDigiTes is under DELILA directory (soon we will use git module).  Under the DELILA directory you can see Components directory.  There are some components for DAQ system.  Those also expects (Make files in the directory) TDigiTes will be under the DELILA directory.  
* eliade.xml  
This is the file to start up the DAQ system.  Describing the structure of the system.  The details will be add here soon.  
* PHA.conf, PSD.conf, Slave.conf  
Those are the setting file for TDigiTes called by some components (e.g. ReaderPSD).  
* Component description  
NYI
