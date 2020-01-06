/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CUERPC_SERVER_HPP_
#define CUERPC_SERVER_HPP_

#include <thread>
#include <functional>
#include <vector>
#include <type_traits>
#include <boost/asio.hpp>

#include "cuerpc/msgpack.hpp"
#include "cuerpc/detail/session.hpp"
#include "cuerpc/detail/noncopyable.hpp"
#include "cuerpc/detail/engines.hpp"
#include "cuerpc/detail/dispatcher.hpp"

namespace cue {
namespace rpc {

class server final : safe_noncopyable {
public:
    server() noexcept : engines_{detail::engines::default_engines()} {
    }

    virtual ~server() noexcept = default;

    server(server&& rhs) noexcept : engines_{detail::engines::default_engines()} {
        swap(rhs);
    }

    server& operator=(server&& rhs) noexcept {
        swap(rhs);
        return *this;
    }

    void swap(server& rhs) noexcept {
        if (this != std::addressof(rhs)) {
            std::swap(acceptor_, rhs.acceptor_);
        }
    }

    template <typename Name, typename... Args>
    server& serve(Name&& name, Args&&... args) {
        detail::dispatcher::instance().serve(std::forward<Name>(name), std::forward<Args>(args)...);
        return *this;
    }

    server& listen(unsigned short port) {
        assert(port != 0);
        listen_impl(boost::asio::ip::tcp::resolver::query{std::to_string(port)});
        return *this;
    }

    template <typename Host>
    server& listen(unsigned short port, Host&& host) {
        assert(port != 0);
        listen_impl(boost::asio::ip::tcp::resolver::query{std::forward<Host>(host), std::to_string(port)});
        return *this;
    }

    void run() {
        engines_.run();
    }

protected:
    void listen_impl(boost::asio::ip::tcp::resolver::query&& query) {
        const boost::asio::ip::tcp::endpoint endpoint{*boost::asio::ip::tcp::resolver{engines_.get()}.resolve(query)};
        acceptor_ = std::make_shared<boost::asio::ip::tcp::acceptor>(engines_.get());
        acceptor_->open(endpoint.protocol());
        acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address{true});
        acceptor_->bind(endpoint);
        acceptor_->listen();
        do_accept();
    }

    void do_accept() {
        static const auto handler = [](std::shared_ptr<detail::session> session, std::shared_ptr<detail::request> req) {
            detail::dispatcher::instance().dispatch(session, req);
        };
        auto session = std::make_shared<detail::session>(handler, this->engines_.get());
        this->acceptor_->async_accept(session->socket(), [this, session](boost::system::error_code code) {
            if (!this->acceptor_->is_open()) {
                return;
            }

            if (!code) {
                session->socket().set_option(boost::asio::ip::tcp::no_delay{true});
                session->start();
            }

            this->do_accept();
        });
    }

    detail::engines& engines_;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
};

} // namespace rpc
} // namespace cue

#endif // CUERPC_SERVER_HPP_
