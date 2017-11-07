/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2017, Sandeep Prakash
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
 * Copyright (c) 2017, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   storage-server.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

#include <glog/logging.h>
#include <ch-cpp-utils/utils.hpp>
#include "storage-server.hpp"

using std::ifstream;
using ChCppUtils::mkPath;

namespace SS {

Config::Config() {
	etcConfigPath = "/etc/ch-storage-server/ch-storage-server.json";
	localConfigPath = "./config/ch-storage-server.json";

	mPort = 0;
	mRoot = "";
}

Config::~Config() {
	LOG(INFO) << "*****************~Config";
}

static bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

bool Config::getConfigFile() {
	string selected = "";
	ifstream config(etcConfigPath);
	if(!fileExists(etcConfigPath)) {
		if(!fileExists(localConfigPath)) {
			LOG(ERROR) << "No config file found in /etc/ch-storage-server or " <<
					"./config. I am looking for ch-storage-server.json";
			return false;
		} else {
			LOG(INFO) << "Found config file "
					"./config/ch-storage-server.json";
			selectedConfigPath = localConfigPath;
			return true;
		}
	} else {
		LOG(INFO) << "Found config file "
				"/etc/ch-storage-server/ch-storage-server.json";
		selectedConfigPath = etcConfigPath;
		return true;
	}
}

bool Config::validateConfigFile() {
	mPort = mJson["server"]["port"];
	LOG(INFO) << "server.port : " << mPort;

	mRoot = mJson["storage"]["root"];
	LOG(INFO) << "storage.root: " << mRoot;
	return true;
}

void Config::init() {
	if(!getConfigFile()) {
		LOG(ERROR) << "Invalid config file.";
		std::terminate();
	}
	ifstream config(selectedConfigPath);
	config >> mJson;
	if(!validateConfigFile()) {
		LOG(ERROR) << "Invalid config file.";
		std::terminate();
	}
	LOG(INFO) << "Config: " << mJson;
}

uint16_t Config::getPort() {
	return mPort;
}

string &Config::getRoot() {
	return mRoot;
}

StorageServer::StorageServer(Config *config) {
	mConfig = config;
	if(mkPath(mConfig->getRoot(), 0744)) {
		LOG(INFO) << "Created root directory: " << mConfig->getRoot();
	} else {
		LOG(ERROR) << "Error root creating directory: " << mConfig->getRoot();
	}
	mTimer = new Timer();
	struct timeval tv = {0};
	tv.tv_sec = 5;
	mTimerEvent = mTimer->create(&tv, StorageServer::_onTimerEvent, this);
	mServer = new HttpServer(mConfig->getPort(), 2);
}

StorageServer::~StorageServer() {
	LOG(INFO) << "*****************~StorageServer";
	delete mServer;
	LOG(INFO) << "Deleted server.";

	mTimer->destroy(mTimerEvent);

	delete mTimer;
}

void StorageServer::_onRequest(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onRequest(event);
}

void StorageServer::onRequest(RequestEvent *event) {
	LOG(INFO) << "POST Request!!";

	if(event->hasBody()) {
		void *body = event->getBody();
		LOG(INFO) << "Body: " << event->getLength() << " bytes, content: " <<
				(char *) body;
		free(body);
	} else {
		LOG(INFO) << "Empty body";
	}

}

void StorageServer::_onTimerEvent(TimerEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onTimerEvent(event);
}

void StorageServer::onTimerEvent(TimerEvent *event) {
	LOG(INFO) << "Timer fired!!";

	mTimer->restart(event);
}

void StorageServer::registerPaths() {
	auto streams = mConfig->mJson["streams"];
	for(auto stream : streams) {
		LOG(INFO) << "Stream: " << stream;
		mServer->route(EVHTTP_REQ_POST, stream["path"],
				StorageServer::_onRequest, this);
	}
}

void StorageServer::start() {
	LOG(INFO) << "Starting server";
	registerPaths();
//	mExitSem.wait();
}

void StorageServer::stop() {
	mExitSem.notify();
	LOG(INFO) << "Stopping server";
}

} // End namespace SS.
