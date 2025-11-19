# Worker Crash Notification Feature - Design Document

## Executive Summary

**Goal**: Notify clients when workers crash or when no workers are available to handle submitted jobs.

**Feasibility**: ✅ **YES** - The current architecture fully supports this feature with minimal modifications.

---

## Current Architecture Analysis

### Existing Communication Patterns

#### 1. **Client-Broker Communication**
- **Protocol**: Request/Reply pattern with timeout
- **Client sends**: Job requests via `session.send()`
- **Client receives**: Responses via `session.recv()` with 2.5 second timeout
- **Status**: `_status` flag tracks connection health
- **Location**: `mdcliapi.hpp:111-144`

#### 2. **Worker Heartbeat Mechanism** (Already Implemented!)
- Workers send `MDPW_HEARTBEAT` every 2.5 seconds
- Broker tracks worker expiry: `wrk->m_expiry = s_clock() + HEARTBEAT_EXPIRY`
- Broker purges expired workers in `purge_workers()` (line 231-261)
- **Recovery**: Lost jobs are pushed back to queue (line 246)
- **Location**: `mdbroker.hpp:231-261`

#### 3. **Internal Service Commands** (Already Implemented!)
- Broker supports `mmi.*` commands for internal operations
- Examples: `mmi.status`, `mmi.list`, `mmi.workers`, `mmi.health`
- Clients can query broker status without worker involvement
- **Location**: `mdbroker.hpp:631-662`

### Current Gaps

❌ **No immediate notification** when worker crashes  
❌ **No error response** when no workers can handle a job  
❌ **Client must poll** `mmi.status` to check job progress  
❌ **No protocol message** for job failure/rejection  

---

## Proposed Solution Architecture

### Overview

Implement **proactive broker-to-client notifications** for two scenarios:

1. **Worker Crash During Job Execution**: Broker detects worker death and notifies affected client
2. **No Available Workers**: Broker immediately rejects job if no workers can handle it

### Key Design Principles

✅ **Minimal Protocol Changes**: Add 2 new protocol messages  
✅ **Backward Compatible**: Existing clients continue to work  
✅ **Asynchronous Notifications**: Don't block broker operations  
✅ **Client Timeout Preserved**: Existing timeout mechanism still works  

---

## Detailed Design

### Phase 1: Protocol Extensions

#### New Protocol Messages (`mdp.hpp`)

```cpp
// Add to mdp.hpp after line 24:
#define MDPC_WORKER_DIED     (char*)"\010"    // Worker crashed during your job
#define MDPC_NO_WORKERS      (char*)"\011"    // No workers available for service
```

Update command array:
```cpp
static char *mdps_commands [] = {
    NULL,
    (char*)"READY", (char*)"REQUEST",
    (char*)"REPLY", (char*)"HEARTBEAT",
    (char*)"DISCONNECT", (char*)"PROCESSES",
    (char*)"FINISHED", (char*)"WORKER_DIED",
    (char*)"NO_WORKERS"
};
```

---

### Phase 2: Broker Modifications

#### 2.1 Track Job-to-Client Mapping

**Location**: `mdbroker.hpp` - Enhance `service_call` structure

```cpp
// Add to service_call structure (around line 120):
struct service_call {
    QString path;
    QString project;
    QString client;          // Already exists
    QCborMap callMap;
    QCborMap parameters;
    int* calls;
    int priority;
    
    // NEW FIELDS:
    int64_t submit_time;     // When job was submitted
    bool client_notified;    // Prevent duplicate notifications
};
```

#### 2.2 Enhanced Worker Purge with Client Notification

**Location**: `mdbroker.hpp:231-261` - Modify `purge_workers()`

```cpp
void purge_workers()
{
    QSet<worker*> toCull;
    QSet<service_call*> failedJobs;  // NEW: Track failed jobs
    int64_t now = s_clock();
    
    for (auto wrk = m_workers_threads.begin(); wrk != m_workers_threads.end(); ++wrk)
    {
        if ((*wrk)->m_worker->m_expiry <= now)
        {
            toCull.insert((*wrk)->m_worker);
            
            // NEW: Capture jobs that were being processed
            if ((*wrk)->parameters)
            {
                for (auto& job: m_ongoing_jobs)
                {
                    if (job == (*wrk)->parameters)
                    {
                        // Track failed job for notification
                        if (!job->client_notified)
                        {
                            failedJobs.insert(job);
                            job->client_notified = true;
                        }
                        
                        // Re-queue job for retry
                        m_requests.push_front(job);
                    }
                }
            }
        }
    }
    
    // Delete expired workers
    for (auto wrk : toCull)
    {
        qDebug() << "I: deleting expired worker:" << wrk->m_identity;
        worker_delete(wrk, 0);
    }
    
    // NEW: Send crash notifications to affected clients
    for (auto job : failedJobs)
    {
        send_worker_crash_notification(job);
    }
}
```

#### 2.3 New Helper: Send Worker Crash Notification

**Location**: `mdbroker.hpp` - Add new method

```cpp
// Add after purge_workers() method (around line 261)
void send_worker_crash_notification(service_call* job)
{
    qDebug() << "Notifying client" << job->client << "that worker crashed for job" << job->path;
    
    zmsg* msg = new zmsg();
    
    // Build notification message
    QCborMap notification;
    notification["error"] = "WORKER_CRASHED";
    notification["service"] = job->path;
    notification["message"] = "Worker processing your job has crashed. Job will be retried.";
    notification["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    notification["job_id"] = job->parameters.value("JobID").toString();
    
    msg->push_back(notification.toCborValue().toCbor());
    
    // Format as MDP client message
    msg->wrap(QString(MDPC_CLIENT));
    msg->wrap(job->client, "");
    
    // Send to client
    msg->send(*m_socket);
    delete msg;
}
```

#### 2.4 Check Worker Availability on Job Submission

**Location**: `mdbroker.hpp:286-415` - Modify `service_dispatch()`

```cpp
void service_dispatch(service *srv, zmsg *msg)
{
    if (!srv) return;
    
    // Queue message if provided
    if (msg)
    {
        auto client = msg->pop_front();
        auto empty = msg->pop_front();
        auto map = QCborValue::fromCbor(msg->pop_front()).toMap();
        
        // Queue all job calls
        while (msg->parts())
        {
            service_call* call = new service_call;
            call->path = srv->m_name;
            call->client = client;
            call->callMap = map;
            call->parameters = QCborValue::fromCbor(msg->pop_front()).toMap();
            call->submit_time = s_clock();  // NEW
            call->client_notified = false;  // NEW
            
            m_requests.push_back(call);
        }
        
        // NEW: Check if ANY workers can handle this service
        bool has_capable_worker = false;
        for (auto w = m_workers.begin(); w != m_workers.end(); ++w)
        {
            if (srv->m_process.contains(w.value()))
            {
                has_capable_worker = true;
                break;
            }
        }
        
        // NEW: If no workers exist for this service, notify client immediately
        if (!has_capable_worker)
        {
            qDebug() << "No workers available for service" << srv->m_name;
            send_no_workers_notification(client, srv->m_name);
            
            // Optionally remove jobs from queue or mark them
            // For now, keep them queued in case worker comes online
        }
    }
    
    purge_workers();
    
    // Rest of dispatch logic remains the same...
    while (!m_waiting_threads.empty() && !m_requests.empty())
    {
        // ... existing worker assignment logic ...
    }
}
```

#### 2.5 New Helper: Send No Workers Notification

**Location**: `mdbroker.hpp` - Add new method

```cpp
void send_no_workers_notification(const QString& client, const QString& service)
{
    qDebug() << "Notifying client" << client << "that no workers available for" << service;
    
    zmsg* msg = new zmsg();
    
    // Build notification message
    QCborMap notification;
    notification["error"] = "NO_WORKERS";
    notification["service"] = service;
    notification["message"] = "No workers are currently available to process this job.";
    notification["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    msg->push_back(notification.toCborValue().toCbor());
    
    // Format as MDP client message
    msg->wrap(QString(MDPC_CLIENT));
    msg->wrap(client, "");
    
    // Send to client
    msg->send(*m_socket);
    delete msg;
}
```

---

### Phase 3: Client Modifications

#### 3.1 Enhanced Client Receive Logic

**Location**: `mdcliapi.hpp:111-144` - Modify `recv()` method

```cpp
zmsg* recv()
{
    zmq::pollitem_t items[] = { { *m_client, 0, ZMQ_POLLIN, 0 } };
    zmq::poll(items, 1, std::chrono::milliseconds(m_timeout));
    
    if (items[0].revents & ZMQ_POLLIN)
    {
        zmsg *msg = new zmsg(*m_client);
        
        assert(msg->parts() >= 3);
        assert(msg->pop_front().length() == 0);  // empty frame
        
        auto header = msg->pop_front();
        assert(header == MDPC_CLIENT);
        
        auto service = msg->pop_front();
        
        // NEW: Check if this is an error notification
        if (msg->parts() > 0)
        {
            auto firstFrame = msg->front();
            auto data = QCborValue::fromCbor(firstFrame);
            
            if (data.isMap())
            {
                auto map = data.toMap();
                if (map.contains("error"))
                {
                    // This is an error notification!
                    handle_error_notification(map);
                    // Don't return this as normal reply
                    delete msg;
                    return nullptr;
                }
            }
        }
        
        return msg;  // Normal reply
    }
    
    _status = false;
    return nullptr;
}

protected:
    // NEW: Virtual method for error handling (can be overridden)
    virtual void handle_error_notification(const QCborMap& error)
    {
        QString errorType = error["error"].toString();
        QString message = error["message"].toString();
        QString service = error["service"].toString();
        
        qWarning() << "ERROR from broker:" << errorType;
        qWarning() << "Service:" << service;
        qWarning() << "Message:" << message;
        
        // Subclasses can override this for GUI popups
    }
```

#### 3.2 GUI Client with Popup Notifications

**Location**: `mainzCli.cpp` or new GUI client wrapper

```cpp
class NotifyingMDCli : public mdcli
{
    Q_OBJECT

public:
    NotifyingMDCli(QString broker, QString id = QString(), int verbose = 0)
        : mdcli(broker, id, verbose)
    {}

signals:
    void workerCrashed(QString service, QString message);
    void noWorkersAvailable(QString service, QString message);

protected:
    void handle_error_notification(const QCborMap& error) override
    {
        QString errorType = error["error"].toString();
        QString message = error["message"].toString();
        QString service = error["service"].toString();
        
        // Show popup notification
        if (errorType == "WORKER_CRASHED")
        {
            QMessageBox::warning(
                nullptr,
                "Worker Crashed",
                QString("A worker processing job '%1' has crashed.\n\n%2\n\nThe job will be automatically retried.")
                    .arg(service)
                    .arg(message)
            );
            emit workerCrashed(service, message);
        }
        else if (errorType == "NO_WORKERS")
        {
            QMessageBox::critical(
                nullptr,
                "No Workers Available",
                QString("No workers are available to process job '%1'.\n\n%2\n\nPlease start worker servers and try again.")
                    .arg(service)
                    .arg(message)
            );
            emit noWorkersAvailable(service, message);
        }
        
        // Call base implementation for logging
        mdcli::handle_error_notification(error);
    }
};
```

---

## Implementation Flow Diagrams

### Scenario 1: Worker Crashes During Job Execution

```
┌─────────┐                 ┌────────┐                 ┌────────┐
│ Client  │                 │ Broker │                 │ Worker │
└────┬────┘                 └───┬────┘                 └───┬────┘
     │                          │                          │
     │ 1. Send Job Request      │                          │
     ├─────────────────────────>│                          │
     │                          │                          │
     │                          │ 2. Dispatch Job          │
     │                          ├─────────────────────────>│
     │                          │                          │
     │                          │                          │ 3. Worker
     │                          │                          │    Processing...
     │                          │                          │
     │                          │                          X 4. CRASH!
     │                          │                          │    (no heartbeat)
     │                          │                          │
     │                    5. Heartbeat Timeout             │
     │                       purge_workers()               │
     │                          │                          │
     │ 6. MDPC_WORKER_DIED      │                          │
     │<─────────────────────────┤                          │
     │    Notification          │                          │
     │                          │                          │
     ▼ 7. Show Popup            │ 8. Re-queue Job          │
   [Warning Dialog]             │                          │
   "Worker crashed,             │                          │
    retrying..."                │                          │
                                │ 9. Assign to Another     │
                                │    Worker (if available) │
                                ▼                          ▼
```

### Scenario 2: No Workers Available for Service

```
┌─────────┐                 ┌────────┐
│ Client  │                 │ Broker │
└────┬────┘                 └───┬────┘
     │                          │
     │ 1. Send Job Request      │
     ├─────────────────────────>│
     │   (service="PluginX")    │
     │                          │
     │                    2. Check Workers
     │                       for "PluginX"
     │                          │
     │                          │ m_workers.empty()
     │                          │ OR no worker has
     │                          │ PluginX capability
     │                          │
     │ 3. MDPC_NO_WORKERS       │
     │<─────────────────────────┤
     │    Immediate Notification│
     │                          │
     ▼ 4. Show Popup            │
   [Critical Dialog]            │
   "No workers available        │
    for PluginX. Start          │
    worker servers."            ▼
```

---

## Backward Compatibility

### Existing Clients (No Modifications)
- Continue to work with timeout mechanism
- `recv()` returns `nullptr` on timeout
- No popup notifications, but logs will show warnings

### New Clients (With Modifications)
- Receive proactive crash notifications
- Display popup warnings/errors
- Better user experience

---

## Testing Strategy

### Test Case 1: Worker Crash Detection
```
1. Start broker
2. Start 1 worker
3. Submit job from client
4. Kill worker process (simulate crash)
5. Wait for heartbeat timeout (~7.5 seconds)
6. Verify: Client receives MDPC_WORKER_DIED notification
7. Verify: Popup appears with crash message
8. Verify: Job is re-queued in broker
```

### Test Case 2: No Workers Available
```
1. Start broker (NO workers)
2. Submit job from client
3. Verify: Client immediately receives MDPC_NO_WORKERS
4. Verify: Popup appears with "no workers" message
5. Start a worker
6. Resubmit job
7. Verify: Job processes successfully
```

### Test Case 3: Worker Dies After Partial Completion
```
1. Start broker + worker
2. Submit multi-step job
3. Kill worker after step 1 completes
4. Verify: Client notified of crash
5. Verify: Job restarted from beginning or resumed
```

---

## Configuration Options

### Broker Configuration (`mdbroker.hpp`)

```cpp
// Add configuration flags
bool m_notify_worker_crash = true;      // Send crash notifications
bool m_notify_no_workers = true;        // Send no-worker notifications
int m_retry_attempts = 3;               // Max retries for failed jobs
int64_t m_crash_notification_delay = 0; // Delay before notifying (ms)
```

### Client Configuration (`mdcliapi.hpp`)

```cpp
// Add configuration flags
bool m_enable_notifications = true;     // Receive error notifications
bool m_show_popups = true;              // Display GUI popups
bool m_log_errors = true;               // Log errors to console
```

---

## Performance Considerations

### Broker Impact
- **Minimal**: One additional ZMQ message per crashed worker
- **No blocking**: Notifications sent asynchronously
- **Memory**: Small QCborMap per notification (~200 bytes)

### Network Impact
- **Crash notification**: ~150-300 bytes per message
- **Frequency**: Only on worker death (rare event)
- **Latency**: <1ms to send notification

### Client Impact
- **Minimal**: One additional `recv()` path for error messages
- **GUI blocking**: Popup shows in separate thread (non-blocking)

---

## Future Enhancements

### Phase 4: Advanced Features

1. **Job Retry Policy**
   - Configurable retry attempts
   - Exponential backoff
   - Persistent job queue

2. **Worker Health Scoring**
   - Track worker crash history
   - Prefer stable workers
   - Auto-disable problematic workers

3. **Client Job Tracking**
   - Unique job IDs
   - Query job status anytime
   - Job history/logs

4. **Worker Auto-Restart**
   - Detect crashes at OS level
   - Auto-restart worker processes
   - Alert administrators

---

## Implementation Checklist

- [ ] **Phase 1**: Protocol Extensions
  - [ ] Add `MDPC_WORKER_DIED` constant
  - [ ] Add `MDPC_NO_WORKERS` constant
  - [ ] Update command array

- [ ] **Phase 2**: Broker Modifications
  - [ ] Enhance `service_call` structure
  - [ ] Modify `purge_workers()` for notifications
  - [ ] Add `send_worker_crash_notification()`
  - [ ] Modify `service_dispatch()` for worker check
  - [ ] Add `send_no_workers_notification()`

- [ ] **Phase 3**: Client Modifications
  - [ ] Modify `mdcli::recv()` for error handling
  - [ ] Add `handle_error_notification()` method
  - [ ] Create `NotifyingMDCli` subclass
  - [ ] Integrate popup notifications

- [ ] **Phase 4**: Testing
  - [ ] Write unit tests
  - [ ] Test crash scenarios
  - [ ] Test no-worker scenarios
  - [ ] Test backward compatibility

- [ ] **Phase 5**: Documentation
  - [ ] Update ARCHITECTURE.md
  - [ ] Add API documentation
  - [ ] Create user guide

---

## Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Notification flood during mass worker crash | High | Low | Rate limiting, batch notifications |
| Client can't handle notification format | Medium | Low | Backward compatible, optional feature |
| Broker performance degradation | Low | Very Low | Async notifications, minimal overhead |
| Notification lost in network | Medium | Low | Client timeout still works as fallback |

---

## Conclusion

✅ **Implementation is feasible** with the current architecture  
✅ **Minimal code changes** required (< 200 lines total)  
✅ **Backward compatible** with existing clients  
✅ **Performance impact negligible**  
✅ **Improves user experience significantly**  

The design leverages existing infrastructure (heartbeat mechanism, internal services, ZMQ messaging) and adds proactive notifications without breaking existing functionality.

**Recommended**: Proceed with implementation in phases, starting with Protocol Extensions.
