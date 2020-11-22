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
#include <fstream>
#include <glog/logging.h>
#include "firebase-client.hpp"

#include <ch-cpp-utils/http/client/http.hpp>
#include <ch-cpp-utils/utils.hpp>

using std::ostringstream;
using std::ifstream;
using std::ofstream;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpResponse;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

using ChCppUtils::getEpochNano;
using ChCppUtils::getDateTime;
using ChCppUtils::fileExists;

namespace SS {


FirebaseClient::FirebaseClient(Config *config) {
	LOG(INFO) << "*****************FirebaseClient";
	mConfig = config;

	mTimer = new Timer();
	mTimerEvent = nullptr;

	mLastSentNs = 0;
	mIntervalNs = mConfig->getNotFirebaseIntervalSeconds() * 1000 * 1000 * 1000;

	etcTargetsPath = "/etc/ch-storage-server/ch-firebase-targets.json";
	localTargetsPath = "./config/ch-firebase-targets.json";
}

FirebaseClient::~FirebaseClient() {
	LOG(INFO) << "*****************~FirebaseClient";

	if(mTimerEvent != nullptr) {
		delete mTimerEvent;
	}

	delete mTimer;
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

void FirebaseClient::_onTimerEvent(TimerEvent *event, void *this_) {
  FirebaseClient *server = (FirebaseClient *) this_;
  server->onTimerEvent(event);
}

void FirebaseClient::onTimerEvent(TimerEvent *event) {
	LOG(INFO) << "Timer fired! Sending firebase notifications.";

	sendTargets();

	delete mTimerEvent;
	mTimerEvent = nullptr;
}

bool FirebaseClient::init() {
	return true;
}

void FirebaseClient::send(string &target) {
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

vector<string> FirebaseClient::getFirebaseTargets() {
	string targetsFile = mConfig->getNotFirebaseTargetsJson();

	LOG(INFO) << "Firebase targets file: " << targetsFile;

	ifstream targetsFileStream(targetsFile);
	json targetsJson;
	targetsFileStream >> targetsJson;

	json targetsMap = targetsJson["devices"];

	vector<string> v;
	for (auto it = targetsMap.begin(); it != targetsMap.end(); ++it) {
		// std::cout << it.key() << "\n";
		// std::cout << it.value() << "\n";
		LOG(INFO) << "Target: " << it.key() << " -> " << it.value();
		v.push_back(it.value());
	}
	return v;
}

void FirebaseClient::sendTargets() {
	mLastSentNs = getEpochNano();
	for(auto target : getFirebaseTargets()) {
		send(target);
	}
}

void FirebaseClient::send(json &message) {

	uint64_t currentNs = getEpochNano();
	uint64_t elapsedNs = currentNs - mLastSentNs;
	LOG(INFO) << "Current: " << currentNs << ", Last: " << mLastSentNs <<
			", elapsed: " << elapsedNs << ", Interval: " << mIntervalNs;

	if(elapsedNs >= mIntervalNs) {
		LOG(INFO) << "FirebaseClient: Interval expired. Sending notifications.";
		sendTargets();
	} else if (mTimerEvent != nullptr) {
		LOG(INFO) << "FirebaseClient: Interval not expired. But a timer is already been created.";
	} else {
		LOG(INFO) << "FirebaseClient: Interval not expired. Creating a timer.";
		struct timeval tv = {0};
		tv.tv_sec = mConfig->getNotFirebaseIntervalSeconds();
		mTimerEvent = mTimer->create(&tv, FirebaseClient::_onTimerEvent, this);
	}
}

bool FirebaseClient::selectConfigFile() {
	if(!fileExists(etcTargetsPath)) {
		if(!fileExists(localTargetsPath)) {
			LOG(ERROR) << "No config file found in /etc/ch-storage-client or " <<
					"./config. I am looking for ch-firebase-targets.json";
			return false;
		} else {
			LOG(INFO) << "Found config file "
					"./config/ch-firebase-targets.json";
			selectedConfigPath = localTargetsPath;
			return true;
		}
	} else {
		LOG(INFO) << "Found config file "
				"/etc/ch-storage-client/ch-firebase-targets.json";
		selectedConfigPath = etcTargetsPath;
		return true;
	}
}

bool FirebaseClient::addTarget(string &target) {

	if(selectConfigFile()) {

		ifstream config(selectedConfigPath);
		config >> mTargetsJson;

		json newTarget;

		newTarget = json::parse(target);

		string instanceId = newTarget.value("instanceId", "");
		string deviceId = newTarget.value("deviceId", "");
		string deviceName = newTarget.value("deviceName", "");

		if(instanceId.length() > 0 && deviceId.length() > 0 && deviceName.length() > 0) {
			mTargetsJson[deviceId] = newTarget;

			ofstream newConfig(selectedConfigPath);

			std::cout << std::setw(4) << mTargetsJson << std::endl;

			newConfig << std::setw(4) << mTargetsJson << std::endl;

			newConfig.close();

			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}

	
}

string FirebaseClient::getTargets() {
	if(selectConfigFile()) {
		ifstream config(selectedConfigPath);
		config >> mTargetsJson;
		return mTargetsJson.dump();
	} else {
		return "{}";
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

// 	std::cout << mFirebaseClient->getTargets() << std::endl;

// 	string target = "{\"instanceId\": \"instance-id\",\"deviceId\":\"device-id\", \"deviceName\": \"device-name\"}";
// 	mFirebaseClient->addTarget(target);

// 	std::cout << mFirebaseClient->getTargets() << std::endl;

// 	// json message;
// 	// message["empty"] = "empty";
// 	// mFirebaseClient->send(message);

// 	// THREAD_SLEEP_5S;

// 	// mFirebaseClient->send(message);

// 	// THREAD_SLEEP_30S;
// 	// THREAD_SLEEP_30S;
// 	// THREAD_SLEEP_5S;

// 	// mFirebaseClient->send(message);

// 	THREAD_SLEEP_FOREVER;/firebase/target/device/register