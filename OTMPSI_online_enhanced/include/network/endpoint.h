#ifndef OTMPSI_NETWORK_ENDPOINT_H_
#define OTMPSI_NETWORK_ENDPOINT_H_

#include <string>

#include "utils/common.h"

// Abstract base class for network endpoints
class Endpoint {
public:
    // Virtual destructor
    virtual ~Endpoint() = default;

    // Method to start the endpoint
    virtual void Start() = 0;

    // Method to stop the endpoint
    virtual void Stop() = 0;

    // Method to stop listen
    virtual void StopListen() = 0;

    // Method to connect to a remote endpoint
    virtual void
    Connect(const std::string &remote_name, const std::string &remote_address, const std::string &local_name) = 0;

    // Method to close a connection with a remote endpoint
    virtual void CloseChannel(const std::string &remote_name) = 0;

    // Method to write data to a remote endpoint
    virtual void Write(const std::string &remote_name, const void *buf, uint32 len) = 0;

    // Method to asynchronously write data to a remote endpoint
    virtual void AsyncWrite(const std::string &remote_name, void *buf, uint32 len) = 0;

    // Method to read data from a remote endpoint
    virtual void Read(const std::string &remote_name, void *buf, uint32 len) = 0;

    // Method to get the names of all connected remote endpoints
    virtual std::vector<std::string> GetRemoteNames() = 0;

    // Method to get the total amount of data sent in a more readable form
    virtual uint64 GetTotalBytesSent() const = 0;

    // Method to get the total amount of data received in a more readable form
    virtual uint64 GetTotalBytesReceived() const = 0;

    // Method to reset the total amount of data sent and received
    virtual void ResetCounters() = 0;
};

#endif // OTMPSI_NETWORK_ENDPOINT_H_
