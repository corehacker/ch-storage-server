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
 * \file   kafka-client.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Apr 23, 2018
 *
 * \brief
 *
 ******************************************************************************/
#include <sstream>
#include <glog/logging.h>
#include "kafka-client.hpp"

using std::ostringstream;
using RdKafka::ErrorCode;

namespace SS {

void KafkaEventCb::event_cb (Event &event) {
	switch (event.type())
	{
		case Event::EVENT_ERROR:
			LOG(ERROR) << "ERROR (" << RdKafka::err2str(event.err()) << "): " << event.str();
			break;

		case Event::EVENT_STATS:
			LOG(INFO) << "\"STATS\": " << event.str();
			break;

		case Event::EVENT_LOG:
			LOG(INFO) << "LOG-" << event.severity() << "-" << event.fac().c_str() <<
				": " << event.str().c_str();
			break;

		default:
			LOG(INFO) << "EVENT " << event.type() <<
					" (" << RdKafka::err2str(event.err()) << "): " << event.str();
			break;
	}
}

void KafkaDeliveryReportCb::dr_cb (Message &message) {
	LOG(INFO) << "Message delivery for (" << message.len() << " bytes): " <<
			message.errstr();
	if (message.key())
		LOG(INFO) << "Key: " << *(message.key()) << ";";
}

KafkaClient::KafkaClient(Config *config) {
	LOG(INFO) << "*****************KafkaClient";
	mConfig = config;

	std::string errstr;
	mConf = Conf::create(RdKafka::Conf::CONF_GLOBAL);
	mConf->set("event_cb", &mEventCb, errstr);
	mConf->set("dr_cb", &mDeliveryCb, errstr);
	mConf->set("metadata.broker.list", mConfig->getNotKafkaConnection(), errstr);

	mTopicConf = Conf::create(RdKafka::Conf::CONF_TOPIC);

	mProducer = nullptr;
	mTopic = nullptr;
}

KafkaClient::~KafkaClient() {
	LOG(INFO) << "*****************~KafkaClient";
	delete mTopic;
	delete mProducer;
}

bool KafkaClient::init() {
	std::string errstr;
	mProducer = Producer::create(mConf, errstr);
	if (!mProducer) {
		LOG(ERROR) << "Failed to create producer: " << errstr;
		return false;
	}
	mTopic = Topic::create(mProducer, mConfig->getNotKafkaTopic(), mTopicConf,
			errstr);
	if (!mTopic) {
		LOG(ERROR) << "Failed to create topic: " << errstr;
		return false;
	}
	return true;
}

void KafkaClient::send(json &message) {
	ostringstream os;
	os << message;
	string jsonString = os.str();
	const char *msg = jsonString.c_str();
	size_t length = jsonString.length();
    RdKafka::ErrorCode resp =
    		mProducer->produce(mTopic, mConfig->getNotKafkaPartition(),
			  RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
			  (void *) msg, length, NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR)
    	LOG(ERROR) << "% Produce failed: " << RdKafka::err2str(resp);
    else
    	LOG(INFO) << "% Produced message (" << length << " bytes)";
    mProducer->poll(0);
}

} // End namespace SS.
