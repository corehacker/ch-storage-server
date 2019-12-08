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
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include <libavutil/pixdesc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <ch-cpp-utils/semaphore.hpp>
#include <ch-cpp-utils/thread-pool.hpp>
#include <ch-cpp-utils/http/client/http.hpp>

#include "config.hpp"
#include "mail-client.hpp"
#include "kafka-client.hpp"
#include "firebase-client.hpp"

#ifdef ENABLE_TENSORFLOW
#include "label-image.hpp"
#endif

using std::unordered_map;
using std::make_pair;
using std::vector;
using std::list;
using std::mutex;
using std::condition_variable;

using ChCppUtils::Semaphore;
using ChCppUtils::ThreadPool;
using ChCppUtils::ThreadJobBase;
using ChCppUtils::ThreadJob;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

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

typedef bool (*NextFrame) (Mat &frame, uint32_t index,
      bool lastFrame, uint32_t linesize, void *this_);

class FileProcessingCtxt {
private:
   string filename;
   AVFormatContext *ifmt_ctx;
   int video_stream_index;
	AVCodecContext *avctx;
   AVCodec* vcodec;
	AVFrame *pFrame;
	AVFrame *pFrameRGB;
	struct SwsContext *img_convert_ctx;
	AVStream* vstrm;
   uint32_t frameCount;
   uint32_t nextFrameIndex;
   NextFrame nextFrame;
   void *this_;
   vector<uint8_t> framebuf;
   int dst_width;
   int dst_height;
   AVPixelFormat dst_pix_fmt;
   uint8_t *buffer;

   void initBuffers();
   bool initFfmpeg();
   void processFrame(bool lastFrame = false);

public:
   FileProcessingCtxt(string &filename);
   ~FileProcessingCtxt();
   void process(NextFrame nextFrame, void *this_);
};

class MotionDetectorCtxt;

class UploadContext {
public:
   UploadContext(MotionDetectorCtxt *ctxt, HttpRequest *request);
   ~UploadContext();
   uint8_t* getBuffer();
   void setBuffer(uint8_t* buffer);
   uint32_t getLength();
   void setLength(uint32_t length);
   MotionDetectorCtxt* getContext();
   HttpRequest* getRequest();
   string& getUrl();
   void setUrl(string url);
   void wait();
   void done();

private:
   MotionDetectorCtxt *context;
   HttpRequest *request;
   Semaphore mFinished;

   uint8_t *buffer;
   uint32_t length;
   string url;
};

class MotionDetectorCtxt {
private:
	Config *mConfig;
   string filename;
   bool detected;
   FileProcessingCtxt *mFPCtxt;
   vector<Mat> detectedFrames;
	uint32_t mMinArea;
	Mat firstFrame;
#ifdef ENABLE_TENSORFLOW
   LabelImage *mLabelImage;
#endif
   mutex mFinishedUploadsMutex;
   list<UploadContext *> mFinishedUploads;

#ifdef ENABLE_TENSORFLOW
   bool detectTf(Mat &frame);
#endif
	bool detectCv(Mat &frame);
   static bool _nextFrame (Mat &frame, uint32_t index, bool lastFrame, uint32_t linesize, void *this_);
   bool nextFrame (Mat &frame, uint32_t index, bool lastFrame, uint32_t linesize);
	void render(const String& winname, InputArray mat);

	void freeFinishedUploads();

	static void _onLoad(HttpRequestLoadEvent *event, void *this_);
	void onLoad(HttpRequestLoadEvent *event, UploadContext *context);
	void saveFrame(Mat &frame, string &filename, uint32_t index, bool lastFrame, uint32_t linesize);

public:
   MotionDetectorCtxt(Config *config, string &filename);
   ~MotionDetectorCtxt();
   void process();
#ifdef ENABLE_TENSORFLOW
	void setLabelImage(LabelImage *labelImage);
#endif
   bool motionDetected();
};

class MotionDetectorThread {
private:
	Config *mConfig;
#ifdef ENABLE_TENSORFLOW
   LabelImage *mLabelImage;
#endif

public:
	MotionDetectorThread(Config *config);
	~MotionDetectorThread();
	void processJob(MotionDetectorJob *data);
};

class MotionDetector {
private:
	Config *mConfig;

	VideoCapture cameraCapture;

	ThreadPool *mPool;
	unordered_map<pthread_t, MotionDetectorThread *> mPoolCtxt;

	MailClient *mMailClient;
	KafkaClient *mKafkaClient;
   FirebaseClient *mFirebaseClient;

	void initiateCameraCapture();

	static void *_routine(void *arg, struct event_base *base);
	void *routine(MotionDetectorJob *data);

public:
	MotionDetector(Config *config);
      MotionDetector(Config *config, KafkaClient *kafkaClient);
	~MotionDetector();
	void init();
	void start();
	void deinit();
	void process(string &filename);
	void notify(string &filename);
};

} // End namespace SS.

#endif /* SRC_MOTION_DETECTOR_HPP_ */
