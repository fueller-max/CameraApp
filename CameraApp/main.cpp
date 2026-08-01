
#include "tcp_server.h"
#include "thread_safe_queue.h"
#include <asio.hpp>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io_context;


        ThreadSafeQueue<int> outbound_pipeline; //  server  -> clients
        ThreadSafeQueue<int> inbound_pipeline;  //  clients -> server 

        Server server(io_context, 2030, outbound_pipeline, inbound_pipeline);

        // --- OUTBOUND PRODUCER THREAD ---
        // Simulates your outside source generating integers to broadcast
        int external_counter = 1000;
        std::thread outbound_producer([&outbound_pipeline, &external_counter]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                external_counter++;
                outbound_pipeline.push(external_counter);
            }
            });
        outbound_producer.detach();

        // --- INBOUND CONSUMER THREAD ---
        //  Messages from clients
        std::thread inbound_consumer([&inbound_pipeline]() {
            int received_integer = 0;
            while (true) {
                // Poll the queue every 50ms for client data
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                while (inbound_pipeline.try_pop(received_integer)) {
                    std::cout << "[Main Thread Logic] Displaying data from pipeline: "
                        << received_integer << std::endl;
                }
            }
            });
        inbound_consumer.detach();

        std::cout << "Server active. Monitoring both pipelines..." << std::endl;
        io_context.run();

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}