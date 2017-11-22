#ifndef __WIM_GET_FLAGS_H_
#define __WIM_GET_FLAGS_H_

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class get_flags : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            int32_t flags_;

        public:

            explicit get_flags(wim_packet_params _params);

            int32_t flags() const;

            virtual ~get_flags();
        };

    }

}


#endif// __WIM_GET_FLAGS_H_
