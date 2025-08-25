// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#ifndef HTTPPARSER_H_
#define HTTPPARSER_H_

#include <iostream>
#include <map>
#include <string>
#include "http_request.h"
#include "http_response.h"

namespace mevacoin
{

    // Blocking HttpParser
    class HttpParser
    {
    public:
        HttpParser(){};

        void receiveRequest(std::istream &stream, HttpRequest &request);
        void receiveResponse(std::istream &stream, HttpResponse &response);
        static HttpResponse::HTTP_STATUS parseResponseStatusFromString(const std::string &status);

    private:
        void readWord(std::istream &stream, std::string &word);
        void readHeaders(std::istream &stream, HttpRequest::Headers &headers);
        bool readHeader(std::istream &stream, std::string &name, std::string &value);
        size_t getBodyLen(const HttpRequest::Headers &headers);
        void readBody(std::istream &stream, std::string &body, const size_t bodyLen);
    };

} // namespace mevacoin

#endif /* HTTPPARSER_H_ */
