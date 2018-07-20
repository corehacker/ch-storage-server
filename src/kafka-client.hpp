/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2018, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2018, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   kafka-client.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Apr 23, 2018
 *
 * \brief
 *
 ******************************************************************************/

#include <librdkafka/rdkafkacpp.h>
#include <ch-cpp-utils/third-party/json/json.hpp>

#include "config.hpp"

using RdKafka::Conf;
using RdKafka::Producer;
using RdKafka::Topic;
using RdKafka::EventCb;
using RdKafka::Event;
using RdKafka::DeliveryReportCb;
using RdKafka::Message;

using json = nlohmann::json;

#ifndef SRC_KAFKA_CLIENT_HPP_
#define SRC_KAFKA_CLIENT_HPP_

namespace SS {

class KafkaEventCb : public EventCb {
public:
  void event_cb(Event &event);
};

class KafkaDeliveryReportCb : public DeliveryReportCb {
 public:
  void dr_cb (Message &message);
};

class KafkaClient {
private:
	Config *mConfig;

	Conf *mConf;
	Conf *mTopicConf;
	Producer *mProducer;
	Topic *mTopic;
	KafkaEventCb mEventCb;
	KafkaDeliveryReportCb mDeliveryCb;
public:
	KafkaClient(Config *config);
	~KafkaClient();
	bool init();
	void send(json &message);
};

} // End namespace SS.

#endif /* SRC_KAFKA_CLIENT_HPP_ */
