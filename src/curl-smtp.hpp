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
 * \file   curl-smtp.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Mar 15, 2018
 *
 * \brief
 *
 ******************************************************************************/

#include <curl/curl.h>

#include "config.hpp"

#ifndef SRC_CURL_SMTP_HPP_
#define SRC_CURL_SMTP_HPP_

namespace SS {
/*
 * static const char *payload_text[] = {
 *     "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
 *     "To: " TO_MAIL "\r\n",
 *     "From: " FROM_MAIL "\r\n",
 *     "Cc: " CC_MAIL "\r\n",
 *     "Subject: SMTP example message\r\n",
 *     "\r\n", // empty line to divide headers from body, see RFC5322
 *     "The body of the message starts here.\r\n",
 *     "\r\n",
 *     "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
 *     "Check RFC5322.\r\n",
 *     NULL
 * };
 */

enum UploadNextStep {
	UNKNOWN = 0,
	DATE,
	TO,
	FROM,
	CC,
	SUBJECT,
	NEW_LINE_1,
	BODY,
	END
};

class CurlSmtp {
private:
	Config *mConfig;
	CURL *mCurl;
	uint32_t mNextStep;
   string mMessage;

	static size_t _source(void *ptr, size_t size, size_t nmemb, void *userp);
	size_t source(void *ptr, size_t size, size_t nmemb);

public:
	CurlSmtp(Config *config);
	~CurlSmtp();
	bool send(string &message);
};

} // End namespace SS.

#endif /* SRC_CURL_SMTP_HPP_ */
