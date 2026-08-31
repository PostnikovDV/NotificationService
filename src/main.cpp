#include "stdafx.h"
#include "Server/NotificationServer.h"


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 4)
		{
			std::cerr << "Usage: notification_service <address> <port> <threads>\n";
			std::cerr << "Example: notification_service 0.0.0.0 8083 4\n";
			return EXIT_FAILURE;
		}

		auto const address = net::ip::make_address(argv[1]);
		auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
		auto const threads = std::max<int>(1, std::atoi(argv[3]));

		std::cout << "Starting NotificationService on " << address
			<< ":" << port << " with " << threads << " threads\n";

		net::io_context ioc(threads);

		auto session_manager = std::make_shared<SessionManager>();

		net::co_spawn(
			ioc,
			do_listen(address, port, session_manager),
			[](std::exception_ptr e)
			{
				if (e)
				{
					try
					{
						std::rethrow_exception(e);
					}
					catch (std::exception const& e)
					{
						std::cerr << "Error: " << e.what() << std::endl;
					}
				}
			}
		);

		std::string kafka_brokers = "localhost:9092";
		std::thread kafka_thread([&ioc, session_manager, kafka_brokers]() {
			KafkaConsumer consumer(kafka_brokers, "notification-group");
			consumer.Subscribe("order_notifications");
			consumer.Subscribe("payment_notifications");

			consumer.SetCallback([&ioc, session_manager](const std::string& topic,
				const std::string& message) {
					net::post(ioc, [session_manager, message]() {
						try {
							auto json = nlohmann::json::parse(message);
							if (json.contains("session_id")) {
								std::string session_id = json["session_id"].get<std::string>();
								session_manager->SendToSession(session_id, message);
							}
							else {
								session_manager->Broadcast(message);
							}
						}
						catch (const std::exception& e) {
							std::cerr << "Kafka message error: " << e.what() << std::endl;
						}
						});
				});

			consumer.Run();
			});
		kafka_thread.detach();

		std::vector<std::thread> thread_pool;
		thread_pool.reserve(threads - 1);

		for (int i = 1; i < threads; ++i)
		{
			thread_pool.emplace_back([&ioc]() { ioc.run(); });
		}

		ioc.run();

		for (auto& thread : thread_pool)
		{
			if (thread.joinable()) {
				thread.join();
			}
		}

		std::cout << "NotificationService stopped\n";
		return EXIT_SUCCESS;

	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}