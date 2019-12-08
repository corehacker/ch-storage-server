/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2019, Sandeep Prakash
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
 * Copyright (c) 2019, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   firebase-client.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Dec 07, 2019
 *
 * \brief
 *
 ******************************************************************************/

#include <sstream>
#include <glog/logging.h>
#include "firebase-client.hpp"

#include <ch-cpp-utils/http/client/http.hpp>

using std::ostringstream;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpResponse;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

namespace SS {


FirebaseClient::FirebaseClient(Config *config) {
	LOG(INFO) << "*****************FirebaseClient";
	mConfig = config;
}

FirebaseClient::~FirebaseClient() {
	LOG(INFO) << "*****************~FirebaseClient";
}

void FirebaseClient::_onLoad(HttpRequestLoadEvent *event, void *this_) {
	FirebaseClient *obj = (FirebaseClient *) this_;
  obj->onLoad(event);
}

void FirebaseClient::onLoad(HttpRequestLoadEvent *event) {
  HttpResponse *response = event->getResponse();
  char *body = nullptr;
  uint32_t length = 0;
  if(response->getResponseCode() < 200 || response->getResponseCode() > 209) {
    LOG(INFO) << "Invalid response. Will try later: " << response->getResponseCode();
		LOG(INFO) << "Invalid response. Will try later: " << response->getResponseText();
    return;
  }
  response->getResponseBody((uint8_t **) &body, &length);
  string jsonBody(body, length);
  LOG(INFO) << "Response Body: \"" << jsonBody << "\"";
  if(body) free(body);

  LOG(INFO) << "Request Complete: " << response->getResponseCode();
}

bool FirebaseClient::init() {
	return true;
}

void FirebaseClient::send(json &message, string &target) {
	HttpRequest *request = new HttpRequest();
	request->onLoad(FirebaseClient::_onLoad).bind(this);

	string port = "80";
	if(mConfig->getNotFirebaseProtocol() == "http") {
		port = "80";
	} else if(mConfig->getNotFirebaseProtocol() == "https") {
		port = "443";
	}

	ostringstream fullUrl;
	fullUrl << mConfig->getNotFirebaseProtocol() << "://" << mConfig->getNotFirebaseHostname() << ":" << port << mConfig->getNotFirebaseUrl() << "/" << target;

	ostringstream authHeader;
	authHeader << "key=" << mConfig->getNotFirebaseAuthKey();

	json payload;

	/*
	{
  "message": {
    "topic" : "security",
    "data": {
      "body": "Notification from cURL",
      "title": "Notification from cURL"
    },
    "time_to_live": 100
  }
	*/
	payload["message"]["topic"] = mConfig->getNotFirebaseTopic();
	payload["message"]["data"]["body"] = "Security Alert";
	payload["message"]["data"]["title"] = "Security Alert";
	payload["message"]["time_to_live"] = mConfig->getNotFirebaseTtl();
	string payloadString = payload.dump();

	LOG(INFO) << "Payload: " << payloadString;

	request->open(EVHTTP_REQ_POST, fullUrl.str())
		.setHeader("Host", mConfig->getNotFirebaseHostname())
		.setHeader("User-Agent", "curl/7.58.0")
		.setHeader("Accept", "*/*")
		.setHeader("Authorization", authHeader.str())
		.setHeader("ttl", mConfig->getNotFirebaseTtl())
		.setHeader("Content-Type", mConfig->getNotFirebaseContentType())
		.send((void *) payloadString.c_str(), payloadString.size());
}

void FirebaseClient::send(json &message) {
	LOG(INFO) << "FirebaseClient: Sending notifications.";
	for(auto target : mConfig->getNotFirebaseTargets()) {
		send(message, target);
	}

}

} // End namespace SS.

// #include "config.hpp"

// using SS::Config;
// using SS::FirebaseClient;

// static Config *config = nullptr;

// int main() {
// 	config = new Config();
// 	config->init();

// 	FirebaseClient *mFirebaseClient;
// 	mFirebaseClient = new FirebaseClient(config);
// 	mFirebaseClient->init();

// 	json message;
// 	message["empty"] = "empty";
// 	mFirebaseClient->send(message);

// 	THREAD_SLEEP_FOREVER;

// }
