/** 
 * Copyright 2015 abradbury
 * 
 * boidCPUTestBench.cpp
 * 
 * This file acts as a test bench for the BoidCPU FPGA core. A BoidCPU is a 
 * processing unit responsible for an area of the simulation space. Any boids 
 * that are within a BoidCPU's bounds are managed and controlled by that 
 * BoidCPU. As boids move around the simaulation area they move between BoidCPU 
 * areas and so are transferred between BoidCPUs. 
 * 
 * The test bench is able to simulate messages targeted at the BoidCPU and can 
 * act as any other simulation component. 
 * 
 * This FPGA core was developed using the 2013.4 version Xilinx’s Vivado High 
 * Level Synthesis (HLS) Design Suite.
 *
 * TODO: Towards the end of the project the test bench was used less and may 
 *          have become disjont from the FPGA cores
 *
 ******************************************************************************/

/******************************* Include Files ********************************/

#include "boidCPU.h"

/**************************** Variable Definitions ****************************/

uint32 tbOutputData[20000][MAX_CMD_LEN];
uint32 tbInputData[MAX_INPUT_CMDS][MAX_CMD_LEN];
uint32 tbOutputCount = 0;
uint32 tbInputCount = 0;

uint32 tbData[MAX_CMD_BODY_LEN];
uint32 tbTo;
uint32 tbFrom = CONTROLLER_ID;
uint32 tbDataLength = 0;

uint32 tbCoords[EDGE_COUNT];
uint32 tbNeighbours[MAX_BOIDCPU_NEIGHBOURS];

bool drawBoids = false;

/**************************** Function Prototypes *****************************/

void testSimulationSetup();
void testNeighbourSearch();
void testCalcNextBoidPos();
void testLoadBalance();
void testMoveBoids();
void testDrawBoids();

void simulateNeighbourResponse();
void simulateLoadBalanceInstructions();
void simulateBoidTransfer();

void processNeighbourReply();
void processDrawInfo();

void tbPrintCommand(bool send, uint32 *data);
void tbCreateCommand(uint32 len, uint32 to, uint32 from, uint32 type, uint32 *data);

/******************************************************************************/
/*
 * This is the main method of the test bench and connects to the BoidCPU FPGA 
 * core that is under test. 
 * 
 * Firstly, the messages/commands to be sent to the BoidCPU are constructed and 
 * stored in an output array. Then these messages are written/sent to the 
 * BoidCPU. When the test bench has finished sending the messages the BoidCPU 
 * core is called and it deals with the sent messages. The test bench then 
 * waits for replies and deals with them. 
 * 
 * Note that it is not possible to then send more data to the BoidCPU and for 
 * the BoidCPU to retain its state from the previous messages. 
 *
 * @param   None
 *
 * @return          0 if success, 1 if failure
 *
 ******************************************************************************/
int main() {
    hls::stream<uint32> to_hw, from_hw;

    // Test BoidCPU input ------------------------------------------------------
    // First send the initialisation commands
    testSimulationSetup();

    // Then repeat these commands every time step
    // Comment in/out commands as appropriate
    for (int i = 0; i < 2000; i++ ) {
        testNeighbourSearch();
        simulateNeighbourResponse();
        testCalcNextBoidPos();
        // testLoadBalance();
        // simulateLoadBalanceInstructions();
        testMoveBoids();
        // simulateBoidTransfer();
        testDrawBoids();

        std::cout << "-------------------------------------------" << std::endl;
    }

    // Send data ---------------------------------------------------------------
    outerOutputLoop: for (int i = 0; i < tbOutputCount; i++) {
        tbPrintCommand(true, tbOutputData[i]);
        innerOutputLoop: for (int j = 0; j < tbOutputData[i][CMD_LEN]; j++) {
            to_hw.write(tbOutputData[i][j]);
        }
    }

    // TODO: Come up with a better buffer counter update method
    tbOutputCount = 0;

    std::cout << "======TestBench finished sending======" << std::endl;

    // Run the hardware --------------------------------------------------------
    toplevel(to_hw, from_hw);

    // Receive data ------------------------------------------------------------
    bool inputAvailable = from_hw.read_nb(tbInputData[tbInputCount][CMD_LEN]);

    while (inputAvailable) {
        inLp: for (int i = 0; i < tbInputData[tbInputCount][CMD_LEN] -1 ; i++) {
            tbInputData[tbInputCount][i+1] = from_hw.read();
        }

        tbPrintCommand(false, tbInputData[tbInputCount]);

        // Process data --------------------------------------------------------
        switch (tbInputData[tbInputCount][CMD_TYPE]) {
            case CMD_NBR_REPLY:
                processNeighbourReply();
                break;
            case CMD_DRAW_INFO:
                processDrawInfo();
                break;
            default:
                break;
        }

        // Check for more input ------------------------------------------------
        // Don't really need input buffer if data is processed before the next
        // inputCount++;
        inputAvailable = from_hw.read_nb(tbInputData[tbInputCount][CMD_LEN]);
    }

    std::cout << "=====TestBench finished receiving=====" << std::endl;

    return 0;   // A non-zero return value signals an error
}

/******************************************************************************/
/*
 * Generate a simulation setup command as if sent from the BoidMaster. The 
 * current setup targets a BoidCPU with ID 3 with a single neighbour with ID 4.
 *
 * TODO: Need to send to broadcast during actual testing as random ID unknown
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testSimulationSetup() {
    tbDataLength = 17;

    uint32 newID;
    uint32 initialBoidCount = 20;
    uint32 distinctNeighbourCount = 1;

    // BoidCPU #3
    newID = 3;

    tbCoords[0] = 0;
    tbCoords[1] = 0;
    tbCoords[2] = 640;
    tbCoords[3] = 360;

    tbNeighbours[0] = 4;
    tbNeighbours[1] = 3;
    tbNeighbours[2] = 4;
    tbNeighbours[3] = 4;
    tbNeighbours[4] = 4;
    tbNeighbours[5] = 3;
    tbNeighbours[6] = 4;
    tbNeighbours[7] = 4;

    tbData[CMD_SETUP_NEWID_IDX] = newID;
    tbData[CMD_SETUP_BDCNT_IDX] = initialBoidCount;

    for (int i = 0; i < EDGE_COUNT; i++) {
        tbData[CMD_SETUP_COORD_IDX + i] = tbCoords[i];
    }

    tbData[CMD_SETUP_NBCNT_IDX] = distinctNeighbourCount;

    for (int i = 0; i < MAX_BOIDCPU_NEIGHBOURS; i++) {
        tbData[CMD_SETUP_BNBRS_IDX + i] = tbNeighbours[i];
    }

    tbData[CMD_SETUP_SIMWH_IDX + 0] = 1280;
    tbData[CMD_SETUP_SIMWH_IDX + 1] = 720;

    tbCreateCommand(tbDataLength, CMD_BROADCAST, tbFrom, CMD_SIM_SETUP, tbData);
}

/******************************************************************************/
/*
 * Issue a neighbour search command as if sent from the BoidMaster. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testNeighbourSearch() {
    // 4 0 1 6 ||
    tbDataLength = 0;
    tbTo = CMD_BROADCAST;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, MODE_CALC_NBRS, tbData);
}

/******************************************************************************/
/*
 * Simulate the responses of neighbouring BoidCPUs in the neighbour search 
 * phase of the simulation to the BoidCPU under test. 
 * 
 * Initially was able to programatically generate neighour responses, but this 
 * was disabled as it seemed to be being called before all other test bench 
 * methods. Instead, several hard-coded messages are constructed and sent.
 * 
 * Utilises the partial data flag which signifies the number of remaining 
 * packets that the data/message is split across. 
 * 
 * TODO: Attempt to get the programatic method working again
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void simulateNeighbourResponse() {
    // For each neighbouring BoidCPU, create a list of boids and send this to
    // the BoidCPU under test
    // FIXME: This seems to be called before all other testbench stuff
//  const int boidsPerBoidCPU = 10;
//  const int positionBounds[4] = {0, 0, 720, 720};
//  Boid boidsFromNbrs[MAX_BOIDCPU_NEIGHBOURS][boidsPerBoidCPU];
//
//  int neighboursToSimulate = 1;
//  for (int i = 0; i < neighboursToSimulate; i++) {
//      for (int j = 0; j < boidsPerBoidCPU; j++) {
//          // FIXME: Simulator generates an error unless the vel and pos
//          //  variables are incorrectly swapped round
//          Vector vel = Vector(-MAX_VELOCITY + (rand() % (int)(MAX_VELOCITY -\
//              -MAX_VELOCITY + 1)), -MAX_VELOCITY + (rand() % (int)\
//              (MAX_VELOCITY - -MAX_VELOCITY + 1)));
//
//          Vector pos = Vector(positionBounds[0] + (rand() % (int)\
//              (positionBounds[2] - positionBounds[0] + 1)),\
//              positionBounds[1] + (rand() % (int)(positionBounds[3] -\
//              positionBounds[1] + 1)));
//
//          int boidID = ((neighbours[i] - 1) * boidsPerBoidCPU) + j + 1;
//
//          boidsFromNbrs[i][j] = Boid(boidID, pos, vel);
//      }
//  }
//
//  // Now send the data to the BoidCPU under test
//  for (int i = 0; i < neighboursToSimulate; i++) {
//      // The next step is to create the message data
//      for (int j = 0, k = 0; j < boidsPerBoidCPU; j++, k++) {
//          uint32 position = 0;
//          uint32 velocity = 0;
//
//          // Encode position
//          position |= ((uint32)(((int32_fp)(boidsFromNbrs[i][j].position.x)) << 4) << 16);
//
//          if (boidsFromNbrs[i][j].position.y < 0) {
//              position |= ((~(((uint32)0xFFFF) << 16)) &
//                      ((uint32)((int32_fp)(boidsFromNbrs[i][j].position.y) << 4)));
//          }
//          else position |= ((uint32)((int32_fp)(boidsFromNbrs[i][j].position.y) << 4));
//
//          // Encode velocity
//          velocity |= ((uint32)(((int32_fp)(boidsFromNbrs[i][j].velocity.x)) << 4) << 16);
//
//          if (boidsFromNbrs[i][j].velocity.y < 0) {
//              velocity |= ((~(((uint32)0xFFFF) << 16)) &
//                      ((uint32)((int32_fp)(boidsFromNbrs[i][j].velocity.y) << 4)));
//          }
//          else velocity |= ((uint32)((int32_fp)(boidsFromNbrs[i][j].velocity.y) << 4));
//
//          data[(k * BOID_DATA_LENGTH) + 0] = position;
//          data[(k * BOID_DATA_LENGTH) + 1] = velocity;
//          data[(k * BOID_DATA_LENGTH) + 2] = boidsFromNbrs[i][j].id;
//      }
//
//      int dataLength = boidsPerBoidCPU * BOID_DATA_LENGTH;
//      createCommand(dataLength, 7, neighbours[i], CMD_NBR_REPLY, data);
//  }

    // 32 99 3 8 || 1 12583088 5242880 21 20185648 4289724444 22 12583408
    //   4290838496 23 36700512 65488 24 4194448 4293918720 25 19923232 2162640
    //   26 39846192 4259776 27 18874448 4293918752 28 15729168 2162656 29
    // 8  99 3 8 || 0 3145856 4292870144 30

    tbData[0] = 1;          // Signals that the data continues in 1 more packet

    tbData[1] = 12583088;   // Boid position (bit-shifted value)
    tbData[2] = 5242960;    // Boid velocity (bit-shifted value) 
    tbData[3] = 21 + 10;    // Boid ID

    tbData[4] = 20185648;
    tbData[5] = 5242960;
    tbData[6] = 22 + 10;

    tbData[7] = 12583408;
    tbData[8] = 5242960;
    tbData[9] = 23 + 10;

    tbData[10] = 36700512;
    tbData[11] = 5242960;
    tbData[12] = 24 + 10;

    tbData[13] = 4194448;
    tbData[14] = 5242960;
    tbData[15] = 25 + 10;

    tbData[16] = 19923232;
    tbData[17] = 5242960;
    tbData[18] = 26 + 10;

    tbData[19] = 39846192;
    tbData[20] = 5242960;
    tbData[21] = 27 + 10;

    tbData[22] = 18874448;
    tbData[23] = 5242960;
    tbData[24] = 28 + 10;

    tbData[25] = 15729168;
    tbData[26] = 5242960;
    tbData[27] = 29 + 10;

    int number = 0 + rand() / (RAND_MAX / (9 - 0) + 1);
    tbData[0] = 1;

    tbCreateCommand(1 + (number * 3), 99, 4, CMD_NBR_REPLY, tbData);

    tbData[0] = 0;          // Signals that there are no more packets
    tbData[1] = 3145856;
    tbData[2] = 5242960;
    tbData[3] = 30 + 10;

    tbCreateCommand(1, 99, 4, CMD_NBR_REPLY, tbData);
}

/******************************************************************************/
/*
 * Issue command signalling the start of the boid update phase of the 
 * simulation as if sent from the BoidMaster. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testCalcNextBoidPos() {
    // 4 0 1 9 ||
    tbDataLength = 0;
    tbTo = CMD_BROADCAST;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, MODE_POS_BOIDS, tbData);
}

/******************************************************************************/
/*
 * Issue command signalling the start of the load balancing phase of the 
 * simulation as if sent from the BoidMaster. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testLoadBalance() {
    // 4 0 1 10 ||
    tbDataLength = 0;
    tbTo = CMD_BROADCAST;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, MODE_LOAD_BAL, tbData);
}

/******************************************************************************/
/*
 * Simulate a load balancing command from the BoidMaster instructing BoidCPU #3 
 * to reduce all its free edges by 1 step size. Note that the value sent in the 
 * message body is a bit-shifted value representing this change. For more 
 * information, see the processLoadData() method in boidMaster.cpp.
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void simulateLoadBalanceInstructions() {
    // 5 3 1 20 || 8177
    tbDataLength = 1;
    tbData[0] = 8177;   // Shrink all 4 edges
    tbTo = 3;
    tbFrom = CONTROLLER_ID;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, CMD_LOAD_BAL, tbData);
}

/******************************************************************************/
/*
 * Issue command signalling the start of the boid transfer phase of the 
 * simulation as if sent from the BoidMaster. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testMoveBoids() {
    // 4 0 1 11 ||
    tbDataLength = 0;
    tbTo = CMD_BROADCAST;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, MODE_TRAN_BOIDS, tbData);
}

/******************************************************************************/
/*
 * Simulate the transfer of boids to the BoidCPU under test. The current setup 
 * sends 2 boids to the BoidCPU:
 * 
 *  Boid #42: position (48, 20), velocity (-1, -3)
 *  Boid #43: position (43, 53), velocity (0, 4)
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void simulateBoidTransfer() {
    tbDataLength = 5;
    tbTo = CMD_BROADCAST;

    tbData[0] = 42;
    tbData[1] = 48;
    tbData[2] = 20;
    tbData[3] = -1;
    tbData[4] = -3;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, CMD_BOID, tbData);

    tbData[0] = 43;
    tbData[1] = 53;
    tbData[2] = 21;
    tbData[3] = 0;
    tbData[4] = 4;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, CMD_BOID, tbData);
}

/******************************************************************************/
/*
 * Issue command signalling the BoidCPUs for transfer information about their 
 * boids to the BoidGPU. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void testDrawBoids() {
    // 4 0 1 14 ||
    tbDataLength = 0;
    tbTo = CMD_BROADCAST;
    tbCreateCommand(tbDataLength, tbTo, tbFrom, MODE_DRAW, tbData);
}

/******************************************************************************/
/*
 * Parse neighbouring boid information sent by the BoidCPU under test. Prints 
 * an error if the boids have velocity greater than the maximum. 
 * 
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void processNeighbourReply() {
    int count = (tbInputData[tbInputCount][CMD_LEN] - CMD_HEADER_LEN - 1) /
        BOID_DATA_LENGTH;
    Boid tbBoids[MAX_BOIDS];

    std::cout << "Dummy BoidCPU received " << count << " boids" << std::endl;

    for (int i = 0; i < count; i++) {
        uint32 position = tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 1];
        uint32 velocity = tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 2];

        // Decode position and velocity
        Vector p = Vector(((int32_fp)((int32)position >> 16)) >> 4,
                ((int32_fp)((int16)position)) >> 4);

        Vector v = Vector(((int32_fp)((int32)velocity >> 16)) >> 4,
                ((int32_fp)((int16)velocity)) >> 4);

        if ((v.x > MAX_VELOCITY) || (v.x < -MAX_VELOCITY)) {
            std::cerr << "ERROR: x vel" << std::endl;
        }

        if ((v.y > MAX_VELOCITY) || (v.y < -MAX_VELOCITY)) {
            std::cerr << "ERROR: y vel" << std::endl;
        }

        Boid b = Boid((uint16)tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 3], p, v);
        tbBoids[i] = b;
//      b.printBoidInfo();
    }
}

/******************************************************************************/
/*
 * Intercepts messages being sent from a BoidCPU to the BoidGPU and either 
 * prints them out in an ASCII-style drawing (only suitable for BoidCPUs of 
 * small size, < 40pixels) or textually prints out each boids attributes.
 * 
 * TODO: Check that the ASCII-style drawing works with final message structure
 * TODO: Fix textual print out - bitshifting wrong
 *
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void processDrawInfo() {
    std::cout << "Drawing boids..." << std::endl;

    if (drawBoids == true) {
        int maxBoidID = 0;
        int digits = 0;
        int widthDigits = 0;
        int idDigits = 0;
        int boidAtPos = 0;
        char space = '-';
        char edge = '*';

        // Get the boid bounds
        int boidCPUWidth =  tbCoords[X_MAX] - tbCoords[X_MIN];
        int boidCPUHeight = tbCoords[Y_MAX] - tbCoords[Y_MIN];

        // Get the number of boids
        int tbBoidCount = (tbInputData[tbInputCount][CMD_LEN] - CMD_HEADER_LEN) / 3;

        // Get the maximum boid ID
        for (int i = 0; i < tbBoidCount; i++) {
            if (tbInputData[tbInputCount][CMD_HEADER_LEN + (i * 3)] > maxBoidID) {
                maxBoidID = tbInputData[tbInputCount][CMD_HEADER_LEN + (i * 3)];
            }
        }

        // Determine the number of digits in the max boid ID
        if (maxBoidID < 10) {
            idDigits = 1;
        } else if (maxBoidID < 100) {
            idDigits = 2;
        } else {
            idDigits = 3;
        }

        // Calculate the top edge offset due to the left edge and index
        if (boidCPUHeight < 10) {
            widthDigits = 1;
        } else if (boidCPUHeight < 100) {
            widthDigits = 2;
        } else {
            widthDigits = 3;
        }
        std::cout << std::string(widthDigits, ' ') << edge;

        // Print the top index row
        for (int i = 0; i < boidCPUWidth; i++) {
            if (i < 10) {
                digits = widthDigits;
            } else if (i < 100) {
                digits = widthDigits - 1;
            } else {
                digits = widthDigits - 2;
            }
            std::cout << i << std::string(digits, ' ');
        } std::cout << std::endl;

        // Print the top edge (including offset)
        std::cout << std::string(widthDigits, ' ') << edge;
        for (int i = 0; i < boidCPUWidth; i++) {
            std::cout << edge << std::string(idDigits, ' ');
        } std::cout << std::endl;

        // Print the positions of the boids in the BoidCPU
        for (int y = 0; y < boidCPUHeight; y++) {
            // Print the left edge and index
            if (y < 10) {
                digits = widthDigits - 1;
            } else if (y < 100) {
                digits = widthDigits - 2;
            } else {
                digits = widthDigits - 3;
            }
            std::cout << std::string(digits, ' ') << y << edge;

            // Print the boid ID at the appropriate position
            for (int x = 0; x < boidCPUWidth; x++) {
                for (int i = 0, j = 0; j < tbBoidCount; j++, i = i + 3) {
                    int boidID = tbInputData[tbInputCount][CMD_HEADER_LEN + i];
                    int boidX =  tbInputData[tbInputCount][CMD_HEADER_LEN + i + 1];
                    int boidY =  tbInputData[tbInputCount][CMD_HEADER_LEN + i + 2];

                    if (boidX == x) {
                        if (boidY == y) {
                            boidAtPos = tbInputData[tbInputCount][CMD_HEADER_LEN + i];
                            break;
                        }
                    }
                }

                if (boidAtPos != 0) {
                    if (boidAtPos < 10) {
                        digits = idDigits;
                    } else if (boidAtPos < 100) {
                        digits = idDigits - 1;
                    } else {
                        digits = idDigits - 2;
                    }
                    std::cout << boidAtPos << std::string(digits, space);

                    boidAtPos = 0;
                } else {
                    std::cout << std::string(idDigits + 1, space);
                }
            }

            // Print the right edge
            std::cout << edge << y << std::endl;
        }

        // Print the bottom edge (including offset)
        std::cout << std::string(widthDigits, ' ') << edge;
        for (int i = 0; i < boidCPUWidth; i++) {
            std::cout << edge << std::string(idDigits, ' ');
        } std::cout << std::endl;

        // Print the bottom index row
        std::cout << std::string(widthDigits, ' ') << edge;
        for (int i = 0; i < boidCPUWidth; i++) {
            if (i < 10) {
                digits = widthDigits;
            } else if (i < 100) {
                digits = widthDigits - 1;
            } else {
                digits = widthDigits - 2;
            }
            std::cout << i << std::string(digits, ' ');
        } std::cout << std::endl;
    } else {
        // FIXME: Wrong print out
//      int tbBoidCount = (tbInputData[tbInputCount][CMD_LEN] - CMD_HEADER_LEN) / BOID_DATA_LENGTH;
//
//      for (int i = 0; i < tbBoidCount; i++) {
//          uint32 position = tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 0];
//          uint32 velocity = tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 1];
//
//          Vector p = Vector((int12)((position & (~(uint32)0xFFFFF)) >> 20),
//                  (int12)((position & (uint32)0xFFF00) >> 8));
//
//          Vector v = Vector((int12)((velocity & (~(uint32)0xFFFFF)) >> 20),
//                  (int12)((velocity & (uint32)0xFFF00) >> 8));
//
//          uint16 boidID = (uint16)tbInputData[tbInputCount][CMD_HEADER_LEN + (BOID_DATA_LENGTH * i) + 2];
//
//          std::cout << "Boid " << boidID << " has position [" << p.x
//              << ", " << v.y << "]" << std::endl;
//        }
    }
}

/******************************************************************************/
/*
 * Takes data to be transmitted and places it in an queue of data. This queue 
 * is processed, i.e. the elements sent, in the top level method when all the 
 * messages to be sent to the BoidCPU under test have been created. 
 * 
 * TODO: Add a check to not add the data if the output buffer is full.
 *
 * @param   len     The length of the message body
 * @param   to      The ID of the recipient of the message
 * @param   from    The ID of the message sender
 * @param   type    The type of the message (defined in boidCPU.h)
 * @param   data    The message data
 *
 * @return  None
 *
 ******************************************************************************/
void tbCreateCommand(uint32 len, uint32 to, uint32 from, uint32 type, uint32 *data) {
    tbOutputData[tbOutputCount][CMD_LEN]  = len + CMD_HEADER_LEN;
    tbOutputData[tbOutputCount][CMD_TO]   = to;
    tbOutputData[tbOutputCount][CMD_FROM] = from;
    tbOutputData[tbOutputCount][CMD_TYPE] = type;

    if (len > 0) {
        dataToCmd: for (int i = 0; i < len; i++) {
            tbOutputData[tbOutputCount][CMD_HEADER_LEN + i] = data[i];
        }
    }

    tbOutputCount++;
}

/******************************************************************************/
/*
 * Parses a message and prints it out to the standard output.
 *
 * @param   send    True if the message is being sent, false otherwise
 * @param   data    The array containing the message
 *
 * @return  None
 *
 ******************************************************************************/
void tbPrintCommand(bool send, uint32 *data) {
    if (send) {
        if (data[CMD_TO] == CMD_BROADCAST) {
            std::cout << "-> TX, TestBench sent broadcast:                   ";
        } else if (data[CMD_TO] == BOIDGPU_ID) {
            std::cout << "-> TX, TestBench sent command to BoidGPU:          ";
        } else if (data[CMD_TO] == CONTROLLER_ID) {
            std::cout << "-> TX, TestBench sent command to BoidMaster:       ";
        } else {
            std::cout << "-> TX, TestBench sent command to " << data[CMD_TO]
                    << ":               ";
        }
    } else {
        if (data[CMD_FROM] == BOIDGPU_ID) {
            // This should never happen - BoidGPU should just receive
            std::cout << "<- RX, TestBench received command from BoidGPU:    ";
        } else if (data[CMD_FROM] == CONTROLLER_ID) {
            std::cout << "<- RX, TestBench received command from BoidMaster: ";
        } else {
            std::cout << "<- RX, TestBench received command from "
                    << data[CMD_FROM] << ":         ";
        }
    }

    switch (data[CMD_TYPE]) {
    case MODE_INIT:
        std::cout << "initialise self                   ";
        break;
    case CMD_PING:
        std::cout << "BoidCPU ping                      ";
        break;
    case CMD_PING_REPLY:
        std::cout << "BoidCPU ping response             ";
        break;
    case CMD_USER_INFO:
        std::cout << "user info                         ";
        break;
    case CMD_SIM_SETUP:
        std::cout << "setup BoidCPU                     ";
        break;
    case MODE_CALC_NBRS:
        std::cout << "calculate neighbours              ";
        break;
    case CMD_NBR_REPLY:
        std::cout << "neighbouring boids from neighbour ";
        break;
    case MODE_POS_BOIDS:
        std::cout << "calculate new boid positions      ";
        break;
    case MODE_LOAD_BAL:
        std::cout << "load balance mode                 ";
        break;
    case CMD_LOAD_BAL:
        std::cout << "load balance instructions         ";
        break;
    case CMD_LOAD_BAL_REQUEST:
        std::cout << "load balance request              ";
        break;
    case MODE_TRAN_BOIDS:
        std::cout << "transfer boids                    ";
        break;
    case CMD_BOID:
        std::cout << "boid in transit                   ";
        break;
    case MODE_DRAW:
        std::cout << "send boids to BoidGPU             ";
        break;
    case CMD_DRAW_INFO:
        std::cout << "boid info heading to BoidGPU      ";
        break;
    case CMD_ACK:
        std::cout << "ACK signal                        ";
        break;
    case CMD_PING_END:
        std::cout << "end of ping                       ";
        break;
    case CMD_KILL:
        std::cout << "kill simulation                   ";
        break;
    default:
        std::cout << "UNKNOWN COMMAND: (" << data[CMD_TYPE] << ")             ";
        break;
    }

    int i = 0;
    for (i = 0; i < CMD_HEADER_LEN; i++) {
        std::cout << data[i] << " ";
    }
    std::cout << "|| ";

    for (i = 0; i < data[CMD_LEN] - CMD_HEADER_LEN; i++) {
        std::cout << data[CMD_HEADER_LEN + i] << " ";
    }
    std::cout << std::endl;
}
