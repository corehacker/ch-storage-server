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
#include "image-saver.hpp"

#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <chrono>
#include <cstdio>

#include <glog/logging.h>

#include <opencv2/opencv.hpp>

#include "mc-config.hpp"

using namespace cv;

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

ImageSaver::ImageSaver(MCConfig *config) {
	mConfig = config;
	if(mkPath(mConfig->getRoot(), 0755)) {
		LOG(INFO) << "Created root directory: " << mConfig->getRoot();
	} else {
		LOG(ERROR) << "Error root creating directory: " << mConfig->getRoot();
	}

	mSaverPool = new ThreadPool(1, false);

	mTimer = new Timer();
	struct timeval tv = {0};
	tv.tv_sec = mConfig->getPurgeIntervalSec();
	mTimerEvent = mTimer->create(&tv, ImageSaver::_onTimerEvent, this);
	mServer = new HttpServer(mConfig->getPort(), 2);
}

ImageSaver::~ImageSaver() {
	LOG(INFO) << "*****************~ImageSaver";

	delete mServer;
	LOG(INFO) << "Deleted server.";

	mTimer->destroy(mTimerEvent);

	delete mSaverPool;

	delete mTimer;
}

string ImageSaver::getDestinationDir(RequestEvent *event) {
	string destination = mConfig->getRoot();
	string path = event->getPath();
	string prefix = path.substr(0, path.find_last_of('/'));
	destination += prefix;
	return destination;
}

string ImageSaver::getDestinationImagePath(RequestEvent *event) {
	string destination = getDestinationDir(event);
	string path = event->getPath();
	string filename = path.substr(path.find_last_of('/') + 1);
	destination += "/" + filename;
	return destination;
}

void *ImageSaver::_saveImageRoutine(void *arg, struct event_base *base) {
	SaveImageEvent *evt = (SaveImageEvent *) arg;
	return evt->imageSaver->saveImageRoutine(evt);
}

void *ImageSaver::saveImageRoutine(SaveImageEvent *evt) {
	LOG(INFO) << "File " << evt->destination <<
					" does not exist. Will create.";
	Mat image(evt->height, evt->width, CV_8UC3, evt->buffer, evt->linesize);

	imwrite(evt->destination, image);

	free(evt->buffer);
	delete evt;
	return NULL;
}

uint32_t ImageSaver::getImageWidth(RequestEvent *event) {
	HttpQuery query = event->getQuery();
	auto search = query.find("width");
	string width = "0";
	if(search != query.end()) {
		width = search->second;
	}
	return (uint32_t) stoi(width);
}

uint32_t ImageSaver::getImageHeight(RequestEvent *event) {
	HttpQuery query = event->getQuery();
	auto search = query.find("height");
	string height = "0";
	if(search != query.end()) {
		height = search->second;
	}
	return (uint32_t) stoi(height);
}

uint32_t ImageSaver::getImageLinesize(RequestEvent *event) {
	HttpQuery query = event->getQuery();
	auto search = query.find("linesize");
	string linesize = "0";
	if(search != query.end()) {
		linesize = search->second;
	}
	return (uint32_t) stoi(linesize);
}

bool ImageSaver::generateImageEvent(RequestEvent *event) {
	bool status = false;
	void *body = event->getBody();
	LOG(INFO) << "Body: " << event->getLength() << " bytes";
	string destination = getDestinationImagePath(event);
	if(!fileExists(destination) &&
			getImageWidth(event) &&
			getImageHeight(event) &&
			getImageLinesize(event)) {
		SaveImageEvent *evt = new SaveImageEvent();
		evt->imageSaver = this;
		evt->destination = destination;
		evt->length = event->getLength();
		evt->buffer = (uint8_t *) body;
		evt->width = getImageWidth(event);
		evt->height = getImageHeight(event);
		evt->linesize = getImageLinesize(event);
		ThreadJob *job = new ThreadJob(ImageSaver::_saveImageRoutine, evt);
		mSaverPool->addJob(job);
		status = true;
	} else {
		if(fileExists(destination))
			LOG(WARNING) << "File " << destination << " exists.";
		else
			LOG(WARNING) << "Expected query params: width, height, linesize";
		status = false;
	}
//	free(body);
	return status;
}

void ImageSaver::_onRequest(RequestEvent *event, void *this_) {
	ImageSaver *server = (ImageSaver *) this_;
	server->onRequest(event);
}

void ImageSaver::onRequest(RequestEvent *event) {
	if(event->hasBody()) {
		if(generateImageEvent(event)) {
			send200OK(event->getRequest()->getRequest());
		} else {
			LOG(WARNING) << "Saving failed:" << event->getPath();
			send400BadRequest(event->getRequest()->getRequest());
		}
	} else {
		LOG(INFO) << "Empty body";
		send400BadRequest(event->getRequest()->getRequest());
	}
}

void ImageSaver::_onFilePurge (OnFileData &data, void *this_) {
	ImageSaver *server = (ImageSaver *) this_;
	server->onFilePurge(data);
}

void ImageSaver::onFilePurge (OnFileData &data) {
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

void ImageSaver::_onTimerEvent(TimerEvent *event, void *this_) {
	ImageSaver *server = (ImageSaver *) this_;
	server->onTimerEvent(event);
}

void ImageSaver::onTimerEvent(TimerEvent *event) {
	FtsOptions options;
//	LOG(INFO) << "Timer fired!";
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
		fts.walk(ImageSaver::_onFilePurge, this);
	}

	mTimer->restart(event);
}

void ImageSaver::_onNewFile(OnFileData &data, void *this_) {
	ImageSaver *server = (ImageSaver *) this_;
	server->onNewFile(data);
}

void ImageSaver::onNewFile(OnFileData &data) {
	LOG(INFO) << "New file: " << data.path;
}


void ImageSaver::_onEmptyDir(OnFileData &data, void *this_) {
	ImageSaver *server = (ImageSaver *) this_;
	server->onEmptyDir(data);
}

void ImageSaver::onEmptyDir(OnFileData &data) {

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


void ImageSaver::registerPaths() {
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
				ImageSaver::_onRequest, this);
	}
}

void ImageSaver::start() {
	LOG(INFO) << "Starting server";

	registerPaths();
//	mExitSem.wait();
}

void ImageSaver::stop() {
	mExitSem.notify();
	LOG(INFO) << "Stopping server";
}

} // End namespace SS.
