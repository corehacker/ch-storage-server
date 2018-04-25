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
 * \file   config.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <vector>
#include <string>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/config.hpp>

#ifndef SRC_CONFIG_HPP_
#define SRC_CONFIG_HPP_

using std::vector;
using std::string;

namespace SS {

class Config : public ChCppUtils::Config {
public:
	Config();
	~Config();
	void init();

	uint16_t getPort();
	string &getRoot();
	uint32_t getPurgeTtlSec();
	uint32_t getPurgeIntervalSec();

	bool	 getCameraEnable();
	uint32_t getCameraDevice();
	double_t getCameraFps();
	uint32_t getCameraWidth();
	uint32_t getCameraHeight();

	uint32_t getMDThreadCount();
	uint32_t getMDMinArea();

	bool getMDStaticEnable();
	string getMDStaticFile();

	bool getMDRender();
	uint32_t getMDRenderDelay();

   bool getMDTfEnable();
   string getMDTfGraph();
   string getMDTfLabels();
   uint32_t getMDTfInputWidth();
   uint32_t getMDTfInputHeight();
   uint32_t getMDTfInputMean();
   uint32_t getMDTfInputStd();
   string getMDTfInputLayer();
   string getMDTfOutputLayer();

	bool getNotEnable();
	bool getNotEmailEnable();
	uint32_t getNotEmailThreadCount();
	string getNotEmailFrom();
	vector<string> getNotEmailTo();
	vector<string> getNotEmailCc();
	string getNotEmailSubject();
	string getNotEmailSmtpUrl();
	uint64_t getNotEmailAggregate();

private:
	uint16_t mPort;
	string mRoot;

	uint32_t mPurgeTtlSec;
	system_clock::time_point mPurgeTtlTp;
	uint32_t mPurgeIntervalSec;

	string etcConfigPath;
	string localConfigPath;
	string selectedConfigPath;

	bool	 mCameraEnable;
	uint32_t mCameraDevice;
	double_t mCameraFps;
	uint32_t mCameraWidth;
	uint32_t mCameraHeight;

	uint32_t mMDThreadCount;
	uint32_t mMDMinArea;

	bool mMDStaticEnable;
	string mMDStaticFile;

	bool mMDRender;
	uint32_t mMDRenderDelay;

   bool mMDTfEnable;
   string mMDTfGraph;
   string mMDTfLabels;
   uint32_t mMDTfInputWidth;
   uint32_t mMDTfInputHeight;
   uint32_t mMDTfInputMean;
   uint32_t mMDTfInputStd;
   string mMDTfInputLayer;
   string mMDTfOutputLayer;

	bool mNotEnable;
	bool mNotEmailEnable;
	uint32_t mNotEmailThreadCount;
	string mNotEmailFrom;
	vector<string> mNotEmailTo;
	vector<string> mNotEmailCc;
	string mNotEmailSubject;
	string mNotEmailSmtpUrl;
	uint64_t mNotEmailAggregate;

	bool populateConfigValues();
};

} // End namespace SC.


#endif /* SRC_CONFIG_HPP_ */
