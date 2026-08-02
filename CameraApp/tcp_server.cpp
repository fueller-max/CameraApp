#include "tcp_server.h"
#include "thread_safe_queue.h" 

// ==========================================
// CLIENT SESSION IMPLEMENTATION
// ==========================================

ClientSession::ClientSession(tcp::socket socket, Server & server)
    : socket_(std::move(socket)), server_(server) {
}

ClientSession::~ClientSession() {
    server_.unregister_client(shared_from_this());
}

void ClientSession::start() {
    std::cout << "PLC connected: " << socket_.remote_endpoint() << std::endl;
    do_read();
}

void ClientSession::send_binary_data(const std::vector<uint8_t>& raw_bytes) {
    auto self(shared_from_this());
    asio::post(socket_.get_executor(), [this, self, raw_bytes]() {
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push_back(raw_bytes);
        if (!write_in_progress) {
            do_write_external();
        }
        });
}

void ClientSession::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(read_buffer_, sizeof(int)),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                if (length == sizeof(int)) {
                    int received_val = 0;
                    std::memcpy(&received_val, read_buffer_, sizeof(int));
                    handle_received_integer(received_val);
                }
                do_read();
            }
        });
}

void ClientSession::handle_received_integer(int value) {
    server_.push_to_inbound_queue(value);
}

void ClientSession::do_write_external() {
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(write_queue_.front().data(), write_queue_.front().size()),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                write_queue_.pop_front();
                if (!write_queue_.empty()) {
                    do_write_external();
                }
            }
        });
}

// ==========================================
// SERVER IMPLEMENTATION
// ==========================================

Server::Server(asio::io_context& io_context, short port,
    ThreadSafeQueue<int>& outbound_queue,
    ThreadSafeQueue<int>& inbound_queue)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
    outbound_queue_(outbound_queue),
    inbound_queue_(inbound_queue),
    check_queue_timer_(io_context) {

    std::cout << "TCP Server starting on port " << port << "...\n";
    do_accept();
    process_outbound_queue();
}

void Server::broadcast_bytes(const std::vector<uint8_t>& raw_bytes) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& client : active_clients_) {
        client->send_binary_data(raw_bytes);
    }
}

void Server::push_to_inbound_queue(int value) {
    inbound_queue_.push(value);
}

void Server::register_client(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    active_clients_.push_back(client);
}

void Server::unregister_client(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    active_clients_.erase(
        std::remove(active_clients_.begin(), active_clients_.end(), client),
        active_clients_.end());
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto new_client = std::make_shared<ClientSession>(std::move(socket), *this);
                register_client(new_client);
                new_client->start();
            }
            do_accept();
        });
}

void Server::process_outbound_queue() {
    int outgoing_integer = 0;
    while (outbound_queue_.try_pop(outgoing_integer)) {
        std::vector<uint8_t> byte_packet(sizeof(int));
        std::memcpy(byte_packet.data(), &outgoing_integer, sizeof(int));
        broadcast_bytes(byte_packet);
    }


    check_queue_timer_.expires_after(std::chrono::milliseconds(50));
    check_queue_timer_.async_wait([this](std::error_code ec) {
        if (!ec) {
            process_outbound_queue();
        }
        });
}
