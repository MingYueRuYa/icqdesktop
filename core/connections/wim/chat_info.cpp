#include "stdafx.h"
#include "chat_info.h"

using namespace core;
using namespace wim;

chat_info::chat_info()
    : create_time_(0)
    , members_count_(0)
    , friend_count_(0)
    , blocked_count_(0)
    , pending_count_(0)
    , you_blocked_(false)
    , you_pending_(false)
    , you_member_(false)
    , public_(false)
    , live_(false)
    , controlled_(false)
    , joinModeration_(false)
    , age_restriction_(false)
{
}

int32_t chat_info::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("sn");
    if (iter_aimid != _node.MemberEnd() && iter_aimid->value.IsString())
        aimid_ = rapidjson_get_string(iter_aimid->value);

    auto iter_name = _node.FindMember("name");
    if (iter_name != _node.MemberEnd() && iter_name->value.IsString())
        name_ = rapidjson_get_string(iter_name->value);

    auto iter_location = _node.FindMember("location");
    if (iter_location != _node.MemberEnd() && iter_location->value.IsString())
        location_ = rapidjson_get_string(iter_location->value);

    auto iter_stamp = _node.FindMember("stamp");
    if (iter_stamp != _node.MemberEnd() && iter_stamp->value.IsString())
        stamp_ = rapidjson_get_string(iter_stamp->value);

    auto iter_about = _node.FindMember("about");
    if (iter_about != _node.MemberEnd() && iter_about->value.IsString())
        about_ = rapidjson_get_string(iter_about->value);

    auto iter_create_time = _node.FindMember("createTime");
    if (iter_create_time != _node.MemberEnd() && iter_create_time->value.IsInt())
        create_time_ = iter_create_time->value.GetInt();

    auto iter_members_count = _node.FindMember("membersCount");
    if (iter_members_count != _node.MemberEnd() && iter_members_count->value.IsInt())
        members_count_ = iter_members_count->value.GetInt();

    auto iter_friends_count = _node.FindMember("friendsCount");
    if (iter_friends_count != _node.MemberEnd() && iter_friends_count->value.IsInt())
        friend_count_ = iter_friends_count->value.GetInt();

    auto iter_join_moderation = _node.FindMember("joinModeration");
    if (iter_join_moderation != _node.MemberEnd() && iter_join_moderation->value.IsBool())
        joinModeration_ = iter_join_moderation->value.GetBool();

    auto iter_age_restriction = _node.FindMember("ageRestriction");
    if (iter_age_restriction != _node.MemberEnd() && iter_age_restriction->value.IsBool())
        age_restriction_ = iter_age_restriction->value.GetBool();

    auto iter_yours = _node.FindMember("you");
    if (iter_yours != _node.MemberEnd())
    {
        auto iter_your_role = iter_yours->value.FindMember("role");
        if (iter_your_role != iter_yours->value.MemberEnd() && iter_your_role->value.IsString())
        {
            your_role_ = rapidjson_get_string(iter_your_role->value);
            you_member_ = true;
        }
        else
        {
            you_member_ = false;
        }

        auto iter_you_blocked = iter_yours->value.FindMember("blocked");
        if (iter_you_blocked != iter_yours->value.MemberEnd() && iter_you_blocked->value.IsBool())
            you_blocked_ = iter_you_blocked->value.GetBool();

        auto iter_you_pending_ = iter_yours->value.FindMember("pending");
        if (iter_you_pending_ != iter_yours->value.MemberEnd() && iter_you_pending_->value.IsBool())
            you_pending_ = iter_you_pending_->value.GetBool();
    }

    auto iter_public = _node.FindMember("public");
    if (iter_public != _node.MemberEnd() && iter_public->value.IsBool())
        public_ = iter_public->value.GetBool();

    auto iter_live = _node.FindMember("live");
    if (iter_live != _node.MemberEnd() && iter_live->value.IsBool())
        live_ = iter_live->value.GetBool();

    auto iter_controlled = _node.FindMember("controlled");
    if (iter_controlled != _node.MemberEnd() && iter_controlled->value.IsBool())
        controlled_ = iter_controlled->value.GetBool();

    auto iter_blocked_count = _node.FindMember("blockedCount");
    if (iter_blocked_count != _node.MemberEnd() && iter_blocked_count->value.IsInt())
        blocked_count_ = iter_blocked_count->value.GetInt();

    auto iter_pending_count = _node.FindMember("pendingCount");
    if (iter_pending_count != _node.MemberEnd() && iter_pending_count->value.IsInt())
        pending_count_ = iter_pending_count->value.GetInt();

    auto iter_members_version = _node.FindMember("membersVersion");
    if (iter_members_version != _node.MemberEnd() && iter_members_version->value.IsString())
        members_version_ = rapidjson_get_string(iter_members_version->value);

    auto iter_info_version = _node.FindMember("infoVersion");
    if (iter_info_version != _node.MemberEnd() && iter_info_version->value.IsString())
        info_version_ = rapidjson_get_string(iter_info_version->value);

    auto iter_info_creator = _node.FindMember("creator");
    if (iter_info_creator != _node.MemberEnd() && iter_info_creator->value.IsString())
        creator_ = rapidjson_get_string(iter_info_creator->value);

    auto iter_info_default_role = _node.FindMember("defaultRole");
    if (iter_info_default_role != _node.MemberEnd() && iter_info_default_role->value.IsString())
        default_role_ = rapidjson_get_string(iter_info_default_role->value);

    auto iter_members = _node.FindMember("members");
    if (iter_members != _node.MemberEnd() && iter_members->value.IsArray())
    {
        for (auto iter = iter_members->value.Begin(), end = iter_members->value.End(); iter != end; ++iter)
        {
            chat_member_info member_info;
            auto iter_aimid = iter->FindMember("sn");
            if (iter_aimid != iter->MemberEnd() && iter_aimid->value.IsString())
                member_info.aimid_ = rapidjson_get_string(iter_aimid->value);

            auto iter_role = iter->FindMember("role");
            if (iter_role != iter->MemberEnd() && iter_role->value.IsString())
                member_info.role_ = rapidjson_get_string(iter_role->value);

            auto iter_friend = iter->FindMember("friend");
            if (iter_friend != iter->MemberEnd() && iter_friend->value.IsBool())
                member_info.friend_ = iter_friend->value.GetBool();

            auto iter_no_avatar = iter->FindMember("noAvatar");
            if (iter_no_avatar != iter->MemberEnd() && iter_no_avatar->value.IsBool())
                member_info.no_avatar_ = iter_no_avatar->value.GetBool();

            auto iter_anketa = iter->FindMember("anketa");
            if (iter_anketa != iter->MemberEnd())
            {
                auto iter_first_name = iter_anketa->value.FindMember("firstName");
                if (iter_first_name != iter_anketa->value.MemberEnd() && iter_first_name->value.IsString())
                    member_info.first_name_ = rapidjson_get_string(iter_first_name->value);

                auto iter_last_name = iter_anketa->value.FindMember("lastName");
                if (iter_last_name != iter_anketa->value.MemberEnd() && iter_last_name->value.IsString())
                    member_info.last_name_ = rapidjson_get_string(iter_last_name->value);

                auto iter_nickname = iter_anketa->value.FindMember("nickname");
                if (iter_nickname != iter_anketa->value.MemberEnd() && iter_nickname->value.IsString())
                    member_info.nick_name_ = rapidjson_get_string(iter_nickname->value);
            }

            members_.push_back(std::move(member_info));
        }
    }

    auto iter_owner = _node.FindMember("owner");
    if (iter_owner != _node.MemberEnd())
    {
        auto iter_owner_aimid = iter_yours->value.FindMember("sn");
        if (iter_owner_aimid != iter_owner->value.MemberEnd() && iter_owner_aimid->value.IsString())
            owner_ = rapidjson_get_string(iter_owner_aimid->value);
    }

    return 0;
}

void chat_info::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("aimid", aimid_);
    _coll.set_value_as_string("name", name_);
    _coll.set_value_as_string("location", location_);
    _coll.set_value_as_string("stamp", stamp_);
    _coll.set_value_as_string("about", about_);
    _coll.set_value_as_string("your_role", your_role_);
    _coll.set_value_as_string("owner", owner_);
    _coll.set_value_as_string("members_version", members_version_);
    _coll.set_value_as_string("info_version", info_version_);
    _coll.set_value_as_int("create_time", create_time_);
    _coll.set_value_as_int("members_count", members_count_);
    _coll.set_value_as_int("friend_count", friend_count_);
    _coll.set_value_as_int("blocked_count", blocked_count_);
    _coll.set_value_as_int("pending_count", pending_count_);
    _coll.set_value_as_bool("you_blocked", you_blocked_);
    _coll.set_value_as_bool("you_pending", you_pending_);
    _coll.set_value_as_bool("you_member", you_member_);
    _coll.set_value_as_bool("public", public_);
    _coll.set_value_as_bool("live", live_);
    _coll.set_value_as_bool("controlled", controlled_);
    _coll.set_value_as_string("stamp", stamp_);
    _coll.set_value_as_bool("joinModeration", joinModeration_);
    _coll.set_value_as_bool("age_restriction", age_restriction_);
    _coll.set_value_as_string("creator", creator_);
    _coll.set_value_as_string("default_role", default_role_);

    ifptr<iarray> members_array(_coll->create_array());

    if (!members_.empty())
    {
        members_array->reserve((int32_t)members_.size());
        for (const auto& member : members_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("aimid", member.aimid_);
            _coll_message.set_value_as_string("role", member.role_);
            _coll_message.set_value_as_string("first_name", member.first_name_);
            _coll_message.set_value_as_string("last_name", member.last_name_);
            _coll_message.set_value_as_string("nick_name", member.nick_name_);
            _coll_message.set_value_as_bool("friend", member.friend_);
            _coll_message.set_value_as_bool("no_avatar", member.no_avatar_);
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            members_array->push_back(val.get());
        }
    }

    _coll.set_value_as_array("members", members_array.get());
}