#ifndef __WIM_SEND_MESSAGE_H_
#define __WIM_SEND_MESSAGE_H_

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace archive
    {
        class quote;
        typedef std::vector<quote> quotes_vec;
        typedef std::map<std::string, std::string> mentions_map;
    }
}


namespace core
{
    enum class message_type;

    namespace wim
    {
        class send_message : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

            const message_type type_;

            std::string aimid_;
            std::string message_text_;

            uint32_t sms_error_;
            uint32_t sms_count_;

            std::string wim_msg_id_;
            std::string internal_id_;

            int64_t hist_msg_id_;
            int64_t before_hist_msg_id;

            bool duplicate_;

            core::archive::quotes_vec quotes_;
            core::archive::mentions_map mentions_;

        public:

            uint32_t get_sms_error() const { return sms_error_; }
            uint32_t get_sms_count() const { return sms_count_; }

            const std::string& get_wim_msg_id() const { return wim_msg_id_; }
            const std::string& get_internal_id() const { return internal_id_; }

            int64_t get_hist_msg_id() const { return hist_msg_id_; }
            int64_t get_before_hist_msg_id() const { return before_hist_msg_id; }

            bool is_duplicate() const { return duplicate_; }

            send_message(
                wim_packet_params _params,
                const message_type _type,
                const std::string& _internal_id,
                const std::string& _aimid,
                const std::string& _message_text,
                const core::archive::quotes_vec& _quotes,
                const core::archive::mentions_map& _mentions);

            virtual ~send_message();
        };

    }

}


#endif// __WIM_SEND_MESSAGE_H_
