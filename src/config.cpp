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

} // End namespace SS.
