#include "../stdafx.h"

#include "NotificationServer.h"

#include "../Session/WebSocketSessionImpl.h"

net::awaitable<void> do_listen(
	net::ip::address address,
	unsigned short port,
	std::shared_ptr<SessionManager> session_manager
)
{
	auto executor = co_await net::this_coro::executor;
	auto acceptor = net::ip::tcp::acceptor{ executor, {address, port} };

	std::cout << "Listening on " << address << ":" << port << std::endl;

	for (;;)
	{
		auto socket = co_await acceptor.async_accept(executor);
		std::cout << "New client connected" << std::endl;

		auto strand = net::make_strand(executor);

		beast::tcp_stream tcp_stream(std::move(socket));

		websocket::stream<beast::tcp_stream> ws_stream(std::move(tcp_stream));

		net::co_spawn(
			strand,
			do_session(std::move(ws_stream), session_manager, strand),
			[](std::exception_ptr e)
			{
				if (e)
				{
					try
					{
						std::rethrow_exception(e);
					}
					catch (std::exception& e)
					{
						std::cerr << "Error in session: " << e.what() << "\n";
					}
				}
			}
		);
	}
}

net::awaitable<void> do_session(
	websocket::stream<beast::tcp_stream> stream,
	std::shared_ptr<SessionManager> session_manager,
	net::strand<net::any_io_executor> strand
)
{
	try
	{
		stream.set_option(
			websocket::stream_base::timeout::suggested(beast::role_type::server)
		);

		stream.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res) {
				res.set(http::field::server, "NotificationService");
			}
		));

		co_await stream.async_accept();

		beast::flat_buffer buffer;


		auto [ec, _] = co_await stream.async_read(buffer, net::as_tuple);

		if (ec)
		{
			std::cerr << "Error reading auth message: " << ec.message() << std::endl;
			co_return;
		}

		auto data = beast::buffers_to_string(buffer.data());
		buffer.consume(buffer.size());
		auto json = nlohmann::json::parse(data);

		if (!json.contains("session_id"))
		{
			std::cerr << "No session_id in first message" << std::endl;
			stream.close(websocket::close_code::policy_error);
			co_return;
		}

		std::string session_id = json["session_id"].get<std::string>();
		std::cout << "Client authenticated: " << session_id << std::endl;

		nlohmann::json auth_response
		{
			{ "type", "auth_success" },
			{ "session_id", session_id }
		};
		stream.text(true);
		co_await stream.async_write(net::buffer(auth_response.dump()));


		auto session_wrapper = std::make_shared<WebSocketSessionImpl>(
			stream, session_id, strand
		);
		session_manager->AddSession(session_id, session_wrapper);

		for (;;)
		{
			auto [read_ec, _] = co_await stream.async_read(buffer, net::as_tuple);

			if (read_ec == websocket::error::closed)
			{
				std::cout << "Client disconnected: " << session_id << std::endl;
				session_manager->RemoveSession(session_id);
				co_return;
			}

			if (read_ec)
			{
				std::cerr << "Read error: " << read_ec.message() << std::endl;
				session_manager->RemoveSession(session_id);
				co_return;
			}

			auto msg = beast::buffers_to_string(buffer.data());
			buffer.consume(buffer.size());
			std::cout << "Received from " << session_id << ": " << msg << std::endl;

			stream.text(stream.got_text());
			co_await stream.async_write(net::buffer(msg));
		}

	}
	catch (const std::exception& e)
	{
		std::cerr << "Session error: " << e.what() << std::endl;
		co_return;
	}
}
