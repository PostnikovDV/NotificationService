#pragma once

#include <memory>

class WebSocketSession;

class SessionManager
{
public:
	SessionManager() = default;
	~SessionManager() = default;

	void AddSession(const std::string& session_id, std::shared_ptr<WebSocketSession> session);
	void RemoveSession(const std::string& session_id);
	void SendToSession(const std::string& session_id, const std::string& message);
	void Broadcast(const std::string& message);
	void ProcessPendingMessages(const std::string& session_id);
	void CleanExpiredMessages();

private:
	struct PendingMessage {
		std::string message;
		std::chrono::steady_clock::time_point timestamp;
	};

	std::unordered_map<std::string, std::shared_ptr<WebSocketSession>> sessions_;
	std::unordered_map<std::string, std::vector<PendingMessage>> pending_messages_;
	mutable std::mutex mutex_;

	static constexpr int MAX_PENDING_MESSAGES = 100;
	static constexpr auto MESSAGE_EXPIRY = std::chrono::hours(24);
};