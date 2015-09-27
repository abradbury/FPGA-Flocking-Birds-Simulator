FPGA Implementation
===================

The FPGA component of this project presents a partially scalable multi-FPGA implementation of the *boids* model of flocking birds presented by [Craig Reynolds](http://www.red3d.com/cwr/boids/). The implementation was able to simulate realistic flocking behaviour for up to 30 boids on a single FPGA and reduced flocking behaviour for a limited period of time for 20 boids across 2 FPGAs. The system is designed in such a way that it would work using heterogeneous (different) FPGAs.  

Main contributions: 
* A flexible simulation initialisation procedure whereby new processing components are detected using a ping-based system and incorporated into the simulation
* A method to reduce the communications load during neighbour search by utilising multicast messages and bit-shifting

![GIF showing 30 flocking on 1 BoidCPU on 1 FPGA](../doc/img/flocking.gif)

Tools Used
----------
The BoidCPU and BoidMaster FPGA cores were developed using the 2013.4 version Xilinx’s Vivado High Level Synthesis (HLS) Design Suite. These FPGA cores were then placed and routed using Xilinx Platform Studio (XPS) 14.7, part of the Xilinx Embedded Development Kit (EDK). Vendor-supplied cores included an Ethernet controller and MicroBlaze softcore processor. Xilnix Software Development Kit (SDK) 14.7 took the bitstream generated by XPS and programmed the Gatekeeper component on to the MicroBlaze. The final bitstream was placed on the FPGAs using Xilinx iMPACT 14.7. 

![Diagram showing the tools used during design development](../doc/img/tools.png)
