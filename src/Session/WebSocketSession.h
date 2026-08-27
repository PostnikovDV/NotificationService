#pragma once

class WebSocketSession
{
public:
	virtual ~WebSocketSession() = default;

	virtual void SendMessageToSocket(const std::string& message) = 0;
	virtual std::string GetSessionId() const = 0;
	virtual bool IsOpen() const = 0;
	virtual void Close() = 0;
	virtual void Run() = 0;
};