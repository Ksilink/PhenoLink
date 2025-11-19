# ZMQ Distributed Job Queue: Broker-Worker-Client Architecture

## Overview

This document describes the Majordomo Protocol implementation where a central broker coordinates job distribution between clients and workers. The broker receives jobs from clients, dispatches them to registered workers, and manages completion notifications. Workers register their capabilities and process jobs asynchronously.

---

## Trace 1: Broker Initialization and Main Event Loop

**Purpose**: Queue server startup - creates broker, binds to TCP port, and enters main polling loop to handle client/worker messages

### Flow Diagram
```
Broker Initialization and Main Event Loop
├── main() entry point
│   └── mainZMQ_QueueServer.cpp:99
│       ├── broker brk(verbose)                          [1a]
│       ├── brk.bind("tcp://*:13555")                    [1b]
│       └── brk.start_brokering()                        [1c]
│           └── while (!s_interrupted)
│               └── mdbroker.hpp:1200
│                   ├── zmq::poll(items, timeout)        [1d]
│                   └── if (items[0].revents & ZMQ_POLLIN)
│                       └── mdbroker.hpp:1209
│                           ├── zmsg *msg = new zmsg(*m_socket)
│                           ├── msg->pop_front() extracts sender/header
│                           ├── if (header == MDPC_CLIENT)  [1e]
│                           │   └── client_process(sender, msg)
│                           │       └── mdbroker.hpp:1224
│                           └── else if (header == MDPW_WORKER) [1f]
│                               └── worker_process(sender, msg)
│                                   └── mdbroker.hpp:1227
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 1a | `mainZMQ_QueueServer.cpp:217` | **Broker instantiation** - Creates the Majordomo broker instance that will coordinate all workers and clients |
| 1b | `mainZMQ_QueueServer.cpp:221` | **Bind to network port** - Broker listens on TCP port (default 13555) for incoming connections |
| 1c | `mainZMQ_QueueServer.cpp:223` | **Enter broker main loop** - Starts the broker's event loop to process messages |
| 1d | `mdbroker.hpp:1206` | **Poll for incoming messages** - Broker waits for messages on the ZMQ socket with timeout for heartbeat management |
| 1e | `mdbroker.hpp:1223` | **Route client messages** - Identifies message as coming from a client and routes to `client_process()` |
| 1f | `mdbroker.hpp:1226` | **Route worker messages** - Identifies message as coming from a worker and routes to `worker_process()` |

---

## Trace 2: Worker Registration and Plugin Advertisement

**Purpose**: Worker server startup - connects to broker, registers as available worker, and advertises plugin capabilities

### Flow Diagram
```
Worker Registration Flow
├── main() in mainZMQ_WorkerServer.cpp:646
│   └── ZMQThread instantiation                          [2a]
│       └── thread.start() calls run()
│           └── mainZMQ_WorkerServer.cpp:871
│               ├── Load plugins & build list
│               │   └── processlist->push_back()         [2b]
│               ├── session.set_worker_preamble()        [2c]
│               │   └── send_to_broker(MDPW_PROCESSLIST)
│               │       └── mdwrkapi.hpp:62
│               └── session.recv() loop starts
│                   └── mainZMQ_WorkerServer.cpp:348
│                       └── connect_to_broker()
│                           └── mdwrkapi.hpp:93
│                               └── send_to_broker(MDPW_READY) [2d]
│
└── Broker: start_brokering() main loop
    └── mdbroker.hpp:1197
        └── worker_process() handles message
            └── mdbroker.hpp:1227
                └── if (MDPW_PROCESSLIST)
                    └── mdbroker.hpp:1079
                        ├── Parse plugin JSON             [2e]
                        ├── Store in wrk->m_plugins
                        │   └── mdbroker.hpp:1100
                        └── srv->add_worker(wrk)          [2f]
                            └── Register to service queue
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 2a | `mainZMQ_WorkerServer.cpp:869` | **Create worker thread** - Instantiates the ZMQ worker thread that will connect to the broker proxy |
| 2b | `mainZMQ_WorkerServer.cpp:327` | **Build process list** - Prepares list of available plugins to advertise to broker |
| 2c | `mainZMQ_WorkerServer.cpp:345` | **Set worker preamble** - Configures worker with thread count and plugin list before registration |
| 2d | `mdwrkapi.hpp:106` | **Send READY to broker** - Worker announces availability to broker with MDPW_READY message |
| 2e | `mdbroker.hpp:1096` | **Broker parses plugin info** - Broker receives and parses worker's plugin capabilities from PROCESSLIST |
| 2f | `mdbroker.hpp:1106` | **Register worker to service** - Broker adds worker to the service queue for each advertised plugin |

---

## Trace 3: Client Job Submission to Broker

**Purpose**: Client submits processing job - connects to broker via mdcli session and sends job parameters

### Flow Diagram
```
Client Job Submission Flow
├── main() in mainzCli.cpp:524
│   └── startProcess() function                          [3a]
│       ├── mdcli session(server) - create client
│       ├── session.send() query plugin params
│       │   └── mainzCli.cpp:429
│       ├── helper.setupProcess() - prepare jobs
│       │   └── mainzCli.cpp:453
│       └── session.send(proc, req)                      [3b]
│           └── mdcli::send() in mdcliapi.hpp:82
│               ├── Format MDP protocol frames
│               │   └── mdcliapi.hpp:92
│               └── request->send(*m_client)             [3c]
│                   └── mdcliapi.hpp:100
│                       └── ZMQ socket transmission
│                           └── [Network: TCP to Broker]
│                               └── Broker main loop
│                                   └── mdbroker.hpp:1200
│                                       ├── zmq::poll() receives msg
│                                       ├── Route by header type
│                                       └── client_process()    [3d]
│                                           └── mdbroker.hpp:1224
│                                               ├── Parse service name
│                                               └── service_dispatch() [3e]
│                                                   └── mdbroker.hpp:1189
│                                                       ├── Create service_call
│                                                       ├── Queue in m_requests
│                                                       └── Attempt worker match
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 3a | `mainzCli.cpp:425` | **Create client session** - Client establishes ZMQ connection to broker server |
| 3b | `mainzCli.cpp:479` | **Send job to broker** - Client submits job request with process name and parameters via ZMQ |
| 3c | `mdcliapi.hpp:100` | **Transmit ZMQ message** - Client API sends formatted MDP message to broker socket |
| 3d | `mdbroker.hpp:1224` | **Broker receives client message** - Broker routes incoming client message to client processing handler |
| 3e | `mdbroker.hpp:1189` | **Dispatch to service queue** - Broker queues job for the requested service and attempts worker assignment |

---

## Trace 4: Broker Job Dispatch to Available Worker

**Purpose**: Broker matches queued job with available worker - selects worker thread, prepares parameters, and sends MDPW_REQUEST

### Flow Diagram
```
Broker Job Dispatch Flow
├── service_dispatch() in mdbroker.hpp:286
│   ├── while (!m_waiting_threads.empty() &&
│   │        !m_requests.empty())                        [4a]
│   │   └── mdbroker.hpp:342
│   │       ├── Find available worker for service
│   │       ├── Find available thread for worker
│   │       └── start_job(wrk, th, job)                  [4b]
│   │           └── mdbroker.hpp:408
│   │               ├── recurseExpandParameters()        [4c]
│   │               ├── job->parameters.insert(ThreadID)
│   │               │   └── mdbroker.hpp:1305
│   │               ├── thread->parameters = job
│   │               │   └── mdbroker.hpp:1312
│   │               └── worker_send(wrk, MDPW_REQUEST,   [4d]
│   │                   job->path, msg)
│   │                   └── mdbroker.hpp:1317
│   │                       └── msg->send(*m_socket)
│   │                           └── mdbroker.hpp:1154
│   │                               └── [ZMQ network transmission]
│
└── Worker receives in mdwrkapi.cpp
    └── mdwrk::recv() polling loop
        └── mdwrkapi.cpp:16
            ├── zmq::poll() on worker socket
            │   └── mdwrkapi.cpp:44
            ├── msg = new zmsg(*m_worker)
            │   └── mdwrkapi.cpp:47
            └── if (command.compare(MDPW_REQUEST) == 0)  [4e]
                └── mdwrkapi.cpp:73
                    └── return std::make_pair("Request", msg) [4f]
                        └── mdwrkapi.cpp:77
                            └── Returned to ZMQThread::run()
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 4a | `mdbroker.hpp:338` | **Match jobs to workers** - Broker checks for available worker threads and pending job requests |
| 4b | `mdbroker.hpp:408` | **Assign job to worker** - Broker initiates job assignment to selected worker thread |
| 4c | `mdbroker.hpp:1303` | **Expand job parameters** - Broker expands compressed parameters into full job specification |
| 4d | `mdbroker.hpp:1317` | **Send REQUEST to worker** - Broker transmits job request with parameters to worker via MDPW_REQUEST |
| 4e | `mdwrkapi.cpp:73` | **Worker receives REQUEST** - Worker's recv() loop detects incoming MDPW_REQUEST command from broker |
| 4f | `mdwrkapi.cpp:77` | **Return request to handler** - Worker API returns request message to application for processing |

---

## Trace 5: Worker Job Processing and Execution

**Purpose**: Worker processes received job - creates plugin instance, executes in separate thread, and prepares results

### Flow Diagram
```
Worker Job Processing Pipeline
├── ZMQThread::run() main loop
│   └── mainZMQ_WorkerServer.cpp:312
│       ├── session.recv() receives request
│       │   └── mainZMQ_WorkerServer.cpp:351
│       └── startProcessServer() called                  [5a]
│           └── mainZMQ_WorkerServer.cpp:424
│               ├── plugin->clone() creates instance     [5b]
│               │   └── mainZMQ_WorkerServer.cpp:543
│               ├── plugin->read(params) sets parameters
│               │   └── mainZMQ_WorkerServer.cpp:557
│               ├── RunnerThread* runner created
│               │   └── mainZMQ_WorkerServer.cpp:577
│               └── boost::thread spawned                [5c]
│                   └── mainZMQ_WorkerServer.cpp:579
│                       └── RunnerThread::operator()()
│                           └── mainZMQ_WorkerServer.cpp:290
│                               └── run_plugin(plugin)
│                                   └── mainZMQ_WorkerServer.cpp:294
│                                       ├── plugin->prepareData()
│                                       ├── plugin->exec()       [5d]
│                                       │   └── mainZMQ_WorkerServer.cpp:223
│                                       ├── plugin->finished()
│                                       └── plugin->gatherData() [5e]
│                                           └── mainZMQ_WorkerServer.cpp:227
│
├── Callback checks finished threads
│   ├── runner->join.try_join_for()
│   │   └── mainZMQ_WorkerServer.cpp:363
│   └── save_and_send_binary() async                    [5f]
│       └── mainZMQ_WorkerServer.cpp:376
│           └── NetworkProcessHandler::filterBinary()
│               └── mainZMQ_WorkerServer.cpp:609
│
└── session.send_to_broker(MDPW_READY)
    └── mainZMQ_WorkerServer.cpp:380
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 5a | `mainZMQ_WorkerServer.cpp:424` | **Start process handler** - Worker main loop calls startProcessServer with job path and parameters |
| 5b | `mainZMQ_WorkerServer.cpp:543` | **Clone plugin instance** - Worker creates isolated plugin instance for this specific job |
| 5c | `mainZMQ_WorkerServer.cpp:579` | **Launch processing thread** - Worker spawns boost thread to execute plugin processing asynchronously |
| 5d | `mainZMQ_WorkerServer.cpp:223` | **Execute plugin logic** - Plugin performs actual image processing or analysis work |
| 5e | `mainZMQ_WorkerServer.cpp:227` | **Gather results** - Plugin collects processing results into JSON object with timing info |
| 5f | `mainZMQ_WorkerServer.cpp:376` | **Save results async** - Worker saves binary results (images) asynchronously via HTTP |

---

## Trace 6: Job Completion and Result Notification

**Purpose**: Worker signals completion - sends MDPW_READY to broker, broker finalizes job and notifies other workers

### Flow Diagram
```
Worker Job Completion Flow
├── Worker thread completion callback
│   └── mainZMQ_WorkerServer.cpp:352
│       └── session.send_to_broker(MDPW_READY)           [6a]
│           └── mainZMQ_WorkerServer.cpp:380
│               └── ZMQ message transmission
│                   └── mdwrkapi.hpp:86
│
├── Broker main event loop
│   └── mdbroker.hpp:1197
│       └── worker_process() receives message
│           └── mdbroker.hpp:1227
│               └── if (command == MDPW_READY)           [6b]
│                   └── mdbroker.hpp:951
│                       ├── Match thread to finished job
│                       │   └── mdbroker.hpp:978
│                       ├── Decrement job counter
│                       │   └── mdbroker.hpp:988
│                       └── For each finished commit:
│                           └── mdbroker.hpp:1009
│                               ├── QtConcurrent::run(finalize) [6c]
│                               │   └── mdbroker.hpp:1025
│                               │       └── finalize_process()
│                               │           └── mdbroker.hpp:770
│                               │               ├── fuseArrow() - merge results
│                               │               │   └── mdbroker.hpp:839
│                               │               └── QProcess python scripts
│                               │                   └── mdbroker.hpp:877
│                               │
│                               └── worker_send(MDPW_FINISHED) [6d]
│                                   └── mdbroker.hpp:1014
│                                       └── ZMQ message to worker
│                                           └── mdbroker.hpp:1154
│
└── Worker receives FINISHED command
    └── mdwrk::recv() returns "Finished"
        └── mdwrkapi.cpp:91
            └── NetworkProcessHandler::clearData()       [6e]
                └── mainZMQ_WorkerServer.cpp:435
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 6a | `mainZMQ_WorkerServer.cpp:380` | **Signal job complete** - Worker sends MDPW_READY with client ID and thread ID to indicate job completion |
| 6b | `mdbroker.hpp:951` | **Broker receives READY** - Broker processes MDPW_READY as job completion signal from worker |
| 6c | `mdbroker.hpp:1025` | **Finalize commit async** - Broker triggers post-processing: fuse Arrow files and run Python scripts |
| 6d | `mdbroker.hpp:1014` | **Send FINISHED to worker** - Broker notifies worker that commit is complete and data can be cleared |
| 6e | `mainZMQ_WorkerServer.cpp:435` | **Worker clears data** - Worker receives MDPW_FINISHED and clears cached data for the commit |

---

## Trace 7: Heartbeat and Connection Management

**Purpose**: Workers maintain connection via periodic heartbeats - broker monitors liveness and purges expired workers

### Flow Diagram
```
Worker Heartbeat Mechanism
├── mdwrk::recv() polling loop
│   └── mdwrkapi.cpp:16
│       ├── zmq::poll() waits for messages
│       │   └── mdwrkapi.cpp:44
│       ├── Check heartbeat timer                        [7a]
│       │   └── mdwrkapi.cpp:113
│       └── Send heartbeat to broker                     [7b]
│           └── send_to_broker(MDPW_HEARTBEAT)
│               └── mdwrkapi.cpp:139
│
Broker Main Event Loop
├── start_brokering() poll loop
│   └── mdbroker.hpp:1197
│       ├── zmq::poll() receives messages
│       │   └── mdbroker.hpp:1206
│       ├── worker_process() handles worker msgs
│       │   └── mdbroker.hpp:1227
│       │       ├── Receive MDPW_HEARTBEAT               [7c]
│       │       │   └── mdbroker.hpp:1055
│       │       └── Update expiry timestamp              [7d]
│       │           └── mdbroker.hpp:1056
│       │
│       └── Periodic maintenance (every interval)
│           └── mdbroker.hpp:1239
│               ├── purge_workers() cleanup              [7e]
│               │   └── mdbroker.hpp:1240
│               │       └── Check each worker expiry     [7f]
│               │           └── mdbroker.hpp:238
│               │               └── worker_delete() if expired
│               │                   └── mdbroker.hpp:259
│               │                       └── Recover lost jobs to queue
│               │                           └── mdbroker.hpp:246
│               │
│               └── Send heartbeat to all workers
│                   └── mdbroker.hpp:1242
```

### Key Locations

| ID | Location | Description |
|----|----------|-------------|
| 7a | `mdwrkapi.cpp:113` | **Heartbeat timer check** - Worker checks if it's time to send heartbeat (every 2.5 seconds) |
| 7b | `mdwrkapi.cpp:139` | **Send heartbeat with metrics** - Worker sends MDPW_HEARTBEAT with available threads and system health metrics |
| 7c | `mdbroker.hpp:1055` | **Broker receives heartbeat** - Broker processes worker heartbeat and updates expiry timestamp |
| 7d | `mdbroker.hpp:1056` | **Update worker expiry** - Broker extends worker's expiry time (3-5 heartbeat intervals) |
| 7e | `mdbroker.hpp:1240` | **Purge expired workers** - Broker periodically removes workers that haven't sent heartbeat in time |
| 7f | `mdbroker.hpp:238` | **Check worker expiry** - Broker identifies expired workers and recovers their lost jobs back to queue |

---

## Key Components

### Files
- **`mdbroker.hpp`** - Majordomo broker implementation (central coordinator)
- **`mdwrkapi.hpp/cpp`** - Worker API for connecting to broker and processing jobs
- **`mdcliapi.hpp`** - Client API for submitting jobs to broker
- **`mainZMQ_QueueServer.cpp`** - Broker server entry point
- **`mainZMQ_WorkerServer.cpp`** - Worker server entry point with plugin management
- **`mainzCli.cpp`** - Client application entry point

### Protocol Messages
- **`MDPC_CLIENT`** - Client message header
- **`MDPW_WORKER`** - Worker message header
- **`MDPW_READY`** - Worker ready for work / job completion signal
- **`MDPW_REQUEST`** - Broker job dispatch to worker
- **`MDPW_HEARTBEAT`** - Keep-alive message
- **`MDPW_PROCESSLIST`** - Worker plugin capability advertisement
- **`MDPW_FINISHED`** - Commit finalization complete

### Architecture Patterns
- **Event-driven polling** - ZMQ poll() with timeout for message processing
- **Heartbeat-based liveness** - Workers expire if no heartbeat received
- **Queue-based job dispatch** - Broker maintains request queues per service
- **Plugin architecture** - Workers advertise and execute modular plugins
- **Asynchronous processing** - Jobs execute in separate threads with callbacks
- **Result finalization** - Post-processing phase with data fusion and cleanup

---

**Document Generated**: 2025-11-18  
**Last Updated**: 2025-11-18
