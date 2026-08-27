#pragma once

#include "../Session/SessionManager.h"
#include "../Kafka/KafkaConsumer.h"

net::awaitable<void> do_listen(
	net::ip::address address,
	unsigned short port,
	std::shared_ptr<SessionManager> session_manager
);

net::awaitable<void> do_session(
	websocket::stream<beast::tcp_stream> stream,
	std::shared_ptr<SessionManager> session_manager
);