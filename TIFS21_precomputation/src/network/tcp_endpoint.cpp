#include "network/tcp_endpoint.h"

#include <boost/bind/bind.hpp>


// Method to stop the endpoint
void TcpEndpoint::Stop() {
    StopListen();
    for (auto remote: this->GetRemoteNames()) {
        this->CloseChannel(remote);
    }
    io_service_.stop();
    tg.join_all();
};


// Method to get the names of all connected remote endpoints
std::vector<std::string> TcpEndpoint::GetRemoteNames() {
    std::vector<std::string> remotes;
    remotes.reserve(channels_.size());

    // Iterate over all channels and add their names to the vector
    for (const auto &channel: channels_) {
        remotes.push_back(channel.first);
    }

    return remotes;
};

// Method to write data from the buffer to the socket
void TcpChannel::DoWrite() {
    // Check if there is no writing in progress
    if (buffer_seq_.empty()) {
        // Switch buffers
        active_buffer_ ^= 1;
        // Add data from the active buffer to the buffer sequence
        for (const auto &data: buffers_[active_buffer_]) {
            buffer_seq_.emplace_back(boost::asio::buffer(data.first, data.second));
        }
        // Start an asynchronous write operation
        boost::asio::async_write(
                socket_, buffer_seq_,
                boost::bind(&TcpChannel::WriteHandler, shared_from_this(), boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
    }
}

// Handler for asynchronous write operations
void TcpChannel::WriteHandler(const boost::system::error_code &error, size_t size) {
    std::lock_guard<std::mutex
    > lock(buffer_mtx_);
    // Clear the active buffer and buffer sequence
    buffers_[active_buffer_].clear();
    buffer_seq_.clear();
    // Free the memory used by the data in the active buffer
    for (const auto &data: buffers_[active_buffer_]) {
        free(data.first);
    }
    // Check if there was no error
    if (!error) {
        // Check if there is more data to write
        if (!buffers_[active_buffer_ ^ 1].empty()) {
            DoWrite();
        }
    }
}

// Method to connect to a remote endpoint
void
TcpEndpoint::Connect(const std::string &remote_name, const std::string &remote_address, const std::string &local_name) {
    // Parse the remote address and port from the input string
    std::string addr = remote_address.substr(0, remote_address.find(':'));
    std::string port =
            remote_address.substr(remote_address.find(':') + 1, remote_address.length() - remote_address.find(':') - 1);
    // Resolve the remote address
    tcp::resolver::query query(addr, port, tcp::resolver::query::canonical_name);
    tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);
    tcp::resolver::iterator end;

    // Create a new TcpChannel object
    TcpChannel::TcpChannelPointer new_connection = TcpChannel::Create(this->io_service_);
    // Try to connect to the remote endpoint
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end) {
        int cnt = 0;
        new_connection->socket().close();
        new_connection->socket().connect(*endpoint_iterator, error);
        // Retry connecting if the connection was refused
        while (error == boost::asio::error::connection_refused && cnt < retryLimit) {
            new_connection->socket().close();
            new_connection->socket().connect(*endpoint_iterator, error);
            cnt++;
// std::cout << "retry connecting to " << remote_name << " in 3 seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        endpoint_iterator++;
    }
    // Check if there was an error
    if (error) throw boost::system::system_error(error);

    const char *cstr = local_name.c_str();
    new_connection->Write(cstr, nameSizeLimit);

    // Add the new channel to the map of channels
    channels_.insert(std::make_pair(remote_name, new_connection));
}


// Method to get the total amount of data sent in a more readable form
uint64 TcpEndpoint::GetTotalBytesSent() const {
    return total_bytes_sent_;
}

// Method to get the total amount of data received in a more readable form
uint64 TcpEndpoint::GetTotalBytesReceived() const {
    return total_bytes_received_;
}


// Method to reset the counters
void TcpEndpoint::ResetCounters() {
    total_bytes_sent_ = 0;
    total_bytes_received_ = 0;
}
