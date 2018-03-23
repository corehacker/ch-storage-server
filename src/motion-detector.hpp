/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2018, Sandeep Prakash
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
 * Copyright (c) 2018, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   motion-detector.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Feb 26, 2018
 *
 * \brief
 *
 ******************************************************************************/
#include <opencv2/opencv.hpp>

#include <ch-cpp-utils/thread-pool.hpp>

#include "config.hpp"
#include "mail-client.hpp"

using std::unordered_map;
using std::make_pair;

using ChCppUtils::ThreadPool;
using ChCppUtils::ThreadJobBase;
using ChCppUtils::ThreadJob;

using namespace cv;

#ifndef SRC_MOTION_DETECTOR_HPP_
#define SRC_MOTION_DETECTOR_HPP_

namespace SS {

class MotionDetector;

class MotionDetectorJob {
public:
	string mFilename;
	MotionDetector *mMD;
	MotionDetectorJob (MotionDetector *md, string &filename);
   ~MotionDetectorJob ();
   void notify();
};

class MotionDetectorThread {
private:
	Config *mConfig;

	uint32_t mMinArea;

	Mat capturedImageRgbFrame;
	Mat capturedImageGrayFrame;
	Mat firstFrame;

	bool detect(const string &filename);
	void render(const String& winname, InputArray mat);

public:
	MotionDetectorThread(Config *config);
	~MotionDetectorThread();
	void processJob(MotionDetectorJob *data);
	void detect(Mat &frame);
};

class MotionDetector {
private:
	Config *mConfig;

	VideoCapture cameraCapture;

	ThreadPool *mPool;
	unordered_map<pthread_t, MotionDetectorThread *> mPoolCtxt;

	MailClient *mMailClient;

	void initiateCameraCapture();

	static void *_routine(void *arg, struct event_base *base);
	void *routine(MotionDetectorJob *data);

public:
	MotionDetector(Config *config);
	~MotionDetector();
	void init();
	void start();
	void deinit();
	void process(string &filename);
	void notify(string &filename);
};

} // End namespace SS.

#endif /* SRC_MOTION_DETECTOR_HPP_ */
