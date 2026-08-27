#pragma once

#include "WebSocketSession.h"

#include "SessionManager.h"

class WebSocketSessionImpl :
	public WebSocketSession,
	public std::enable_shared_from_this<WebSocketSessionImpl>
{
public:
	WebSocketSessionImpl(websocket::stream<beast::tcp_stream>& stream,
		const std::string& session_id)
		: ws_(stream)
		, session_id_(session_id)
		, is_open_(true) {}

	void SendMessageToSocket(const std::string& message) override
	{
		if (!is_open_) return;

		ws_.text(true);
		beast::error_code ec;
		ws_.write(net::buffer(message), ec);
		if (ec) {
			std::cerr << "Send error: " << ec.message() << std::endl;
			is_open_ = false;
		}
	}

	std::string GetSessionId() const override { return session_id_; }
	bool IsOpen() const override { return is_open_ && ws_.is_open(); }

	void Close() override
	{
		if (is_open_) {
			is_open_ = false;
			beast::error_code ec;
			ws_.close(websocket::close_code::normal, ec);
		}
	}
	void Run() override {}

private:
	websocket::stream<beast::tcp_stream>& ws_;
	std::string session_id_;
	bool is_open_;
};