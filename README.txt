CRTS: Cognitive Radio Test System

The Cognitive Radio Test System (CRTS) is software designed to enable testing, evaluation, and development of cognitive engines (CEs) for cognitive radios (CRs). Users of CRTS are able to design and test a variety of CEs through custom combinations of the built-in CRTS cognitive abilities. Similarly, these CEs can be tested in a variety of signal environments by combining several of the built-in signal environment emulation capabilities. Through the parameterization of the cognitive abilities and of the signal environments, and through the ability to test each CE against each signal environment in succession, CRTS enables effective testing and development of CEs.

CRTS comes with the ability to test CEs with both a) simulated signal environments and b)  USRPs for live radio transmissions in emulated signal environments.  

Building CRTS:
    1. Install the necessary dependencies
            Required: libconfig>=1.4, liquid-dsp, uhd>=3.5, liquid-usrp (fmromano version)
            Recommended: linux>=3.10
    2. Build with
            $ make

Running CRTS:
    CRTS must be run from the root of the source tree. 
    To run CRTS in simulation mode, use:
            $./crts -c
       The crts binary will simulate both a transmitter and a receiver, perform the cognitive functions, and record experiment data.

    To run CRTS using USRPs, use both
            $./crts -rc
        for the controller and
            $./crts -r
        for the receiver node.
        They can be run on separate machines, but they must be networked. Check the command line options for specifying IP addresses and ports. 
        When using CRTS with USRPs, the crts contorller connects to the transmitter USRP, performs the cognitive functions, and records experiment data. The crts receiver, on the other hand, connects to the receiver USRP and sends feedback to the transmitter over a TCP/IP connection. 

    For available command line options, use:
            $ ./crts -h

Creating and Testing Cognitive Engines
    Cognitive Engines are represented by configuration files located in the ceconfigs/ directory of the source tree. All CE config files should be placed there. An example CE config file, called 'ce1.txt', is provided. Read it to learn about the currently supported options for CEs.
    To test one or more CEs, they should be listed in the CE master file, 'master_cogengine_file.txt'. CRTS will run each listed CE through each test scenario. More details can be found in the CE master file.

Creating and Using Scenarios/Signal Environments
    Testing Scenarios are represented by configuration files located in the scconfigs/ directory of the source tree. All scenario config files should be placed there. Several example scenario config files are provided with CRTS. 
    To use a scenario file in a test, it should be listed in scenario master file, 'master_scenario_file.txt', which uses the same format as the CE master file. 

Data logging
    All data files are logged in the data/ directory. The file names have the date and time of the start of the test appended to them.
