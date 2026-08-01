#pragma once

#include <asio.hpp>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <iostream>
#include <algorithm>
#include <cstring>

using asio::ip::tcp;

// Forward declarations to handle circular dependency
class Server;
template <typename T> class ThreadSafeQueue;

// ==========================================
// CLIENT SESSION MANAGEMENT
// ==========================================
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(tcp::socket socket, Server& server);
    ~ClientSession();

    void start();
    void send_binary_data(const std::vector<uint8_t>& raw_bytes);

private:
    void do_read();
    void handle_received_integer(int value);
    void do_write_external();

    tcp::socket socket_;
    Server& server_;
    char read_buffer_[sizeof(int)];
    std::deque<std::vector<uint8_t>> write_queue_;
};

// ==========================================
// SERVER ARCHITECTURE
// ==========================================
class Server {
public:
    Server(asio::io_context& io_context, short port,
        ThreadSafeQueue<int>& outbound_queue,
        ThreadSafeQueue<int>& inbound_queue);

    void broadcast_bytes(const std::vector<uint8_t>& raw_bytes);
    void push_to_inbound_queue(int value);
    void register_client(std::shared_ptr<ClientSession> client);
    void unregister_client(std::shared_ptr<ClientSession> client);

private:
    void do_accept();
    void process_outbound_queue();

    tcp::acceptor acceptor_;
    ThreadSafeQueue<int>& outbound_queue_;
    ThreadSafeQueue<int>& inbound_queue_;
    asio::steady_timer check_queue_timer_;
    std::vector<std::shared_ptr<ClientSession>> active_clients_;
    std::mutex clients_mutex_;
};