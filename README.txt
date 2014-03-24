Build with
$ make

To remove previous build, use
$ make clean

CRTS must be run from the main crts directory. 
To run:
$ ./crts
and optionally, if using USRP's,
$ ./crts_rx

For available command line options use:
$ ./crts -h
or 
$ ./crts_rx -h

All scenario config files should be placed in the scconfigs/ folder.
All cognitive engine config files should be place in the ceconfigs/ folder.

Data files are logged in the data/ directory with the date and time appended.

Dependencies include:
libconfig>=1.4, liquid-dsp, liquid-usrp, uhd=3.5

Recommended:
linux>=3.10

