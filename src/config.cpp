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
 * \file   config.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <string>
#include <glog/logging.h>

#include "config.hpp"

using std::ifstream;

namespace SS {


Config::Config() :
				ChCppUtils::Config("/etc/ch-storage-server/ch-storage-server.json",
						"./config/ch-storage-server.json") {
	mPort = 0;
	mRoot = "";
	mPurgeIntervalSec = 60;
	mPurgeTtlSec = 60;

	mCameraEnable = false;
	mCameraDevice = 0;
	mCameraFps = 5.0;
	mCameraWidth = 640;
	mCameraHeight = 480;

	mMDMinArea = 500;
	mMDThreadCount = 1;

	mMDStaticEnable = false;
	mMDStaticFile = "";

	mMDRender = false;
	mMDRenderDelay = 0;

   mMDTfEnable = false;
   mMDTfGraph = "";
   mMDTfLabels = "";
   mMDTfInputWidth = 0;
   mMDTfInputHeight = 0;
   mMDTfInputMean = 0;
   mMDTfInputStd = 0;
   mMDTfInputLayer = "";
   mMDTfOutputLayer = "";

	mNotEnable = false;
	mNotEmailEnable = false;
	mNotEmailThreadCount = 0;
	mNotEmailFrom = "";
	mNotEmailSubject = "";
	mNotEmailSmtpUrl = "";
	mNotEmailAggregate = 0;

	mNotKafkaEnable = false;
	mNotKafkaConnection = "";
	mNotKafkaTopic = "";
	mNotKafkaPartition = 0;

	mNotFirebaseEnable = false;
	mNotFirebaseProtocol = "";
	mNotFirebaseHostname = "";
	mNotFirebaseUrl = "";
	mNotFirebaseAuthKey = "";
	mNotFirebaseTtl = "";
	mNotFirebaseTopic = "";
	mNotFirebaseContentType = "";
	mNotFirebaseIntervalSeconds = 60;
}

Config::~Config() {
	LOG(INFO) << "*****************~Config";
}

bool Config::populateConfigValues() {
	LOG(INFO) << "<-----------------------Config";
	mPort = mJson["server"]["port"];
	LOG(INFO) << "server.port : " << mPort;

	mRoot = mJson["storage"]["root"];
	LOG(INFO) << "storage.root: " << mRoot;

	mPurgeTtlSec = mJson["purge"]["ttl-s"];
	LOG(INFO) << "purge.ttl-s: " << mPurgeTtlSec;

	mPurgeIntervalSec = mJson["purge"]["interval-s"];
	LOG(INFO) << "purge.interval-s: " << mPurgeIntervalSec;

	mCameraEnable = mJson["camera"]["enable"];
	LOG(INFO) << "camera.enable: " << mCameraEnable;

	mCameraDevice = mJson["camera"]["device"];
	LOG(INFO) << "camera.device: " << mCameraDevice;

	mCameraFps    = mJson["camera"]["fps"];
	LOG(INFO) << "camera.fps: " << mCameraFps;

	mCameraWidth  = mJson["camera"]["width"];
	LOG(INFO) << "camera.width: " << mCameraWidth;

	mCameraHeight = mJson["camera"]["height"];
	LOG(INFO) << "camera.height: " << mCameraHeight;

	mMDMinArea = mJson["motion-detector"]["min-area"];
	LOG(INFO) << "motion-detector.min-area: " << mMDMinArea;

	mMDThreadCount = mJson["motion-detector"]["thread-count"];
	LOG(INFO) << "motion-detector.thread-count: " << mMDThreadCount;

	mMDStaticEnable = mJson["motion-detector"]["static"]["enable"];
	LOG(INFO) << "motion-detector.static.enable: " << mMDStaticEnable;

	mMDStaticFile = mJson["motion-detector"]["static"]["file"];
	LOG(INFO) << "motion-detector.static.file: " << mMDStaticFile;

	mMDRender = mJson["motion-detector"]["render"];
	LOG(INFO) << "motion-detector.render: " << mMDRender;

	mMDRenderDelay = mJson["motion-detector"]["render-delay"];
	LOG(INFO) << "motion-detector.render-delay: " << mMDRenderDelay;

   mMDTfEnable = mJson["motion-detector"]["tensorflow"]["enable"];
	LOG(INFO) << "motion-detector.tensorflow.enable: " << mMDTfEnable;

   mMDTfGraph = mJson["motion-detector"]["tensorflow"]["graph"];
	LOG(INFO) << "motion-detector.tensorflow.graph: " << mMDTfGraph;

   mMDTfLabels = mJson["motion-detector"]["tensorflow"]["labels"];
	LOG(INFO) << "motion-detector.tensorflow.labels: " << mMDTfLabels;

   mMDTfInputWidth = mJson["motion-detector"]["tensorflow"]["input-width"];
	LOG(INFO) << "motion-detector.tensorflow.input-width: " << mMDTfInputWidth;

   mMDTfInputHeight = mJson["motion-detector"]["tensorflow"]["input-height"];
	LOG(INFO) << "motion-detector.tensorflow.input-height: " << mMDTfInputHeight;

   mMDTfInputMean = mJson["motion-detector"]["tensorflow"]["input-mean"];
	LOG(INFO) << "motion-detector.tensorflow.input-mean: " << mMDTfInputMean;

   mMDTfInputStd = mJson["motion-detector"]["tensorflow"]["input-std"];
	LOG(INFO) << "motion-detector.tensorflow.input-std: " << mMDTfInputStd;

   mMDTfInputLayer = mJson["motion-detector"]["tensorflow"]["input-layer"];
	LOG(INFO) << "motion-detector.tensorflow.input-layer: " << mMDTfInputLayer;

   mMDTfOutputLayer = mJson["motion-detector"]["tensorflow"]["output-layer"];
	LOG(INFO) << "motion-detector.tensorflow.output-layer: " << mMDTfOutputLayer;

	mNotEnable = mJson["notifications"]["enable"];
	LOG(INFO) << "notifications.enable: " << mNotEnable;

	mNotEmailEnable = mJson["notifications"]["email"]["enable"];
	LOG(INFO) << "notifications.email.enable: " << mNotEmailEnable;

	mNotEmailThreadCount = mJson["notifications"]["email"]["thread-count"];
	LOG(INFO) << "notifications.email.thread-count: " << mNotEmailThreadCount;

	mNotEmailFrom = mJson["notifications"]["email"]["from"];
	LOG(INFO) << "notifications.email.from: " << mNotEmailFrom;

	for(auto to : mJson["notifications"]["email"]["to"]) {
		mNotEmailTo.push_back(to);
	}

	for(auto cc : mJson["notifications"]["email"]["cc"]) {
		mNotEmailCc.push_back(cc);
	}

	mNotEmailSubject = mJson["notifications"]["email"]["subject"];
	LOG(INFO) << "notifications.email.subject: " << mNotEmailSubject;

	mNotEmailSmtpUrl = mJson["notifications"]["email"]["smtp"]["url"];
	LOG(INFO) << "notifications.email.smtp.url: " << mNotEmailSmtpUrl;

	mNotEmailAggregate = mJson["notifications"]["email"]["aggregate"];
	LOG(INFO) << "notifications.email.aggregate: " << mNotEmailAggregate;


	mNotKafkaEnable = mJson["notifications"]["kafka"]["enable"];
	LOG(INFO) << "notifications.kafka.enable: " << mNotKafkaEnable;

	mNotKafkaConnection = mJson["notifications"]["kafka"]["connection"];
	LOG(INFO) << "notifications.kafka.connection: " << mNotKafkaConnection;

	mNotKafkaTopic = mJson["notifications"]["kafka"]["topic"];
	LOG(INFO) << "notifications.kafka.topic: " << mNotKafkaTopic;

	mNotKafkaPartition = mJson["notifications"]["kafka"]["partition"];
	LOG(INFO) << "notifications.kafka.partition: " << mNotKafkaPartition;


	mNotFirebaseEnable = mJson["notifications"]["firebase"]["enable"];
	LOG(INFO) << "notifications.firebase.enable: " << mNotFirebaseEnable;

	mNotFirebaseProtocol = mJson["notifications"]["firebase"]["protocol"];
	LOG(INFO) << "notifications.firebase.protocol: " << mNotFirebaseProtocol;

	mNotFirebaseHostname = mJson["notifications"]["firebase"]["hostname"];
	LOG(INFO) << "notifications.firebase.hostname: " << mNotFirebaseHostname;

	mNotFirebaseUrl = mJson["notifications"]["firebase"]["url"];
	LOG(INFO) << "notifications.firebase.url: " << mNotFirebaseUrl;

	mNotFirebaseAuthKey = mJson["notifications"]["firebase"]["auth-key"];
	LOG(INFO) << "notifications.firebase.auth-key: " << mNotFirebaseAuthKey;

	mNotFirebaseTtl = mJson["notifications"]["firebase"]["ttl"];
	LOG(INFO) << "notifications.firebase.ttl: " << mNotFirebaseTtl;

	mNotFirebaseContentType = mJson["notifications"]["firebase"]["content-type"];
	LOG(INFO) << "notifications.firebase.content-type: " << mNotFirebaseContentType;

	mNotFirebaseTopic = mJson["notifications"]["firebase"]["topic"];
	LOG(INFO) << "notifications.firebase.topic: " << mNotFirebaseTopic;
	
	mNotFirebaseIntervalSeconds = mJson["notifications"]["firebase"]["interval-seconds"];
	LOG(INFO) << "notifications.firebase.interval-seconds: " << mNotFirebaseIntervalSeconds;

	for(auto target : mJson["notifications"]["firebase"]["targets"]) {
		mNotFirebaseTargets.push_back(target);
	}

	LOG(INFO) << "----------------------->Config";
	return true;
}

void Config::init() {
	ChCppUtils::Config::init();

	populateConfigValues();
}

uint16_t Config::getPort() {
	return mPort;
}

string &Config::getRoot() {
	return mRoot;
}

uint32_t Config::getPurgeTtlSec() {
	return mPurgeTtlSec;
}

uint32_t Config::getPurgeIntervalSec() {
	return mPurgeIntervalSec;
}

bool Config::getCameraEnable() {
	return mCameraEnable;
}

uint32_t Config::getCameraDevice() {
	return mCameraDevice;
}

double_t Config::getCameraFps() {
	return mCameraFps;
}

uint32_t Config::getCameraWidth() {
	return mCameraWidth;
}

uint32_t Config::getCameraHeight() {
	return mCameraHeight;
}

uint32_t Config::getMDThreadCount() {
	return mMDThreadCount;
}

uint32_t Config::getMDMinArea() {
	return mMDMinArea;
}

bool Config::getMDStaticEnable() {
	return mMDStaticEnable;
}

string Config::getMDStaticFile() {
	return mMDStaticFile;
}

bool Config::getMDRender() {
	return mMDRender;
}

uint32_t Config::getMDRenderDelay() {
	return mMDRenderDelay;
}

bool Config::getMDTfEnable() {
   return mMDTfEnable;
}

string Config::getMDTfGraph() {
   return mMDTfGraph;
}

string Config::getMDTfLabels() {
   return mMDTfLabels;
}

uint32_t Config::getMDTfInputWidth() {
   return mMDTfInputWidth;
}

uint32_t Config::getMDTfInputHeight() {
   return mMDTfInputHeight;
}

uint32_t Config::getMDTfInputMean() {
   return mMDTfInputMean;
}

uint32_t Config::getMDTfInputStd() {
   return mMDTfInputStd;
}

string Config::getMDTfInputLayer() {
   return mMDTfInputLayer;
}

string Config::getMDTfOutputLayer() {
   return mMDTfOutputLayer;
}

bool Config::getNotEnable() {
	return mNotEnable;
}

bool Config::getNotEmailEnable() {
	return mNotEmailEnable;
}

uint32_t Config::getNotEmailThreadCount() {
	return mNotEmailThreadCount;
}

string Config::getNotEmailFrom() {
	return mNotEmailFrom;
}

vector<string> Config::getNotEmailTo() {
	return mNotEmailTo;
}

vector<string> Config::getNotEmailCc() {
	return mNotEmailCc;
}

string Config::getNotEmailSubject() {
	return mNotEmailSubject;
}

string Config::getNotEmailSmtpUrl() {
	return mNotEmailSmtpUrl;
}

uint64_t Config::getNotEmailAggregate() {
	return mNotEmailAggregate;
}

bool Config::getNotKafkaEnable() {
	return mNotKafkaEnable;
}

string Config::getNotKafkaConnection() {
	return mNotKafkaConnection;
}

string Config::getNotKafkaTopic() {
	return mNotKafkaTopic;
}

uint32_t Config::getNotKafkaPartition() {
	return mNotKafkaPartition;
}

bool Config::getNotFirebaseEnable() {
	return mNotFirebaseEnable;
}

string Config::getNotFirebaseProtocol() {
	return mNotFirebaseProtocol;
}

string Config::getNotFirebaseHostname() {
	return mNotFirebaseHostname;
}

string Config::getNotFirebaseUrl() {
	return mNotFirebaseUrl;
}

string Config::getNotFirebaseAuthKey() {
	return mNotFirebaseAuthKey;
}

string Config::getNotFirebaseTtl() {
	return mNotFirebaseTtl;
}

string Config::getNotFirebaseContentType() {
	return mNotFirebaseContentType;
}

string Config::getNotFirebaseTopic() {
	return mNotFirebaseTopic;
}

uint64_t Config::getNotFirebaseIntervalSeconds() {
	return mNotFirebaseIntervalSeconds;
}

vector<string> Config::getNotFirebaseTargets() {
	return mNotFirebaseTargets;
}



} // End namespace SS.
