// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#ifndef YB_YQL_REDIS_REDISSERVER_REDIS_COMMANDS_H
#define YB_YQL_REDIS_REDISSERVER_REDIS_COMMANDS_H

#include <functional>
#include <string>

#include <boost/function.hpp>

#include "yb/client/client_fwd.h"

#include "yb/rpc/service_if.h"

#include "yb/yql/redis/redisserver/redis_fwd.h"
#include "yb/yql/redis/redisserver/redis_server.h"


namespace yb {
namespace redisserver {

typedef boost::function<void(const Status&)> StatusFunctor;

// Context for batch of Redis commands.
class BatchContext : public RefCountedThreadSafe<BatchContext> {
 public:
  virtual const std::shared_ptr<client::YBTable>& table() const = 0;
  virtual const RedisClientCommand& command(size_t idx) const = 0;
  virtual const std::shared_ptr<RedisInboundCall>& call() const = 0;
  virtual const std::shared_ptr<client::YBClient>& client() const = 0;
  virtual const RedisServer* server() = 0;

  virtual void Apply(
      size_t index,
      std::shared_ptr<client::YBRedisReadOp> operation,
      const rpc::RpcMethodMetrics& metrics) = 0;

  virtual void Apply(
      size_t index,
      std::shared_ptr<client::YBRedisWriteOp> operation,
      const rpc::RpcMethodMetrics& metrics) = 0;

  virtual void Apply(
      size_t index,
      std::function<bool(const StatusFunctor&)> functor,
      std::string partition_key,
      const rpc::RpcMethodMetrics& metrics) = 0;

  virtual ~BatchContext() {}
};

typedef scoped_refptr<BatchContext> BatchContextPtr;

// Information about RedisCommand(s) that we support.
struct RedisCommandInfo {
  std::string name;
  // The following arguments should be passed to this functor:
  // Info about its command.
  // Index of call in batch.
  // Batch context.
  std::function<void(const RedisCommandInfo&,
                     size_t,
                     BatchContext*)> functor;
  // Positive arity means that we expect exactly arity-1 arguments and negative arity means
  // that we expect at least -arity-1 arguments.
  int arity;
  yb::rpc::RpcMethodMetrics metrics;
};

typedef std::shared_ptr<RedisCommandInfo> RedisCommandInfoPtr;

void RespondWithFailure(
    std::shared_ptr<RedisInboundCall> call,
    size_t idx,
    const std::string& error);

void FillRedisCommands(const scoped_refptr<MetricEntity>& metric_entity,
                       const std::function<void(const RedisCommandInfo& info)>& setup_method);

#define YB_REDIS_METRIC(name) \
    BOOST_PP_CAT(METRIC_handler_latency_yb_redisserver_RedisServerService_, name)

#define DEFINE_REDIS_histogram_EX(name_identifier, label_str, desc_str) \
  METRIC_DEFINE_histogram( \
      server, BOOST_PP_CAT(handler_latency_yb_redisserver_RedisServerService_, name_identifier), \
      (label_str), yb::MetricUnit::kMicroseconds, \
      "Microseconds spent handling " desc_str " RPC requests", \
      60000000LU, 2)

#define DEFINE_REDIS_histogram(name_identifier, capitalized_name_str) \
  DEFINE_REDIS_histogram_EX( \
      name_identifier, \
      "yb.redisserver.RedisServerService." BOOST_STRINGIZE(name_identifier) " RPC Time", \
      "yb.redisserver.RedisServerService." capitalized_name_str "Command()")

} // namespace redisserver
} // namespace yb

#endif // YB_YQL_REDIS_REDISSERVER_REDIS_COMMANDS_H
