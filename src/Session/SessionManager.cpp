#pragma once

#include "../stdafx.h"

#include "SessionManager.h"

#include "WebSocketSession.h"


void SessionManager::AddSession(const std::string& session_id, std::shared_ptr<WebSocketSession> session)
{
	std::lock_guard<std::mutex> lock(mutex_);
	sessions_[session_id] = session;
	std::cout << "Session added: " << session_id << std::endl;
	ProcessPendingMessages(session_id);
}

void SessionManager::RemoveSession(const std::string& session_id)
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = sessions_.find(session_id);
	if (it != sessions_.end())
	{
		it->second->Close();
		sessions_.erase(it);
		std::cout << "Session removed: " << session_id << std::endl;
	}
}

void SessionManager::SendToSession(const std::string& session_id, const std::string& message)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = sessions_.find(session_id);
	if (it != sessions_.end() && it->second->IsOpen())
	{
		it->second->SendMessageToSocket(message);
		std::cout << "Message sent to: " << session_id << std::endl;
	}
	else
	{
		PendingMessage pending{ message, std::chrono::steady_clock::now() };

		if (pending_messages_[session_id].size() >= MAX_PENDING_MESSAGES)
		{
			pending_messages_[session_id].erase(pending_messages_[session_id].begin());
		}

		pending_messages_[session_id].push_back(pending);
		std::cout << "Message queued for: " << session_id << " (size: " << pending_messages_[session_id].size() << ")" << std::endl;
	}
}

void SessionManager::Broadcast(const std::string& message)
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto& [session_id, session] : sessions_)
	{
		if (session->IsOpen())
		{
			session->SendMessageToSocket(message);
		}
	}
	std::cout << "Broadcast sent to " << sessions_.size() << " sessions" << std::endl;
}

void SessionManager::ProcessPendingMessages(const std::string& session_id)
{
	auto pending_it = pending_messages_.find(session_id);
	if (pending_it == pending_messages_.end() || pending_it->second.empty())
	{
		return;
	}

	auto session_it = sessions_.find(session_id);
	if (session_it == sessions_.end() || !session_it->second->IsOpen())
	{
		return;
	}

	std::cout << "Processing " << pending_it->second.size() << " pending messages for: " << session_id << std::endl;

	for (const auto& pending : pending_it->second) {
		session_it->second->SendMessageToSocket(pending.message);
	}
	pending_messages_.erase(pending_it);
}

void SessionManager::CleanExpiredMessages()
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto now = std::chrono::steady_clock::now();

	for (auto& [session_id, messages] : pending_messages_)
	{
		auto it = std::remove_if(messages.begin(), messages.end(),
			[&](const PendingMessage& msg)
			{
				return msg.timestamp + MESSAGE_EXPIRY < now;
			}
		);
		messages.erase(it, messages.end());
	}

	for (auto it = pending_messages_.begin(); it != pending_messages_.end();)
	{
		if (it->second.empty())
		{
			it = pending_messages_.erase(it);
		}
		else
		{
			++it;
		}
	}
}