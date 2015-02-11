#ifndef __TOPLEVEL_H_
#define __TOPLEVEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <ap_int.h>                 // For arbitrary precision types
#include <ap_fixed.h>               // For fixed point data types
#include <hls_stream.h>
#include "hls_math.h"               // For sqrt

// Command definitions ---------------------------------------------------------
#define CMD_HEADER_LEN          4   // The length of the command header
#define MAX_CMD_BODY_LEN        30  // The max length of the command body
#define MAX_CMD_LEN             CMD_HEADER_LEN + MAX_CMD_BODY_LEN

#define MAX_OUTPUT_CMDS         8  	 // The number of output commands to buffer
#define MAX_INPUT_CMDS          1   // The number of input commands to buffer

#define CMD_LEN                 0   // The index of the command length
#define CMD_TO                  1   // The index of the command target
#define CMD_FROM                2   // The index of the command sender
#define CMD_TYPE                3   // The index of the command type

#define CMD_BROADCAST           0   // The number for a broadcast command
#define CONTROLLER_ID           1   // The ID of the controller
#define BOIDGPU_ID              2   // The ID of the BoidGPU

#define BOID_DATA_LENGTH        3   // The number of bits to send for a boid

#define MODE_INIT               1   //
#define CMD_PING                2   // Controller -> BoidCPU
#define CMD_PING_REPLY          3   // BoidCPU -> Controller
#define CMD_USER_INFO           4   // Controller -> BoidGPU
#define CMD_SIM_SETUP           5   // Controller -> Boid[CG]PU
#define MODE_CALC_NBRS          6   //
#define CMD_NBR_REPLY           8   // BoidCPU -> BoidCPU
#define MODE_POS_BOIDS          9   //
#define CMD_LOAD_BAL            10  // TODO: Decide on implementation
#define MODE_TRAN_BOIDS         11  //
#define CMD_BOID                12  // BoidCPU -> BoidCPU
#define MODE_DRAW               14  // TODO: Perhaps not needed?
#define CMD_DRAW_INFO           15  // BoidCPU -> BoidGPU
#define CMD_KILL                16  // Controller -> All

// Boid definitions ------------------------------------------------------------
#define MAX_BOIDS               30  // The maximum number of boids for a BoidCPU
#define MAX_VELOCITY            5
#define MAX_FORCE               1   // Determines how quickly a boid can turn
#define VISION_RADIUS           20  // How far a boid can see
#define VISION_RADIUS_SQUARED	400
#define MAX_NEIGHBOURING_BOIDS  45  // TODO: Decide on appropriate value?

// BoidCPU definitions ---------------------------------------------------------
#define AREA_WIDTH              720 // TODO: Should a BoidCPU know this?
#define AREA_HEIGHT             720 // TODO: Should a BoidCPU know this?
#define EDGE_COUNT              4   // The number of edges a BoidCPU has
#define MAX_BOIDCPU_NEIGHBOURS  8   // The maximum neighbours a BoidCPUs has

#define X_MIN                   0   // Coordinate index of the min x position
#define Y_MIN                   1   // Coordinate index of the min y position
#define X_MAX                   2   // Coordinate index of the max x position
#define Y_MAX                   3   // Coordinate index of the max y position

// Other definitions -----------------------------------------------------------
#define POLY_MASK_16        0xD295  // Used for random
#define POLY_MASK_15        0x6699  // Used for random


// Typedefs
typedef ap_uint<32> uint32;
typedef ap_int<32> int32;

typedef ap_uint<16> uint16;
typedef ap_int<16> int16;

typedef ap_int<12> int12;   // Used to represent position and negative velocity
typedef ap_int<12> uint12;

typedef ap_uint<8> uint8;
typedef ap_int<8> int8;

typedef ap_uint<4> uint4;
typedef ap_int<4> int4;

// Prototypes
void toplevel(hls::stream<uint32> &input, hls::stream<uint32> &output);

// Classes
class Vector {
 public:
    int12 x;
    int12 y;

    Vector();
    Vector(int12 x_, int12 y_);

    void add(Vector v);
    void sub(Vector v);
    void mul(int12 n);
    void div(int12 n);

    int12 mag();
    void normalise();
    void bound(int12 n);
    bool empty();

    void setMag(int12 mag);
    void limit(int12 max);

    static Vector add(Vector v1, Vector v2);
    static Vector sub(Vector v1, Vector v2);
//    static int12 distanceBetween(Vector v1, Vector v2);
    static uint12 squaredDistanceBetween(Vector v1, Vector v2);
    static bool equal(Vector v1, Vector v2);
};

class Boid {
 public:
    Vector position;        // The current pixel position of the boid
    Vector velocity;        // The current velocity of the boid
    uint16 id;              // TODO: Remove this on deployment
    uint16 index;           // Used to access relevant neighbouring boids

    Boid();
    Boid(uint16 _boidID, Vector initPosition, Vector initVelocity, int index);

    void update();                  // Calculate the boid's new position
    void draw();                    // Draw the boid (send to BoidGPU)

    void setNeighbourCount(int n);
    void printBoidInfo();

 private:
    Vector acceleration;
    uint8 neighbouringBoidsCount;

    Vector align();         // Calculate the alignment force
    Vector separate();      // Calculate the separation force
    Vector cohesion();      // Calculate the cohesion force

    Vector seek(Vector);    // Seek a new position
    void contain();         // Contain the boids within the simulation area
};

#endif
