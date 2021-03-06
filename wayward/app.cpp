#include "wayward/w.hpp"
#include "wayward/private.hpp"

#include <map>
#include <vector>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <sstream>
#include <iostream>
#include <regex>

namespace wayward {
  namespace {
    struct Handler {
      std::string human_readable_regex;
      std::regex regex;
      std::string path;
      std::function<Response(Request&)> handler;
      std::map<size_t, std::string> regex_group_names;

      bool match_and_set_params(Request& req) const {
        MatchResults match;
        if (std::regex_match(req.uri.path(), match, regex)) {
          for (auto& group_name: regex_group_names) {
            req.params[group_name.second] = match[group_name.first];
          }
          return true;
        }
        return false;
      }
    };
  }

  struct App::Private {
    App* app = nullptr;
    std::map<std::string, std::vector<Handler>> method_handlers;
    event_base* base = nullptr;
    evhttp* http = nullptr;

    Handler handler_for_path(std::string path, std::function<Response(Request&)> callback) {
      Handler handler;
      static const std::regex escape {"[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]"};
      static const std::string replacement = "\\\\\\1&";
      std::string escaped_path = std::regex_replace(path, escape, replacement);
      handler.path = std::move(path);

      static const std::regex find_placeholder {"/\\:([\\w\\d]+)(/?)"};
      static const std::string match_placeholder = "/(.+)";
      std::stringstream rs;
      size_t group_counter = 1;
      regex_replace_stream(rs, handler.path, find_placeholder, [&](std::ostream& os, const MatchResults& match) {
        os << match_placeholder;
        handler.regex_group_names[group_counter++] = match[1];
        if (match.size() >= 2) {
          os << match[2]; // Trailing '/'
        }
      });

      handler.human_readable_regex = rs.str();
      handler.regex = std::regex(handler.human_readable_regex);
      handler.handler = std::move(callback);
      return std::move(handler);
    }

    void add_handler(std::string path, std::function<Response(Request&)> handler, std::string method) {
      auto it = method_handlers.find(method);
      if (it == method_handlers.end()) {
        auto pair = method_handlers.insert(std::make_pair(std::move(method), std::vector<Handler>()));
        it = pair.first;
      }
      auto& handlers = it->second;
      handlers.push_back(handler_for_path(std::move(path), handler));
    }

    void handle_request(evhttp_request* req) {
      Request r = priv::make_request_from_evhttp_request(req);
      respond_to_request(r, req);
    }

    void respond_to_request(Request& req, evhttp_request* handle) {
      auto handlers_it = method_handlers.find(req.method);
      Handler* h = nullptr;
      if (handlers_it != method_handlers.end()) {
        for (auto& handler: handlers_it->second) {
          if (handler.match_and_set_params(req)) {
            h = &handler;
          }
        }
      }

      Response response;
      if (h) {
        response = h->handler(req);
      } else {
        response = wayward::not_found();
      }

      evkeyvalq* headers = evhttp_request_get_output_headers(handle);
      for (auto& pair: response.headers) {
        evhttp_add_header(headers, pair.first.c_str(), pair.second.c_str());
      }

      if (app->config.log_requests) {
        std::cout << req.method << " " << req.uri.path() << " " << (int)response.code << '\n';
      }

      evbuffer* body_buffer = evbuffer_new();
      evbuffer_add(body_buffer, response.body.c_str(), response.body.size());
      evhttp_send_reply(handle, (int)response.code, response.reason.size() ? response.reason.c_str() : nullptr, body_buffer);
      evbuffer_free(body_buffer);
    }
  };

  static void app_http_request_cb(evhttp_request* req, void* userdata) {
    App* app = static_cast<App*>(userdata);
    app->priv->handle_request(req);
  }

  App::App() : priv(new Private) {
    priv->app = this;
    priv->base = event_base_new();
    priv->http = evhttp_new(priv->base);
    evhttp_set_gencb(priv->http, app_http_request_cb, this);
  }

  App::~App() {
    evhttp_free(priv->http);
    event_base_free(priv->base);
  }

  int App::listen_and_serve(std::string address, int port) {
    int fd = evhttp_bind_socket(priv->http, address.c_str(), (u_short)port);
    if (fd < 0) return fd;
    return event_base_dispatch(priv->base);
  }

  void App::get(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "GET");
  }

  void App::put(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "PUT");
  }

  void App::post(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "POST");
  }

  void App::del(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "DELETE");
  }

  void App::head(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "HEAD");
  }

  void App::options(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "OPTIONS");
  }

  void App::print_routes() const {
    for (auto& method_handlers: priv->method_handlers) {
      for (auto& handler: method_handlers.second) {
        std::cout << method_handlers.first << " " << handler.path << "    " << handler.human_readable_regex << "\n";
      }
    }
  }
}
