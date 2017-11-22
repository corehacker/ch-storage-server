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

#include <ch-cpp-utils/semaphore.hpp>
#include <ch-cpp-utils/http-server.hpp>
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/fts.hpp>
#include <ch-cpp-utils/fs-watch.hpp>
#include "config.hpp"

#ifndef SRC_STORAGE_SERVER_HPP_
#define SRC_STORAGE_SERVER_HPP_

using namespace std::chrono;

using ChCppUtils::Semaphore;
using ChCppUtils::Http::Server::RequestEvent;
using ChCppUtils::Http::Server::HttpHeaders;
using ChCppUtils::Http::Server::HttpQuery;
using ChCppUtils::Http::Server::HttpServer;
using ChCppUtils::Timer;
using ChCppUtils::TimerEvent;
using ChCppUtils::FsWatch;
using ChCppUtils::OnFileData;

namespace SS {

class StorageServer {
private:
	HttpServer *mServer;
	Timer *mTimer;
	TimerEvent *mTimerEvent;
	Config *mConfig;
	Semaphore mExitSem;
	FsWatch *mFsWatch;

	string getDestinationDir(RequestEvent *event);
	string getDestinationSegmentPath(RequestEvent *event);
	string getDestinationManifestPath(RequestEvent *event);

	bool saveManifest(RequestEvent *event);
	bool saveSegment(RequestEvent *event);

	static void _onRequest(RequestEvent *event, void *this_);
	void onRequest(RequestEvent *event);

	static void _onFilePurge(OnFileData &data, void *this_);
	void onFilePurge(OnFileData &data);

	static void _onTimerEvent(TimerEvent *event, void *this_);
	void onTimerEvent(TimerEvent *event);

	static void _onNewFile(OnFileData &data, void *this_);
	void onNewFile(OnFileData &data);

	static void _onEmptyDir(OnFileData &data, void *this_);
	void onEmptyDir(OnFileData &data);

	void registerPaths();
public:
	StorageServer(Config *config);
	~StorageServer();
	void start();
	void stop();
};

} // End namespace SS.


#endif /* SRC_STORAGE_SERVER_HPP_ */
