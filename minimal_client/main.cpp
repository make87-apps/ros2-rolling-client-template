// Copyright 2016 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <cinttypes>
#include <memory>
#include <cstdlib>   // For std::getenv
#include <string>    // For std::string
#include <iostream>  // For logging/debugging
#include <nlohmann/json.hpp> // JSON library
#include <cstdint>   // For uint64_t

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

using AddTwoInts = example_interfaces::srv::AddTwoInts;

namespace make87 {
std::string sanitize_and_checksum(const std::string& input) {
    const std::string prefix = "ros2_";

    // Sanitize the input string
    std::string sanitized;
    sanitized.reserve(input.size());
    for (char c : input) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }

    // Compute checksum
    uint64_t sum = 0;
    for (unsigned char b : input) {
        sum = (sum * 31 + b) % 1000000007ULL;
    }
    std::string checksum = std::to_string(sum);

    // Calculate maximum allowed length for the sanitized string
    const size_t max_total_length = 256;
    const size_t prefix_length = prefix.size();
    const size_t checksum_length = checksum.size();
    size_t max_sanitized_length = max_total_length - prefix_length - checksum_length;

    // Truncate sanitized string if necessary
    if (sanitized.size() > max_sanitized_length) {
        sanitized = sanitized.substr(0, max_sanitized_length);
    }

    // Construct the final string
    return prefix + sanitized + checksum;
}

std::string resolve_endpoint_name(const std::string& search_endpoint, const std::string& default_value) {
    const char* env_value = std::getenv("ENDPOINTS");
    if (!env_value) {
        std::cerr << "Environment variable ENDPOINTS not set. Using default value.\n";
        return default_value;
    }

    try {
        nlohmann::json json_obj = nlohmann::json::parse(env_value);
        if (json_obj.contains("endpoints") && json_obj["endpoints"].is_array()) {
            for (const auto& endpoint : json_obj["endpoints"]) {
                if (endpoint.contains("endpoint_name") && endpoint["endpoint_name"] == search_endpoint) {
                    if (endpoint.contains("endpoint_key") && endpoint["endpoint_key"].is_string()) {
                        return sanitize_and_checksum(endpoint["endpoint_key"].get<std::string>());
                    }
                }
            }
        }
        std::cerr << "Endpoint " << search_endpoint << " not found or missing endpoint_key. Using default value.\n";
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing ENDPOINTS: " << e.what() << ". Using default value.\n";
    }

    return default_value;
}
}


int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("minimal_client");
  auto client = node->create_client<AddTwoInts>(make87::resolve_endpoint_name("REQUESTER_ENDPOINT", "add_two_ints"));
  while (!client->wait_for_service(std::chrono::seconds(1))) {
    if (!rclcpp::ok()) {
      RCLCPP_ERROR(node->get_logger(), "client interrupted while waiting for service to appear.");
      return 1;
    }
    RCLCPP_INFO(node->get_logger(), "waiting for service to appear...");
  }
  auto request = std::make_shared<AddTwoInts::Request>();
  request->a = 41;
  request->b = 1;
  auto result_future = client->async_send_request(request);
  if (rclcpp::spin_until_future_complete(node, result_future) !=
    rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_ERROR(node->get_logger(), "service call failed :(");
    client->remove_pending_request(result_future);
    return 1;
  }
  auto result = result_future.get();
  RCLCPP_INFO(
    node->get_logger(), "result of %" PRId64 " + %" PRId64 " = %" PRId64,
    request->a, request->b, result->sum);
  rclcpp::shutdown();
  return 0;
}
