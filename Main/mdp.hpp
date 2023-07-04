
#ifndef MDP_HPP
#define MDP_HPP
// From zguide of zmq protocol in C++
// Adding the _PL to the Client / Worker info
//  mdp.h
//  Majordomo Protocol definitions
//


//  This is the version of MDP/Client we implement
#define MDPC_CLIENT         "MDPC01_PL"

//  This is the version of MDP/Worker we implement
#define MDPW_WORKER         "MDPW01_PL"

//  MDP/Server commands, as strings
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPLY          "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"
#define MDPW_PROCESSLIST    "\006"

static char *mdps_commands [] = {
    NULL,
    (char*)"READY", (char*)"REQUEST",
          (char*)"REPLY", (char*)"HEARTBEAT",
          (char*)"DISCONNECT", (char*)"PROCESSES"
};


#endif // MDP_HPP
