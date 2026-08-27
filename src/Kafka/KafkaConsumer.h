#pragma once
#include <librdkafka/rdkafkacpp.h>
#include <functional>
#include <vector>

class KafkaConsumer
{
public:
	using MessageCallback = std::function<void(const std::string& topic, const std::string& message)>;

	KafkaConsumer(const std::string& brokers, const std::string& group_id);
	~KafkaConsumer();

	bool Subscribe(const std::string& topic);
	bool Subscribe(const std::vector<std::string>& topics);
	void SetCallback(MessageCallback callback);
	void Run();
	void Stop();

private:
	std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
	std::unique_ptr<RdKafka::Conf> conf_;
	MessageCallback callback_;
	bool running_ = false;
	std::vector<std::string> topics_;
};