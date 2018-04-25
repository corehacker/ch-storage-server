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
 * \file   mail-client.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Mar 14, 2018
 *
 * \brief
 *
 ******************************************************************************/
#include <glog/logging.h>
#include <ch-cpp-utils/utils.hpp>

#include "mail-client.hpp"
#include "curl-smtp.hpp"

using ChCppUtils::getEpochNano;
using ChCppUtils::getDateTime;

namespace SS {

MailJob::MailJob(MailClient *client, Config *config, string message) {
	mClient = client;
	mConfig = config;
	mMessage = message;
}

MailClient::MailClient(Config *config) {
	mConfig = config;
	mLastSentNs = 0;
	mAggregateNs = mConfig->getNotEmailAggregate() * 1000 * 1000 * 1000;
	mPool = new ThreadPool(mConfig->getNotEmailThreadCount(), false);
}

MailClient::~MailClient() {
	LOG(INFO) << "*****************~MailClient";
	delete mPool;
}

void *MailClient::_sendRoutine(void *arg, struct event_base *base) {
	MailJob *mailJob = (MailJob *) arg;
	return mailJob->mClient->sendRoutine(mailJob);
}

void *MailClient::sendRoutine(MailJob *data) {
	CurlSmtp curlSmtp(mConfig);
	curlSmtp.send(data->mMessage);
	delete data;
	return NULL;
}

void MailClient::notify(string &message) {
	MailJob *mailJob = new MailJob(this, mConfig, message);
	ThreadJob *job = new ThreadJob(MailClient::_sendRoutine, mailJob);
	mPool->addJob(job);
}

void MailClient::notifyMotionDetection(string &message) {
	uint64_t currentNs = getEpochNano();
	uint64_t elapsedNs = currentNs - mLastSentNs;
	LOG(INFO) << "Current: " << currentNs << ", Last: " << mLastSentNs <<
			", elapsed: " << elapsedNs << ", Aggregate: " << mAggregateNs;
	if(elapsedNs >= mAggregateNs) {
		mMDNotEmailBody += "Motion Detection Event | " + getDateTime() +
						" | " + message + "\r\n";
		LOG(INFO) << "Send...";
		LOG(INFO) << mMDNotEmailBody;
		notify(mMDNotEmailBody);
		mMDNotEmailBody = "";
		mLastSentNs = currentNs;
	} else {

		LOG(INFO) << "Motion Detection Event | " << getDateTime() << " | " << message;

		mMDNotEmailBody += "Motion Detection Event | " + getDateTime() +
				" | " + message + "\r\n";

		LOG(INFO) << "Not Sending...";
	}
}

}  // End namespace SS.
