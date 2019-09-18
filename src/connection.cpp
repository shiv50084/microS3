//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include "connection.hpp"
#include "sha1_hmac.hpp"
#include <ctime>

namespace us3 {

namespace {
std::string get_date_rfc2616_gmt() {
  // TODO(m): setlocale() is not guaranteed to be thread safe. Can we do this in a more thread safe
  // manner?

  // Set the locale to "C" (and save old locale).
  const char* old_locale_ptr = setlocale(LC_ALL, NULL);
  setlocale(LC_ALL, "C");

  // Get the current date & time.
  time_t now = time(0);
  struct tm tm = *gmtime(&now);

  // Format the date & time according to RFC2616.
  char buf[100];
  if (strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm) == 0) {
    buf[0] = '\0';
  }

  // Restore the old locale.
  setlocale(LC_ALL, old_locale_ptr);

  return std::string(&buf[0]);
}

std::string mode_to_http_method(const connection_t::mode_t mode) {
  if (mode == connection_t::WRITE) {
    return "PUT";
  } else {
    return "GET";
  }
}
}  // namespace

status::status_t connection_t::open(const char* host_name,
                                    const int port,
                                    const char* path,
                                    const char* access_key,
                                    const char* secret_key,
                                    const mode_t mode,
                                    const net::timeout_t timeout) {
  // Sanity check arguments.
  // Note: All pointers are guaranteed to be non-NULL at this point.
  if (port < 1 || port > 65535) {
    return status::INVALID_OPERATION;
  }

  // We must not open a connection that is already opened.
  if (m_mode != NONE) {
    return status::INVALID_OPERATION;
  }

  // Connect to the remote host.
  // TODO(m): Implement me!

  // Gather information for the HTTP request.
  const std::string http_method = mode_to_http_method(mode);
  const std::string content_type = "application/octet-stream";
  const std::string date_formatted = get_date_rfc2616_gmt();
  const std::string relative_path = std::string(path);

  // Generate a signature based on the request info and the S3 secret key.
  const std::string string_to_sign =
      http_method + "\n\n" + content_type + "\n" + date_formatted + "\n" + relative_path;
  const std::pair<sha1_hmac_t, status::status_t> digest =
      sha1_hmac(secret_key, string_to_sign.c_str());
  if (!is_success(digest)) {
    return digest.second;
  }
  const std::string signature = std::string(value(digest).c_str());

  // Construct the HTTP request header.
  std::string http_header;
  http_header += http_method + " " + std::string(path) + " HTTP/1.1";
  http_header += "\r\nHost: " + std::string(host_name);
  http_header += "\r\nContent-Type: " + content_type;
  http_header += "\r\nDate: " + date_formatted;
  http_header += "\r\nAuthorization: AWS " + std::string(access_key) + ":" + signature;
  http_header += "\r\n\r\n";

  // Send the HTTP header.
  // TODO(m): Implement me!
  return status::ERROR;
}

status::status_t connection_t::close() {
  // We can not close a connection that is already closed.
  if (m_mode == NONE) {
    return status::INVALID_OPERATION;
  }

  // TODO(m): Implement me!
  return status::ERROR;
}

status::status_t connection_t::read(void* buf,
                                    const size_t count,
                                    const net::timeout_t timeout,
                                    size_t& actual_count) {
  // The connection must have been opened in read mode.
  if (m_mode != READ) {
    return status::INVALID_OPERATION;
  }

  // TODO(m): Implement me!
  return status::ERROR;
}

status::status_t connection_t::write(const void* buf,
                                     const size_t count,
                                     const net::timeout_t timeout,
                                     size_t& actual_count) {
  // The connection must have been opened in write mode.
  if (m_mode != WRITE) {
    return status::INVALID_OPERATION;
  }

  // TODO(m): Implement me!
  return status::ERROR;
}

}  // namespace us3
