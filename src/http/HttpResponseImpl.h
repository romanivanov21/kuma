/* Copyright (c) 2014, Fengping Bao <jamol@live.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __HttpResponseImpl_H__
#define __HttpResponseImpl_H__

#include "kmdefs.h"
#include "HttpParserImpl.h"
#include "TcpSocketImpl.h"
#include "Uri.h"

KUMA_NS_BEGIN

class HttpResponseImpl
{
public:
    typedef std::function<void(uint8_t*, uint32_t)> DataCallback;
    typedef std::function<void(int)> EventCallback;
    typedef std::function<void(void)> HttpEventCallback;
    
    HttpResponseImpl(EventLoopImpl* loop);
    ~HttpResponseImpl();
    
    int attachFd(SOCKET_FD fd, uint32_t flags, uint8_t* init_data = nullptr, uint32_t init_len = 0);
    int attachSocket(TcpSocketImpl&& tcp, HttpParserImpl&& parser);
    void addHeader(const std::string& name, const std::string& value);
    void addHeader(const std::string& name, uint32_t value);
    int sendResponse(int status_code, const std::string& desc = nullptr, const std::string& ver = "HTTP/1.1");
    int sendData(const uint8_t* data, uint32_t len);
    void reset(); // reset for connection reuse
    int close();
    
    const std::string& getMethod() const { return http_parser_.getMethod(); }
    const std::string& getUrl() const { return http_parser_.getUrl(); }
    const std::string& getVersion() const { return http_parser_.getVersion(); }
    const std::string& getParamValue(const char* name) const { return http_parser_.getParamValue(name); }
    const std::string& getHeaderValue(const char* name) const { return http_parser_.getHeaderValue(name); }
    void forEachHeader(HttpParserImpl::EnumrateCallback& cb) { return http_parser_.forEachHeader(cb); }
    void forEachHeader(HttpParserImpl::EnumrateCallback&& cb) { return http_parser_.forEachHeader(cb); }
    
    void setDataCallback(const DataCallback& cb) { cb_data_ = cb; }
    void setWriteCallback(const EventCallback& cb) { cb_write_ = cb; }
    void setErrorCallback(const EventCallback& cb) { cb_error_ = cb; }
    void setHeaderCompleteCallback(const HttpEventCallback& cb) { cb_header_ = cb; }
    void setRequestCompleteCallback(const HttpEventCallback& cb) { cb_request_ = cb; }
    void setResponseCompleteCallback(const HttpEventCallback& cb) { cb_response_ = cb; }
    void setDataCallback(DataCallback&& cb) { cb_data_ = std::move(cb); }
    void setWriteCallback(EventCallback&& cb) { cb_write_ = std::move(cb); }
    void setErrorCallback(EventCallback&& cb) { cb_error_ = std::move(cb); }
    void setHeaderCompleteCallback(HttpEventCallback&& cb) { cb_header_ = std::move(cb); }
    void setRequestCompleteCallback(HttpEventCallback&& cb) { cb_request_ = std::move(cb); }
    void setResponseCompleteCallback(HttpEventCallback&& cb) { cb_response_ = std::move(cb); }

protected: // callbacks of tcp_socket
    void onSend(int err);
    void onReceive(int err);
    void onClose(int err);
    
protected:
    const char* getObjKey() const;
    
private:
    enum State {
        STATE_IDLE,
        STATE_RECVING_REQUEST,
        STATE_WAIT_FOR_RESPONSE,
        STATE_SENDING_HEADER,
        STATE_SENDING_BODY,
        STATE_COMPLETE,
        STATE_ERROR,
        STATE_CLOSED
    };
    void setState(State state) { state_ = state; }
    State getState() const { return state_; }
    void buildResponse(int status_code, const std::string& desc, const std::string& ver);
    int sendChunk(const uint8_t* data, uint32_t len);
    void cleanup();
    
    void onHttpData(const char* data, uint32_t len);
    void onHttpEvent(HttpEvent ev);
    void notifyComplete();
    
private:
    HttpParserImpl          http_parser_;
    State                   state_;
    EventLoopImpl*          loop_;
    
    uint8_t*                init_data_;
    uint32_t                init_len_;
    
    std::vector<uint8_t>    send_buffer_;
    uint32_t                send_offset_;
    TcpSocketImpl           tcp_socket_;
    
    HttpParserImpl::STRING_MAP  header_map_;
    
    bool                    is_chunked_;
    bool                    has_content_length_;
    uint32_t                content_length_;
    uint32_t                body_bytes_sent_;
    
    DataCallback            cb_data_;
    EventCallback           cb_write_;
    EventCallback           cb_error_;
    HttpEventCallback       cb_header_;
    HttpEventCallback       cb_request_;
    HttpEventCallback       cb_response_;
    
    bool*                   destroy_flag_ptr_;
};

KUMA_NS_END

#endif
