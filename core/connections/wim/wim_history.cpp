#include "stdafx.h"

#include "../../archive/history_message.h"

#include "../../log/log.h"

#include "wim_history.h"

using namespace core;
using namespace wim;

namespace
{
    void apply_persons(InOut archive::history_message_sptr &_message, const archive::persons_map &_persons);
}

archive::persons_map core::wim::parse_persons(const rapidjson::Value &_node)
{
    archive::persons_map persons;
    auto iter_persons = _node.FindMember("persons");
    if (iter_persons != _node.MemberEnd() && iter_persons->value.IsArray() && !iter_persons->value.Empty())
    {
        for (auto iter_person = iter_persons->value.Begin(); iter_person != iter_persons->value.End(); ++iter_person)
        {
            auto iter_sn = iter_person->FindMember("sn");
            auto iter_friendly = iter_person->FindMember("friendly");
            auto iter_official = iter_person->FindMember("official");

            if (iter_sn == iter_person->MemberEnd() || !iter_sn->value.IsString() || iter_friendly == iter_person->MemberEnd() || !iter_friendly->value.IsString())
            {
                //assert(!"invalid person");
                continue;
            }

            archive::person p;
            p.friendly_ = rapidjson_get_string(iter_friendly->value);
            if (iter_official != iter_person->MemberEnd() && iter_official->value.IsUint())
                p.official_ = iter_official->value.GetUint() == 1;

            std::string sn = rapidjson_get_string(iter_sn->value);
            boost::replace_last(sn, "@uin.icq", std::string());

            persons.emplace(std::move(sn), std::move(p));
        }
    }
    return persons;
}

bool core::wim::parse_history_messages_json(
    const rapidjson::Value &_node,
    const int64_t _older_msg_id,
    const std::string &_sender_aimid,
    Out archive::history_block &_block,
    const archive::persons_map& _persons,
    message_order _order)
{
    assert(!_sender_aimid.empty());

    auto iter_messages = _node.FindMember("messages");
    if (iter_messages == _node.MemberEnd())
        return true;

    if (!iter_messages->value.IsArray())
        return false;

    if (iter_messages->value.Empty())
        return true;

    auto prev_msg_id = _older_msg_id;

    _block.reserve(iter_messages->value.Size());

    const bool is_reverse = message_order::reverse == _order;

    auto iter_message = is_reverse ? iter_messages->value.End() : iter_messages->value.Begin();
    const auto end = is_reverse ? iter_messages->value.Begin() : iter_messages->value.End();
    do
    {
        if (is_reverse)
            --iter_message;

        auto msg = std::make_shared<archive::history_message>();

        if (0 != msg->unserialize(*iter_message, _sender_aimid))
        {
            assert(!"parse message error");
        }
        else
        {
            assert(!msg->is_patch());

            const auto is_same_as_prev = (prev_msg_id == msg->get_msgid());
            assert(!is_same_as_prev);

            if (is_same_as_prev)
            {
                // workaround for the server issue

                __INFO(
                    "delete_history",
                    "server issue detected, message skipped\n"
                    "    older_msg_id=<%1%>\n"
                    "    prev_msg_id=<%2%>\n"
                    "    msg_id=<%3%>",
                    _older_msg_id % prev_msg_id % msg->get_msgid()
                );

                continue;
            }

            msg->set_prev_msgid(prev_msg_id);
            _block.push_back(msg);

            prev_msg_id = msg->get_msgid();
        }

        apply_persons(msg, _persons);
        if (!is_reverse)
            ++iter_message;
    }
    while (iter_message != end);

    return true;
}

namespace
{
    void apply_persons(InOut archive::history_message_sptr &_message, const archive::persons_map &_persons)
    {
        assert(_message);

        if (!_persons.empty())
        {
            _message->set_sender_friendly(_persons.begin()->second.friendly_);
        }

        auto chat_info = _message->get_chat_data();
        if (chat_info)
        {
            chat_info->apply_persons(_persons);
        }

        auto &chat_event = _message->get_chat_event_data();
        if (chat_event)
        {
            chat_event->apply_persons(_persons);
        }

        auto &voip = _message->get_voip_data();
        if (voip)
        {
            voip->apply_persons(_persons);
        }

        _message->apply_persons_to_quotes(_persons);
        _message->apply_persons_to_mentions(_persons);
    }
}