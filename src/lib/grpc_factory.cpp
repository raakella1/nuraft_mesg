///
// Copyright 2018 (c) eBay Corporation
//
// Authors:
//      Brian Szmyd <bszmyd@ebay.com>
//
// Brief:
//   grpc_factory static functions that makes for easy client creation.
//

#include <sds_grpc/client.h>

#include "grpc_client.hpp"
#include "grpc_factory.hpp"

namespace raft_core {

template<typename Payload>
struct client_ctx {
   int32_t              _cur_dest;
   std::string const    _new_srv_addr;

   client_ctx(Payload payload, shared<grpc_factory> factory, int32_t dest, std::string const& new_srv_addr = "") :
      _cur_dest(dest),
      _new_srv_addr(new_srv_addr),
      _payload(payload),
      _cli_factory(factory)
   { }

   Payload payload() const                      { return _payload; }
   shared<grpc_factory> cli_factory() const     { return _cli_factory; }
   std::future<bool> future()                   { return _promise.get_future(); }
   void set(bool const success)                 { return _promise.set_value(success); }

 private:
   Payload const        _payload;
   shared<grpc_factory> _cli_factory;
   std::promise<bool>   _promise;
};

template<typename PayloadType>
shared<cstn::req_msg>
createMessage(PayloadType payload, std::string const& srv_addr = "");

template<>
shared<cstn::req_msg>
createMessage(uint32_t const srv_id, std::string const& srv_addr) {
   assert(!srv_addr.empty());
   auto srv_conf = cstn::srv_config(srv_id, srv_addr);
   auto log = std::make_shared<cstn::log_entry>(
      0,
      srv_conf.serialize(),
      cstn::log_val_type::cluster_server
      );
   auto msg = std::make_shared<cstn::req_msg>(0, cstn::msg_type::add_server_request, 0, 0, 0, 0, 0);
   msg->log_entries().push_back(log);
   return msg;
}

template<>
shared<cstn::req_msg>
createMessage(shared<cstn::buffer> buf, std::string const& ) {
   auto log = std::make_shared<cstn::log_entry>(0, buf);
   auto msg = std::make_shared<cstn::req_msg>(0, cstn::msg_type::client_request, 0, 1, 0, 0, 0);
   msg->log_entries().push_back(log);
   return msg;
}

template<>
shared<cstn::req_msg>
createMessage(int32_t const srv_id, std::string const& ) {
    auto buf = cstn::buffer::alloc(sizeof(srv_id));
    buf->put(srv_id);
    buf->pos(0);
    auto log = std::make_shared<cstn::log_entry>(0, buf, cstn::log_val_type::cluster_server);
    auto msg = std::make_shared<cstn::req_msg>(0, cstn::msg_type::remove_server_request, 0, 0, 0, 0, 0);
    msg->log_entries().push_back(log);
    return msg;
}

template<typename ContextType>
void
respHandler(shared<ContextType> ctx,
            shared<cstn::resp_msg>& rsp,
            shared<cstn::rpc_exception>& err) {
   auto factory = ctx->cli_factory();
   if (err) {
      LOGERROR("{}", err->what());
      ctx->set(false);
      return;
   } else if (rsp->get_accepted()) {
      LOGDEBUGMOD(raft_core, "Accepted response");
      ctx->set(true);
      return;
   } else if (ctx->_cur_dest == rsp->get_dst()) {
      LOGWARN("Request ignored");
      ctx->set(false);
      return;
   } else if (0 > rsp->get_dst()) {
      LOGWARN("No known leader!");
      ctx->set(false);
      return;
   }

   // Not accepted: means that `get_dst()` is a new leader.
   auto gresp = std::dynamic_pointer_cast<grpc_resp>(rsp);
   LOGDEBUGMOD(raft_core, "Updating destination from {} to {}[{}]",
               ctx->_cur_dest,
               rsp->get_dst(),
               gresp->dest_addr);
   ctx->_cur_dest = rsp->get_dst();
   auto client = factory->create_client(gresp->dest_addr);

   // We'll try again by forwarding the message
   auto handler = static_cast<cstn::rpc_handler>([ctx] (shared<cstn::resp_msg>& rsp,
                                                        shared<cstn::rpc_exception>& err) {
         respHandler(ctx, rsp, err);
      });

   LOGDEBUGMOD(raft_core, "Creating new message: {}", ctx->_new_srv_addr);
   auto msg = createMessage(ctx->payload(), ctx->_new_srv_addr);
   client->send(msg, handler);
}

grpc_factory::grpc_factory(int const cli_thread_count, std::string const& name) :
    rpc_client_factory(),
    _worker_name(name)
{
    if (0 < cli_thread_count) {
        if (!sds::grpc::GrpcAyncClientWorker::create_worker(_worker_name.data(), cli_thread_count)) {
            throw std::system_error(ENOTCONN, std::generic_category(), "Failed to create workers");
        }
    }
}

cstn::ptr<cstn::rpc_client>
grpc_factory::create_client(const std::string &client) {
    cstn::ptr<cstn::rpc_client> new_client;;

    // Protected section
    { std::lock_guard<std::mutex> lk(_client_lock);
    auto [it, happened] = _clients.emplace(client, nullptr);
    if (_clients.end() != it) {
        if (!happened) {
            LOGDEBUGMOD(raft_core, "Re-creating client for {}", client);
            if (auto err = reinit_client(it->second); err) {
                LOGERROR("Failed to re-initialize client {}: {}", client, err.message());
            } else {
                new_client = it->second;
            }
        } else {
            if (auto err = create_client(client, it->second); err) {
                LOGERROR("Failed to create client for {}: {}", client, err.message());
            }  else {
                new_client = it->second;
            }
        }
    }
    } // End of Protected section
    return new_client;
}

std::future<bool>
grpc_factory::add_server(uint32_t const srv_id, std::string const& srv_addr, cstn::srv_config const& dest_cfg) {
   auto client = create_client(dest_cfg.get_endpoint());
   assert(client);
   if (!client) {
      std::promise<bool> p;
      p.set_value(false);
      return p.get_future();
   }

   auto ctx = std::make_shared<client_ctx<uint32_t>>(srv_id, shared_from_this(), dest_cfg.get_id(), srv_addr);
   auto handler = static_cast<cstn::rpc_handler>([ctx] (shared<cstn::resp_msg>& rsp,
                                                        shared<cstn::rpc_exception>& err) {
         respHandler(ctx, rsp, err);
      });

   auto msg = createMessage(srv_id, srv_addr);
   client->send(msg, handler);
   return ctx->future();
}

std::future<bool>
grpc_factory::rem_server(uint32_t const srv_id, cstn::srv_config const& dest_cfg) {
   auto client = create_client(dest_cfg.get_endpoint());
   assert(client);
   if (!client) {
      std::promise<bool> p;
      p.set_value(false);
      return p.get_future();
   }

   auto ctx = std::make_shared<client_ctx<int32_t>>(srv_id, shared_from_this(), dest_cfg.get_id());
   auto handler = static_cast<cstn::rpc_handler>([ctx] (shared<cstn::resp_msg>& rsp,
                                                        shared<cstn::rpc_exception>& err) {
         respHandler(ctx, rsp, err);
      });

   auto msg = createMessage(static_cast<int32_t>(srv_id));
   client->send(msg, handler);
   return ctx->future();
}

std::future<bool>
grpc_factory::client_request(shared<cstn::buffer> buf, cstn::srv_config const& dest_cfg) {
   auto client = create_client(dest_cfg.get_endpoint());
   assert(client);
   if (!client) {
      std::promise<bool> p;
      p.set_value(false);
      return p.get_future();
   }

   auto ctx = std::make_shared<client_ctx<shared<cstn::buffer>>>(buf, shared_from_this(), dest_cfg.get_id());
   auto handler = static_cast<cstn::rpc_handler>([ctx] (shared<cstn::resp_msg>& rsp,
                                                        shared<cstn::rpc_exception>& err) {
         respHandler(ctx, rsp, err);
      });

   auto msg = createMessage(buf);
   client->send(msg, handler);
   return ctx->future();
}

}
