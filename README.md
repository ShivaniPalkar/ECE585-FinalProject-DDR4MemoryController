## Project Name:  Memory Controller for DDR4 

****************************************************************************************************************************************************************************************************************************************************************************
## Description:

- The project implements a C++ code for the simulation of a memory controller for a DDR4 wherein the memory channel contains a single-ranked 8GB PC4-25600 DIMM. 
- The code implements an open page opolicy and uses the in-order scheduling of incoming memory requests.
- The project accepts 16 requests at a time and stores them in a queue according to their time of arrival. It follows the CPU clock cycles i.e (DRAM cycles*2) for servicing the requests in the First Come First Serve order.
- It is a user friendly environment where the user can choose debug mode for inout trace files and also chose the trace file to be run.

*****************************************************************************************************************************************************************************************************************************************************************************

## Input Format

- Inputs to the code are CPU requests which arrive at their defined times from an inpout trace file .
- The input includes the time oF arrival of the request (CPU cycles), one out of the three operations to be perfeormed (READ,WRITE,PREFETCH), and a 33 bit address to which the request is made.
- Example:    45    0     0x0000FFFF2 
- Here, we specify 0 for READ, 1 for WRITE and 2 FOR PREFETCH.
- Timing constraints and appropriate commands (ACT, PRE, READ, WRITE)  will be issued to the request accordingly.

*******************************************************************************************************************************************************************************************************************************************************************************

## Output Format

- The code generates the final output in another trace file.

- Example :  0    ACT    2        1         1
            48    RD     2        1         3fb
                    
- The output shows the time at which the respective command is being issued alongwith the bank group number, the bank number and their respective rows and coulmns being accessed at the specified time.

./ECE585_Group_15_Final_Project
    ./src
    ./testcases_self
    ./testcases_prof
    ./ECE585_FINAL_PROJECT_REPORT
    ./README.md