#pragma once

#include "WebSocketSession.h"

#include "SessionManager.h"

class WebSocketSessionImpl :
	public WebSocketSession,
	public std::enable_shared_from_this<WebSocketSessionImpl>
{
public:
	WebSocketSessionImpl(
		websocket::stream<beast::tcp_stream>& stream,
		const std::string& session_id,
		net::strand<net::any_io_executor> strand)
		: ws_(stream)
		, session_id_(session_id)
		, strand_(strand)
	{}

	void SendMessageToSocket(const std::string& message) override
	{
		if (!is_open_)
			return;

		auto self = shared_from_this();

		net::dispatch(strand_, [self, message]() {
			if (!self->is_open_) return;

			self->message_queue_.push(message);

			if (!self->is_writing_) {
				self->WriteNext();
			}
			});
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
	void WriteNext()
	{
		if (!is_open_ || message_queue_.empty())
		{
			is_writing_ = false;
			return;
		}

		is_writing_ = true;

		std::string message = std::move(message_queue_.front());
		message_queue_.pop();

		ws_.text(true);
		auto self = shared_from_this();

		ws_.async_write(
			net::buffer(message),
			[self](beast::error_code ec, std::size_t bytes) {
				self->is_writing_ = false;

				if (ec)
				{
					std::cerr << "Send error: " << ec.message() << std::endl;
					self->is_open_ = false;
					return;
				}

				self->WriteNext();
			}
		);
	}

private:
	websocket::stream<beast::tcp_stream>& ws_;
	std::string session_id_;
	net::strand<net::any_io_executor> strand_;
	bool is_open_{ true };
	bool is_writing_{ false };
	std::queue<std::string> message_queue_;
};