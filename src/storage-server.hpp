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
 * \file   storage-server.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/

#include "../third-party/json/json.hpp"
#include <ch-cpp-utils/semaphore.hpp>
#include <ch-cpp-utils/http-server.hpp>
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/utils.hpp>

#ifndef SRC_STORAGE_SERVER_HPP_
#define SRC_STORAGE_SERVER_HPP_

using namespace std::chrono;

using json = nlohmann::json;
using ChCppUtils::Semaphore;
using ChCppUtils::Http::Server::RequestEvent;
using ChCppUtils::Http::Server::HttpServer;
using ChCppUtils::Timer;
using ChCppUtils::TimerEvent;

namespace SS {

class Config {
public:
	json mJson;

	Config();
	~Config();
	void init();

	uint16_t getPort();
	string &getRoot();
	uint32_t getPurgeTtlSec();
	uint32_t getPurgeIntervalSec();

private:
	uint16_t mPort;
	string mRoot;

	uint32_t mPurgeTtlSec;
	system_clock::time_point mPurgeTtlTp;
	uint32_t mPurgeIntervalSec;

	string etcConfigPath;
	string localConfigPath;
	string selectedConfigPath;

	bool getConfigFile();
	bool validateConfigFile();
};

class StorageServer {
private:
	HttpServer *mServer;
	Timer *mTimer;
	TimerEvent *mTimerEvent;
	Config *mConfig;
	Semaphore mExitSem;

	static void _onRequest(RequestEvent *event, void *this_);
	void onRequest(RequestEvent *event);

	static void _onTimerEvent(TimerEvent *event, void *this_);
	void onTimerEvent(TimerEvent *event);

	void registerPaths();
public:
	StorageServer(Config *config);
	~StorageServer();
	void start();
	void stop();
};

} // End namespace SS.


#endif /* SRC_STORAGE_SERVER_HPP_ */
