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
 * \file   curl-smtp.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Mar 15, 2018
 *
 * \brief
 *
 ******************************************************************************/
#include <glog/logging.h>

#include "curl-smtp.hpp"

#ifndef MIN
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif

namespace SS {

CurlSmtp::CurlSmtp(Config *config) {
	mConfig = config;
	mCurl = curl_easy_init();
	mNextStep = UNKNOWN;
   mMessage = "";
}

CurlSmtp::~CurlSmtp() {
	LOG(INFO) << "*****************~CurlSmtp";
	if (mCurl) {
	    /* curl won't send the QUIT command until you call cleanup, so you should
	     * be able to re-use this connection for additional messages (setting
	     * CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
	     * curl_easy_perform() again. It may not be a good idea to keep the
	     * connection open for a very long time though (more than a few minutes
	     * may result in the server timing out the connection), and you do want to
	     * clean up in the end.
	     */
		curl_easy_cleanup(mCurl);
		mCurl = NULL;
	}
}

size_t CurlSmtp::_source(void *ptr, size_t size, size_t nmemb, void *userp) {
	CurlSmtp *curlSmtp = (CurlSmtp *) userp;
	return curlSmtp->source(ptr, size, nmemb);
}

size_t CurlSmtp::source(void *ptr, size_t size, size_t nmemb) {
   LOG(INFO) << "Curl SMTP Mail Source: size: " << size << ", nmemb: " <<
      nmemb << ", mNextStep: " << mNextStep;
	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
		return 0;
	}

   string data = "";
	switch(mNextStep) {
	case UNKNOWN:
		break;
	case DATE:
		data = "\r\n";
		break;
	case TO: {
		string toList = "To: ";
		for(auto to : mConfig->getNotEmailTo()) {
			toList += to + ", ";
		}
		toList += "\r\n";
		data = toList;
		break;
	}
	case FROM: {
		string from = "From: " + mConfig->getNotEmailFrom() + "\r\n";
		data = from;
		break;
	}
	case CC: {
		string ccList = "Cc: ";
		for(auto cc : mConfig->getNotEmailCc()) {
			ccList += cc + ", ";
		}
		ccList += "\r\n";
		data = ccList;
		break;
	}
	case SUBJECT: {
		string subject = "Subject: " + mConfig->getNotEmailSubject() + "\r\n";
		data = subject;
		break;
	}
	case NEW_LINE_1:
		data = "\r\n";
		break;
	case BODY:
		data = mMessage;
		break;
	case END:
		break;
	}
	mNextStep = mNextStep + 1;

	size_t len = 0;
   if(0 != data.length()) {
      len = MIN(data.length(), (size * nmemb));
      memcpy(ptr, data.data(), len);
      LOG(INFO) << "Length: " << len << ", Body: " << data;
   }
	return len;
}

bool CurlSmtp::send(string &message) {
   mMessage = message;
	struct curl_slist *recipients = NULL;
	if (mCurl) {
      LOG(INFO) << "SMTP | Url: " << mConfig->getNotEmailSmtpUrl();
		/* This is the URL for your mailserver */
		curl_easy_setopt(mCurl, CURLOPT_URL, mConfig->getNotEmailSmtpUrl().data());

		/* Note that this option isn't strictly required, omitting it will result
		 * in libcurl sending the MAIL FROM command with empty sender data. All
		 * autoresponses should have an empty reverse-path, and should be directed
		 * to the address in the reverse-path which triggered them. Otherwise,
		 * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
		 * details.
		 */
      LOG(INFO) << "SMTP | From: " << mConfig->getNotEmailFrom();
		curl_easy_setopt(mCurl, CURLOPT_MAIL_FROM,
				mConfig->getNotEmailFrom().data());

		/* Add two recipients, in this particular case they correspond to the
		 * To: and Cc: addressees in the header, but they could be any kind of
		 * recipient. */
		for(auto to : mConfig->getNotEmailTo()) {
         LOG(INFO) << "SMTP | To: " << to;
			recipients = curl_slist_append(recipients, to.data());
		}
		for(auto cc : mConfig->getNotEmailCc()) {
         LOG(INFO) << "SMTP | Cc: " << cc;
			recipients = curl_slist_append(recipients, cc.data());
		}

		curl_easy_setopt(mCurl, CURLOPT_MAIL_RCPT, recipients);

		/* We're using a callback function to specify the payload (the headers and
		 * body of the message). You could just use the CURLOPT_READDATA option to
		 * specify a FILE pointer to read from. */
		mNextStep = DATE;
		curl_easy_setopt(mCurl, CURLOPT_READFUNCTION, CurlSmtp::_source);
		curl_easy_setopt(mCurl, CURLOPT_READDATA, this);
		curl_easy_setopt(mCurl, CURLOPT_UPLOAD, 1L);

		/* Send the message */
		CURLcode res = CURLE_OK;
		res = curl_easy_perform(mCurl);

		/* Check for errors */
		if (res != CURLE_OK) {
			LOG(ERROR) << "curl_easy_perform() failed: " << curl_easy_strerror(res);
		}

		/* Free the list of recipients */
		curl_slist_free_all(recipients);

		/* curl won't send the QUIT command until you call cleanup, so you should
		 * be able to re-use this connection for additional messages (setting
		 * CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
		 * curl_easy_perform() again. It may not be a good idea to keep the
		 * connection open for a very long time though (more than a few minutes
		 * may result in the server timing out the connection), and you do want to
		 * clean up in the end.
		 */
		curl_easy_cleanup (mCurl);
		mCurl = NULL;

		if (res != CURLE_OK) {
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

} // End namespace SS.

