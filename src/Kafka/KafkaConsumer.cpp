#include "../stdafx.h"

#include "KafkaConsumer.h"


KafkaConsumer::KafkaConsumer(const std::string& brokers, const std::string& group_id)
{
	std::string errstr;
	conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
	conf_->set("bootstrap.servers", brokers, errstr);
	conf_->set("group.id", group_id, errstr);
	conf_->set("enable.auto.commit", "true", errstr);
	conf_->set("auto.offset.reset", "earliest", errstr);

	consumer_.reset(RdKafka::KafkaConsumer::create(conf_.get(), errstr));
}

KafkaConsumer::~KafkaConsumer()
{
	Stop();
}

bool KafkaConsumer::Subscribe(const std::string& topic)
{
	return Subscribe(std::vector<std::string>{topic});
}

bool KafkaConsumer::Subscribe(const std::vector<std::string>& topics)
{
	topics_ = topics;
	consumer_->subscribe(topics_);
	std::cout << "Subscribed to topics: ";
	for (const auto& t : topics_)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	return true;
}

void KafkaConsumer::SetCallback(MessageCallback callback)
{
	callback_ = std::move(callback);
}

void KafkaConsumer::Run()
{
	running_ = true;

	while (running_)
	{
		auto msg = consumer_->consume(1000);

		if (msg->err() == RdKafka::ERR_NO_ERROR)
		{
			std::string message((const char*)msg->payload(), msg->len());
			std::string topic = msg->topic_name();

			if (callback_)
			{
				callback_(topic, message);
			}
		}
		else if (msg->err() != RdKafka::ERR__TIMED_OUT)
		{
			std::cerr << "Kafka error: " << msg->errstr() << std::endl;
		}

		delete msg;
	}
}

void KafkaConsumer::Stop()
{
	running_ = false;
	if (consumer_)
	{
		consumer_->close();
	}
}