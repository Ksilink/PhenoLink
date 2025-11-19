
#ifndef MDP_HPP
#define MDP_HPP
// From zguide of zmq protocol in C++
// Adding the _PL to the Client / Worker info
//  mdp.h
//  Majordomo Protocol definitions
//


//  This is the version of MDP/Client we implement
#define MDPC_CLIENT         (char*)"MDPC01_PL"

//  This is the version of MDP/Worker we implement
#define MDPW_WORKER         (char*)"MDPW01_PL"

//  MDP/Server commands, as strings
#define MDPW_READY          (char*)"\001"
#define MDPW_REQUEST        (char*)"\002"
#define MDPW_REPLY          (char*)"\003"
#define MDPW_HEARTBEAT      (char*)"\004"
#define MDPW_DISCONNECT     (char*)"\005"
#define MDPW_PROCESSLIST    (char*)"\006"
#define MDPW_FINISHED       (char*)"\007"

//  MDP/Client notification commands (broker to client)
#define MDPC_WORKER_DIED    (char*)"\010"    // Worker crashed during job
#define MDPC_NO_WORKERS     (char*)"\011"    // No workers available for service

static char *mdps_commands [] = {
    NULL,
    (char*)"READY", (char*)"REQUEST",
    (char*)"REPLY", (char*)"HEARTBEAT",
    (char*)"DISCONNECT", (char*)"PROCESSES",
    (char*)"FINISHED", (char*)"WORKER_DIED",
    (char*)"NO_WORKERS"
};


#endif // MDP_HPP
