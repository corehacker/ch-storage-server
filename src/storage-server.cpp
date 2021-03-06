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
#include <sys/types.h>
#include <chrono>
#include <cstdio>
#include <dirent.h>

#include "storage-server.hpp"

#include <glog/logging.h>

using std::ifstream;

using ChCppUtils::FtsOptions;
using ChCppUtils::Fts;
using ChCppUtils::mkPath;
using ChCppUtils::directoryListing;
using ChCppUtils::fileExpired;
using ChCppUtils::fileExists;
using ChCppUtils::send400BadRequest;
using ChCppUtils::send200OK;
using ChCppUtils::getDateTime;
using ChCppUtils::getHour;
using ChCppUtils::getDate;

namespace SS {

static int get_num_fds(void);

static int get_num_fds()
{
     int fd_count;
     char buf[64];
     struct dirent *dp;
		 DIR *dir = NULL;

     snprintf(buf, 64, "/proc/%i/fd/", getpid());

     fd_count = 0;
     
		 dir = opendir(buf);
     while ((dp = readdir(dir)) != NULL) {
          fd_count++;
     }
     closedir(dir);
     return fd_count;
}

string trim(const string& str)
{
    size_t first = str.find_first_not_of('*');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of('*');
    return str.substr(first, (last - first + 1));
}

StorageServer::StorageServer(Config *config) {
	mConfig = config;
	if(mkPath(mConfig->getRoot(), 0755)) {
		LOG(INFO) << "Created root directory: " << mConfig->getRoot();
	} else {
		LOG(ERROR) << "Error root creating directory: " << mConfig->getRoot();
	}

	mFsWatch = new FsWatch(mConfig->getRoot());

	mTimer = new Timer();
	struct timeval tv = {0};
	tv.tv_sec = mConfig->getPurgeIntervalSec();
	mTimerEvent = mTimer->create(&tv, StorageServer::_onTimerEvent, this);
	mServer = new HttpServer(mConfig->getPort(), 2);

	mKafkaClient = new KafkaClient(mConfig);
	mKafkaClient->init();

	mFirebaseClient = new FirebaseClient(config);

	mMotionDetector = new MotionDetector(mConfig, mKafkaClient, mFirebaseClient);
	if(mConfig->getCameraEnable()) {
		mMotionDetector->init();
		mMotionDetector->start();
	} else {
		string filename = "";
		if(mConfig->getMDStaticEnable()) {
			filename = mConfig->getMDStaticFile();
         mMotionDetector->process(filename);
		}
	}
	procStat = new ProcStat();
}

StorageServer::~StorageServer() {
	LOG(INFO) << "*****************~StorageServer";

	delete procStat;

	delete mMotionDetector;

	delete mServer;
	LOG(INFO) << "Deleted server.";

	mTimer->destroy(mTimerEvent);

	delete mTimer;

	delete mFsWatch;
}

string StorageServer::getDestinationDir(RequestEvent *event) {
	string destination = mConfig->getRoot();
	string path = event->getPath();
	string prefix = path.substr(0, path.find_last_of('/'));

	destination += prefix + "/" + getDate();
	if(mkPath(destination, 0755)) {
//		LOG(INFO) << "Created date based directory: " << destination;
	} else {
		LOG(ERROR) << "Error creating date based directory: " << destination;
	}

	destination += "/" + getHour();
	if(mkPath(destination, 0755)) {
//		LOG(INFO) << "Created hourly directory: " << destination;
	} else {
		LOG(ERROR) << "Error creating hourly directory: " << destination;
	}
	return destination;
}

string StorageServer::getDestinationManifestPath(RequestEvent *event) {
	string destination = getDestinationDir(event);
	destination += "/index.m3u8";
	return destination;
}

string StorageServer::getDestinationSegmentPath(RequestEvent *event) {
	string destination = getDestinationDir(event);
	string path = event->getPath();
	string filename = path.substr(path.find_last_of('/') + 1);
	destination += "/" + filename;
	return destination;
}

bool StorageServer::saveManifest(RequestEvent *event) {
	HttpQuery query = event->getQuery();
	string extinf = "", targetduration = "";
	auto search = query.find("extinf");
	if(search != query.end()) {
		extinf = search->second;
	}

	search = query.find("targetduration");
	if(search != query.end()) {
		targetduration = search->second;
	}

	bool status = false;
	if(extinf.size() > 0 && targetduration.size() > 0) {
		LOG(INFO) << "#EXTINF: " << search->second;
		LOG(INFO) << "#EXT-X-TARGETDURATION: " << search->second;
		string manifest = getDestinationManifestPath(event);

		string path = event->getPath();
		string filename = path.substr(path.find_last_of('/') + 1);
		std::fstream myfile;
		string body;
		if(!fileExists(manifest)) {
			LOG(INFO) << "Manifest: " << manifest << " does not exist. Will create";
			myfile = std::fstream(manifest, std::ios::out | std::ios::binary);
			body = "#EXTM3U\n"
					"#EXT-X-VERSION:3\n"
					"#EXT-X-MEDIA-SEQUENCE:1\n"
					"#EXT-X-ALLOW-CACHE:NO\n"
					"#EXT-X-TARGETDURATION:" + targetduration + "\n"
					"#EXTINF:" + extinf + ",\n" +
					filename + "\n";


		} else {
			LOG(INFO) << "Manifest: " << manifest << " exists. Will append";
			myfile = std::fstream(manifest,
					std::ios::out | std::ios::binary | std::ios::app);
			body = "#EXTINF:" + extinf + ",\n" +
					filename + "\n";
		}
		myfile.write(body.data(), body.size());
		myfile.close();
		status = true;
	} else {
		status = false;
	}
	return status;
}

bool StorageServer::saveSegment(RequestEvent *event) {
	bool status = false;
	void *body = event->getBody();
	LOG(INFO) << "Body: " << event->getLength() << " bytes";
	string destination = getDestinationSegmentPath(event);
	if(!fileExists(destination)) {
		LOG(INFO) << "File " << destination <<
				" does not exist. Will create.";
		auto myfile = std::fstream(destination,
				std::ios::out | std::ios::binary);
		myfile.write((char*)body, event->getLength());
		myfile.close();
		status = true;
	} else {
		LOG(WARNING) << "File " << destination << " exists.";
		status = false;
	}
	free(body);
	return status;
}

void StorageServer::detectMotion(RequestEvent *event) {
	string filename = getDestinationSegmentPath(event);
	mMotionDetector->process(filename);
}

void StorageServer::_onRequest(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onRequest(event);
}

void StorageServer::onRequest(RequestEvent *event) {
	if(event->hasBody()) {
		if(saveSegment(event) && saveManifest(event)) {
			send200OK(event->getRequest()->getRequest());

			detectMotion(event);
		} else {
			LOG(WARNING) << "Saving failed:" << event->getPath();
			send400BadRequest(event->getRequest()->getRequest());
		}
	} else {
		LOG(INFO) << "Empty body";
		send400BadRequest(event->getRequest()->getRequest());
	}
}

void StorageServer::_onDummyRequest(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onDummyRequest(event);
}

void StorageServer::onDummyRequest(RequestEvent *event) {
	LOG(INFO) << "Dummy Request";
	json j;
	j["filename"] = "/dummy.ts";
	mKafkaClient->send(j);
	send200OK(event->getRequest()->getRequest());
}

void StorageServer::_onDummyNotifyRequest(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onDummyNotifyRequest(event);
}

void StorageServer::onDummyNotifyRequest(RequestEvent *event) {
	LOG(INFO) << "Dummy Notification Request";
	json dummyMessage;
	dummyMessage["empty"] = "Dummy notification message";
	mFirebaseClient->send(dummyMessage);
	send200OK(event->getRequest()->getRequest());
}

void StorageServer::_onFilePurge (OnFileData &data, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onFilePurge(data);
}

void StorageServer::onFilePurge (OnFileData &data) {
	bool markForDelete = fileExpired(data.path, mConfig->getPurgeTtlSec());
	if(markForDelete) {
		if(0 != std::remove(data.path.data())) {
			LOG(ERROR) << "File: " << data.path << ", marked for Delete! failed to delete";
			perror("remove");
		} else {
			LOG(INFO) << "File: " << data.path << ", marked for Delete! Deleted successfully";
		}
	}
}

void StorageServer::_onTimerEvent(TimerEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onTimerEvent(event);
}

void StorageServer::onTimerEvent(TimerEvent *event) {
	uint32_t rss = procStat->getRSS();
	LOG(INFO) << "RSS: " << rss / 1024 << " KB, Max RSS Configured: " << 
		(mConfig->getMaxRss() / 1024) << " KB";

	if(rss > mConfig->getMaxRss()) {
		LOG(ERROR) << "*********FATAL**********";
		LOG(ERROR) << "Too much memory in use. Exiting process.";
		LOG(FATAL) << "*********FATAL**********";
		exit(1);
	}
	
	FtsOptions options;
//	LOG(INFO) << "Timer fired!";
	LOG(INFO) << "Number of open fd(s): " << get_num_fds();
	memset(&options, 0x00, sizeof(FtsOptions));
	options.bIgnoreRegularFiles = false;
	options.bIgnoreHiddenFiles = true;
	options.bIgnoreHiddenDirs = true;
	options.bIgnoreRegularDirs = true;
	for(auto stream : mConfig->mJson["streams"]) {
		string destination = mConfig->getRoot();
		destination += stream["path"];
		destination = trim(destination);
		Fts fts(destination, &options);
		fts.walk(StorageServer::_onFilePurge, this);
	}

	mTimer->restart(event);
}

void StorageServer::_onNewFile(OnFileData &data, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onNewFile(data);
}

void StorageServer::onNewFile(OnFileData &data) {
	LOG(INFO) << "New file: " << data.path;
}


void StorageServer::_onEmptyDir(OnFileData &data, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onEmptyDir(data);
}

void StorageServer::onEmptyDir(OnFileData &data) {

	bool found = false;
	for(auto stream : mConfig->mJson["streams"]) {
		string haystack(mConfig->getRoot());
		haystack += stream["path"];
		if (haystack.find(data.path) != string::npos) {
			found = true;
		}
	}
	if(!found) {
		LOG(INFO) << "Directory empty (will delete): " << data.path;
		remove(data.path.data());
	}
}

void StorageServer::_onFirebaseTargetDeviceRegister(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onFirebaseTargetDeviceRegister(event);
}

void StorageServer::onFirebaseTargetDeviceRegister(RequestEvent *event) {
	LOG(INFO) << "Firebase Target Device Register";

	string body((char *) event->getBody());

	LOG(INFO) << "Firebase Target Device Register: " << body;

	if(mFirebaseClient->addTarget(body)) {
		send200OK(event->getRequest()->getRequest());
	} else {
		send400BadRequest(event->getRequest()->getRequest());
	}
}

void StorageServer::_onFirebaseTargetDevice(RequestEvent *event, void *this_) {
	StorageServer *server = (StorageServer *) this_;
	server->onFirebaseTargetDevice(event);
}

void send200OK(evhttp_request *request, string &body) {
	struct evbuffer *buffer = evhttp_request_get_output_buffer(request);
	if (!buffer)
		return;
	evbuffer_add_printf(buffer, body.c_str());
	evhttp_send_reply(request, HTTP_OK, "", buffer);

	LOG(INFO) << "Sending " << HTTP_OK;
}

void StorageServer::onFirebaseTargetDevice(RequestEvent *event) {
	LOG(INFO) << "Firebase Target Device";

	string targets = mFirebaseClient->getTargets();

	send200OK(event->getRequest()->getRequest(), targets);
}

void StorageServer::registerPaths() {
	auto streams = mConfig->mJson["streams"];
	for(auto stream : streams) {
		LOG(INFO) << "Stream: " << stream;
		string destination = mConfig->getRoot();
		destination += stream["path"];
		destination = trim(destination);
		if(mkPath(destination, 0755)) {
			LOG(INFO) << "Created destination directory: " << destination;
		} else {
			LOG(ERROR) << "Error creating destination directory: " <<
					destination;
		}
		mServer->route(EVHTTP_REQ_POST, stream["path"],
				StorageServer::_onRequest, this);
	}
	mServer->route(EVHTTP_REQ_POST, "/dummy/motion", StorageServer::_onDummyRequest, this);
	mServer->route(EVHTTP_REQ_POST, "/dummy/notify", StorageServer::_onDummyNotifyRequest, this);

	mServer->route(EVHTTP_REQ_POST, "/firebase/target/device/register", StorageServer::_onFirebaseTargetDeviceRegister, this);
	mServer->route(EVHTTP_REQ_GET, "/firebase/target/devices", StorageServer::_onFirebaseTargetDevice, this);
}

void StorageServer::start() {
	LOG(INFO) << "Starting server";

	mFsWatch->init();
	mFsWatch->OnNewFileCbk(StorageServer::_onNewFile, this);
	mFsWatch->OnEmptyDirCbk(StorageServer::_onEmptyDir, this);
	mFsWatch->start();

	registerPaths();
//	mExitSem.wait();
}

void StorageServer::stop() {
	mExitSem.notify();
	LOG(INFO) << "Stopping server";
}

} // End namespace SS.
