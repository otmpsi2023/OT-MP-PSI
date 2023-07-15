#ifndef OTMPSI_NETWORK_TCPENDPOINT_H_
#define OTMPSI_NETWORK_TCPENDPOINT_H_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>


#include "endpoint.h"

using boost::asio::ip::tcp;

const int nameSizeLimit = 128;
const int retryLimit = 20;

// Class for a TCP channel
class TcpChannel : public boost::enable_shared_from_this<TcpChannel> {
public:
    typedef boost::shared_ptr <TcpChannel> TcpChannelPointer;

    // Constructor that takes a reference to an io_service object
    explicit TcpChannel(boost::asio::io_service &io_service) : socket_(io_service) {};

    // Factory method to create a new TcpChannel object
    static TcpChannelPointer Create(boost::asio::io_service &io_service) {
        return TcpChannelPointer(new TcpChannel(io_service));
    }

    // Method to asynchronously write data to the channel
    inline void AsyncWrite(void *buf, uint32 len);

    // Method to write data to the channel
    inline void Write(const void *buf, uint32 len);

    // Method to read data from the channel
    inline void Read(void *buf, uint32 len);

    // Method to get a reference to the underlying socket
    inline tcp::socket &socket();

private:
    // Method to write data from the buffer to the socket
    void DoWrite();

    // Handler for asynchronous write operations
    void WriteHandler(const boost::system::error_code &error, size_t size);

    tcp::socket socket_;
    std::mutex buffer_mtx_;
    std::vector<std::pair<void *, int>> buffers_[2]; // a double buffer
    std::vector<boost::asio::const_buffer> buffer_seq_;
    int active_buffer_ = 0;
};

// Method to asynchronously write data to the channel
void TcpChannel::AsyncWrite(void *buf, uint32 len) {
    std::lock_guard<std::mutex> lock(buffer_mtx_);
    buffers_[active_buffer_ ^ 1].emplace_back(buf, len); // move input data to the inactive buffer
    DoWrite();
}

// Method to write data to the channel
void TcpChannel::Write(const void *buf, uint32 len) {
    boost::system::error_code error;
    boost::asio::write(socket_, boost::asio::buffer(buf, len), error);
    if (error) {
        std::cerr << "Error writing to socket: " << error.message() << std::endl;
    }
}

// Method to read data from the channel
void TcpChannel::Read(void *buf, uint32 len) {
    boost::system::error_code error;
    boost::asio::read(socket_, boost::asio::buffer(buf, len), error);
    if (error) {
        std::cerr << "Error reading from socket: " << error.message() << std::endl;
    }
}

// Method to get a reference to the underlying socket
tcp::socket &TcpChannel::socket() { return socket_; }

// Class for a TCP endpoint
class TcpEndpoint : public Endpoint {
public:
    // Delete the default constructor
    TcpEndpoint() = delete;

    // Default destructor
    ~TcpEndpoint() override = default;

    // Constructor that takes a port number
    explicit TcpEndpoint(int port) : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), resolver_(io_service_) {};

    // Method to start the endpoint
    inline void Start() override;

    // Method to stop the endpoint
    inline void Stop() override;

    // Method to stop listen
    inline void StopListen() override;

    // Method to close a connection with a remote endpoint
    inline void CloseChannel(const std::string &remote_name) override;

    // Method to write data to a remote endpoint
    inline void Write(const std::string &remote_name, const void *buf, uint32 len) override;

    // Method to asynchronously write data to a remote endpoint
    inline void AsyncWrite(const std::string &remote_name, void *buf, uint32 len) override;

    // Method to read data from a remote endpoint
    inline void Read(const std::string &remote_name, void *buf, uint32 len) override;

    // Method to get the names of all connected remote endpoints
    std::vector<std::string> GetRemoteNames() override;

    // Method to connect to a remote endpoint
    void
    Connect(const std::string &remote_name, const std::string &remote_address, const std::string &local_name) override;

    // Method to get the total amount of data sent in a more readable form
    uint64 GetTotalBytesSent() const override;

    // Method to get the total amount of data received in a more readable form
    uint64 GetTotalBytesReceived() const override;

    // Method to reset the total amount of data sent and received
    void ResetCounters() override;

private:
    // Handler for starting the endpoint
    inline void StartHandler();

    // Method to start accepting incoming connections
    inline void StartAccept();

    // Handler for accepting incoming connections
    inline void AcceptHandler(const TcpChannel::TcpChannelPointer &new_connection,
                              const boost::system::error_code &error);

    std::unordered_map<std::string, TcpChannel::TcpChannelPointer> channels_;
    boost::asio::io_service io_service_;
    tcp::acceptor acceptor_;
    tcp::resolver resolver_;
    bool accept_flag;

    boost::thread_group tg;

    //member variables to record the time and amount of data sent/received
    uint64 total_bytes_sent_ = 0;
    uint64 total_bytes_received_ = 0;
    std::chrono::duration<double> total_network_time_ = std::chrono::duration<double>::zero();
};

// Method to start the endpoint
void TcpEndpoint::Start() {
    tg.create_thread(boost::bind(&TcpEndpoint::StartHandler, this));
};

// Method to close a connection with a remote endpoint
void TcpEndpoint::CloseChannel(const std::string &remote_name) { channels_.erase(remote_name); };


// Method to stop the endpoint  listen
void TcpEndpoint::StopListen() {
    accept_flag = false;
    acceptor_.close();
}

// Method to write data to a remote endpoint
void TcpEndpoint::Write(const std::string &remote_name, const void *buf, uint32 len) {
    channels_[remote_name]->Write(buf, len);
    total_bytes_sent_ += len;
};

// Method to asynchronously write data to a remote endpoint
void TcpEndpoint::AsyncWrite(const std::string &remote_name, void *buf, uint32 len) {
    channels_[remote_name]->AsyncWrite(buf, len);
};

// Method to read data from a remote endpoint
void TcpEndpoint::Read(const std::string &remote_name, void *buf, uint32 len) {
    channels_[remote_name]->Read(buf, len);
    total_bytes_received_ += len;
};

// Handler for starting the endpoint
void TcpEndpoint::StartHandler() {
    accept_flag = true;
    StartAccept();
    io_service_.run();
};

// Method to start accepting incoming connections
void TcpEndpoint::StartAccept() {
    TcpChannel::TcpChannelPointer new_connection = TcpChannel::Create(this->io_service_);
    acceptor_.async_accept(new_connection->socket(), boost::bind(&TcpEndpoint::AcceptHandler, this, new_connection,
                                                                 boost::asio::placeholders::error));
};

// Handler for accepting incoming connections
void TcpEndpoint::AcceptHandler(const TcpChannel::TcpChannelPointer &new_connection,
                                const boost::system::error_code &error) {
    if (accept_flag) {
        if (error) {
            std::cerr << "Error accepting connection: " << error.message() << std::endl;
        } else {
            uint8 buffer[nameSizeLimit];
            new_connection->Read(buffer, nameSizeLimit);
            std::string remoteName(reinterpret_cast<char *>(buffer));
            channels_.insert(std::make_pair(remoteName, new_connection));
        }
        StartAccept();
    }
};


#endif // OTMPSI_NETWORK_TCPENDPOINT_H_
