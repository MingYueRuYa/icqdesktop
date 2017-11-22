#ifndef __WIM_HISTORY_H_
#define __WIM_HISTORY_H_

#pragma once


namespace core
{
    namespace archive
    {
        struct person
        {
            person()
                : official_(false)
            {
            }

            std::string friendly_;
            bool official_;
        };
        class history_message;
        typedef std::vector<std::shared_ptr<history_message>> history_block;
        typedef std::map<std::string, person> persons_map;
    }

    namespace wim
    {
        archive::persons_map parse_persons(const rapidjson::Value &_node);

        enum class message_order
        {
            direct,
            reverse
        };

        bool parse_history_messages_json(
            const rapidjson::Value &_node,
            const int64_t _older_msg_id,
            const std::string &_sender_aimid,
            Out archive::history_block &_block,
            const archive::persons_map& _persons,
            message_order _order);
    }
}


#endif //__WIM_HISTORY_H_