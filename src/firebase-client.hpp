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
 * \file   firebase-client.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Dec 07, 2019
 *
 * \brief
 *
 ******************************************************************************/

#include <ch-cpp-utils/http/client/http.hpp>
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/third-party/json/json.hpp>

#include "config.hpp"

using json = nlohmann::json;

using ChCppUtils::Timer;
using ChCppUtils::TimerEvent;
using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpResponse;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

#ifndef SRC_FIREBASE_CLIENT_HPP_
#define SRC_FIREBASE_CLIENT_HPP_

namespace SS {

class FirebaseClient {
private:
	Config *mConfig;

	uint64_t mLastSentNs;
	uint64_t mIntervalNs;
	Timer *mTimer;
	TimerEvent *mTimerEvent;

	static void _onLoad(HttpRequestLoadEvent *event, void *this_);
	void onLoad(HttpRequestLoadEvent *event);

	static void _onTimerEvent(TimerEvent *event, void *this_);
	void onTimerEvent(TimerEvent *event);

	void send(string &target);
	void sendTargets();

	bool selectConfigFile();

	vector<string> getFirebaseTargets();

	string selectedConfigPath;
	string etcTargetsPath;
	string localTargetsPath;

	json mTargetsJson;

public:
	FirebaseClient(Config *config);
	~FirebaseClient();
	bool init();
	void send(json &message);
	bool addTarget(string &target);
	string getTargets();
};

} // End namespace SS.

#endif /* SRC_FIREBASE_CLIENT_HPP_ */
