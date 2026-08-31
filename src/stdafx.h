#pragma once

// Стандартная библиотека
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <filesystem>
#include <queue>

// Boost
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/strand.hpp>


//nlohmann
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;