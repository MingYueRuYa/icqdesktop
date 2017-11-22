#include "stdafx.h"

#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"

#include "../connections/wim/wim_history.h"

#include "../log/log.h"

#include "../tools/file_sharing.h"

#include "../tools/system.h"

#include "history_patch.h"

#include "history_message.h"

using namespace core;
using namespace archive;

using namespace boost::algorithm;

namespace
{
    enum class chat_event_type_class
    {
        min,

        unknown,
        added_to_buddy_list,
        chat_modified,
        mchat,
        buddy_reg,
        buddy_found,
        birthday,
        generic,

        max,
    };

    const std::string c_msgid = "msgId";
    const std::string c_wimid = "wid";
    const std::string c_outgoing = "outgoing";
    const std::string c_time = "time";
    const std::string c_text = "text";
    const std::string c_sticker = "sticker";
    const std::string c_sticker_id = "stickerId";
    const std::string c_mult = "mult";
    const std::string c_voip = "voip";
    const std::string c_chat = "chat";
    const std::string c_reqid = "reqId";
    const std::string c_friendly = "friendly";
    const std::string c_added_to_buddy_list = "addedToBuddyList";
    const std::string c_event_class = "event";
    const std::string c_parts = "parts";
    const std::string c_mentions = "mentions";
    const std::string c_media_type = "mediaType";
    const std::string c_sn = "sn";
    const std::string c_stiker_id = "stickerId";
    const std::string c_type_quote = "quote";
    const std::string c_type_forward = "forward";
    const std::string c_type_text = "text";
    const std::string c_type_sticker = "sticker";
    const std::string c_chat_stamp = "stamp";
    const std::string c_chat_name = "name";

    std::string parse_sender_aimid(const rapidjson::Value &_node);

    void serialize_metadata_from_uri(const std::string &_uri, Out coll_helper &_coll);

    std::string find_person(const std::string &_aimid, const persons_map &_persons);

    bool is_generic_event(const rapidjson::Value& _node);

    chat_event_type_class probe_for_chat_event(const rapidjson::Value& _node);

    chat_event_type_class probe_for_modified_event(const rapidjson::Value& _node);

    str_set read_members(const rapidjson::Value &_parent);

    chat_event_type string_2_chat_event(const std::string &_str);
}


//////////////////////////////////////////////////////////////////////////
// message_header class
//////////////////////////////////////////////////////////////////////////
message_header::message_header()
    :
    id_(-1),
    version_(0),
    time_(0),
    prev_id_(-1),
    data_offset_(-1),
    data_size_(0)
{

}

message_header::message_header(
    message_flags _flags,
    uint64_t _time,
    int64_t _id,
    int64_t _prev_id,
    int64_t _data_offset,
    uint32_t _data_size)
    :
    id_(_id),
    version_(0),
    flags_(_flags),
    time_(_time),
    prev_id_(_prev_id),
    data_offset_(_data_offset),
    data_size_(_data_size)
{
}

int64_t message_header::get_data_offset() const
{
    return data_offset_;
}

void message_header::set_data_offset(int64_t _value)
{
    data_offset_ = _value;
}

uint32_t message_header::get_data_size() const
{
    return data_size_;
}

void message_header::set_data_size(uint32_t _value)
{
    assert(_value > 0);
    data_size_ = _value;
}

void message_header::set_prev_msgid(int64_t _value)
{
    assert(_value >= -1);
    assert(!has_id() || (_value < get_id()));

    prev_id_ = _value;
}

uint32_t message_header::data_sizeof() const
{
    return (
        sizeof(uint8_t) +	/*version_*/
        sizeof(uint32_t) +	/*flags_*/
        sizeof(uint64_t) +	/*time_*/
        sizeof(int64_t) +	/*id_*/
        sizeof(int64_t) +	/*prev_id_*/
        sizeof(int64_t) +	/*data_offset_*/
        sizeof(uint32_t)	/*data_size_*/
        );
}

void message_header::serialize(core::tools::binary_stream& _data) const
{
    _data.write<uint8_t>(version_);
    _data.write<uint32_t>(flags_.value_);
    _data.write<uint64_t>(time_);
    _data.write<int64_t>(id_);
    _data.write<int64_t>(prev_id_);
    _data.write<int64_t>(data_offset_);
    _data.write<uint32_t>(data_size_);

#ifdef _DEBUG
    if (_data.available() != data_sizeof())
    {
        assert(!"invalid data size");
    }
#endif
}

bool message_header::unserialize(core::tools::binary_stream& _data)
{
    if (_data.available() < data_sizeof())
        return false;

    version_ = _data.read<uint8_t>();
    flags_.value_ = _data.read<uint32_t>();
    time_ = _data.read<uint64_t>();
    id_ = _data.read<int64_t>();
    prev_id_ = _data.read<int64_t>();
    data_offset_ = _data.read<int64_t>();
    data_size_ = _data.read<uint32_t>();

    return true;
}

void message_header::merge_with(const message_header &rhs)
{
    assert(id_ == rhs.id_);

    if (rhs.is_modified())
    {
        __INFO(
            "delete_history",
            "merging modification\n"
            "    id=<%1%>",
            id_
        );

        modifications_.emplace_back(rhs);
    }

    version_ = rhs.version_;
    time_ = rhs.time_;

    const auto updateOffset = ((rhs.data_offset_ != -1) && !rhs.is_patch());
    if (updateOffset)
    {
        data_offset_ = rhs.data_offset_;
        data_size_ = rhs.data_size_;
    }

    __INFO(
        "delete_history",
        "header merge\n"
        "    header-id=<%1%>\n"
        "    is-patch=<%2%>\n"
        "    is-deleted=<%3%>\n"
        "    is-modified=<%4%>\n"
        "    rhs-is-patch=<%5%>\n"
        "    rhs-is-deleted=<%6%>\n"
        "    rhs-is-modified=<%7%>",
        id_ % is_patch() % is_deleted() % is_modified() % rhs.is_patch() % rhs.is_deleted() % rhs.is_modified());

    const auto updatePrevId = (rhs.prev_id_ != -1);
    if (updatePrevId)
    {
        prev_id_ = rhs.prev_id_;
    }

    const auto reset_patch_flag = (!is_patch() || !rhs.is_patch());
    const auto set_deleted_flag = (bool)flags_.flags_.deleted_;
    const auto set_modified_flag = (bool)flags_.flags_.modified_;

    flags_ = rhs.flags_;

    if (reset_patch_flag)
    {
        flags_.flags_.patch_ = 0;
    }

    if (set_deleted_flag)
    {
        flags_.flags_.deleted_ = 1;
    }

    if (set_modified_flag)
    {
        flags_.flags_.modified_ = 1;
    }
}

bool message_header::is_deleted() const
{
    return flags_.flags_.deleted_;
}

bool message_header::is_modified() const
{
    return flags_.flags_.modified_;
}

bool message_header::is_patch() const
{
    return flags_.flags_.patch_;
}

bool message_header::is_outgoing() const
{
    return flags_.flags_.outgoing_;
}

const message_header_vec& message_header::get_modifications() const
{
    assert(is_modified());

    return modifications_;
}

bool message_header::has_modifications() const
{
    return (is_modified() && !modifications_.empty());
}

//////////////////////////////////////////////////////////////////////////
// sticker_data class
//////////////////////////////////////////////////////////////////////////
int32_t core::archive::sticker_data::unserialize(const rapidjson::Value& _node)
{
    auto iter_id = _node.FindMember("id");

    if (iter_id == _node.MemberEnd() || !iter_id->value.IsString())
        return -1;

    id_ = rapidjson_get_string(iter_id->value);

    return 0;
}

enum message_fields : uint32_t
{
    mf_msg_id									= 1,
    mf_flags									= 2,
    mf_time										= 3,
    mf_wimid									= 4,
    mf_text										= 5,
    mf_chat										= 6,
    mf_sticker									= 7,
    mf_mult										= 8,
    mf_voip										= 9,
    mf_sticker_id								= 10,
    mf_chat_sender								= 11,
    mf_chat_name								= 12,
    mf_prev_msg_id								= 13,
    mf_internal_id								= 14,
    mf_chat_friendly							= 15,
    mf_file_sharing								= 16,
    //mf_file_sharing_outgoing					= 17,
    mf_file_sharing_uri							= 18,
    mf_file_sharing_local_path					= 19,
    //mf_file_sharing_uploading_id				= 20,
    mf_sender_friendly							= 21,
    mf_chat_event								= 22,
    mf_chat_event_type							= 23,
    mf_chat_event_sender_friendly				= 24,
    mf_chat_event_mchat_members					= 25,
    mf_chat_event_new_chat_name					= 26,
    mf_voip_event_type                          = 27,
    mf_voip_sender_friendly                     = 28,
    mf_voip_sender_aimid                        = 29,
    mf_voip_duration                            = 30,
    mf_voip_is_incoming                         = 31,
    mf_chat_event_generic_text                  = 32,
    mf_chat_event_new_chat_description          = 33,
    mf_quote_text                               = 34,
    mf_quote_sn                                 = 35,
    mf_quote_msg_id                             = 36,
    mf_quote_time                               = 37,
    mf_quote_chat_id                            = 38,
    mf_quote                                    = 39,
    mf_quote_friendly                           = 40,
    mf_quote_is_forward                         = 41,
    mf_chat_event_new_chat_rules                = 42,
    mf_chat_event_sender_aimid                  = 43,
    mf_quote_set_id                             = 44,
    mf_quote_sticker_id                         = 45,
    mf_quote_chat_stamp                         = 46,
    mf_quote_chat_name                          = 47,
    mf_mention                                  = 48,
    mf_mention_sn                               = 49,
    mf_mention_friendly                         = 50,
};

sticker_data::sticker_data()
{
}

sticker_data::sticker_data(std::string _id)
    : id_(std::move(_id))
{
    assert(boost::starts_with(id_, "ext:"));
}

void core::archive::sticker_data::serialize(core::tools::tlvpack& _pack)
{
    _pack.push_child(core::tools::tlv(message_fields::mf_sticker_id, id_));
}

int32_t core::archive::sticker_data::unserialize(core::tools::tlvpack& _pack)
{
    assert(id_.empty());

    auto tlv_id = _pack.get_item(message_fields::mf_sticker_id);
    if (!tlv_id)
        return -1;

    id_ = tlv_id->get_value<std::string>(std::string());

    return 0;
}

void core::archive::sticker_data::serialize(icollection* _collection)
{
    coll_helper coll(_collection, false);

    auto ids = get_ids(id_);

    coll.set<uint32_t>("set_id", ids.first);
    coll.set<uint32_t>("sticker_id", ids.second);
}

std::pair<int32_t, int32_t> core::archive::sticker_data::get_ids(const std::string& _id)
{
    std::vector<std::string> components;
    components.reserve(4);

    boost::split(Out components, _id, boost::is_any_of(":"));

    assert(components.size() == 4);
    if (components.size() != 4)
    {
        return std::make_pair(0, 0);
    }

    assert(components[0] == "ext");
    assert(components[2] == "sticker");

    const auto set_id = std::stoul(components[1]);
    assert(set_id > 0);

    const auto sticker_id = std::stoul(components[3]);
    assert(sticker_id > 0);

    return std::make_pair(set_id, sticker_id);
}

core::archive::voip_data::voip_data()
    : type_(voip_event_type::invalid)
    , duration_sec_(-1)
    , is_incoming_(-1)
{
}

void core::archive::voip_data::apply_persons(const archive::persons_map &_persons)
{
    assert(!sender_aimid_.empty());
    sender_friendly_ = find_person(sender_aimid_, _persons);
}

bool core::archive::voip_data::unserialize(
    const rapidjson::Value &_node,
    const std::string &_sender_aimid)
{
    assert(!_sender_aimid.empty());
    assert(type_ == core::voip_event_type::invalid);

    assert(sender_aimid_.empty());
    sender_aimid_ = _sender_aimid;

    unserialize_duration(_node);

    // read type

    const auto type_node_iter = _node.FindMember("type");
    if (type_node_iter == _node.MemberEnd())
    {
        assert(!"invalid format");
        return false;
    }

    const auto &type_node = type_node_iter->value;
    if (!type_node.IsString())
    {
        assert(!"invalid format");
        return false;
    }

    const std::string type = rapidjson_get_string(type_node);

    // read subtype

    std::string subtype;

    const auto subtype_node_iter = _node.FindMember("subtype");
    if (subtype_node_iter != _node.MemberEnd())
    {
        const auto &subtype_node = subtype_node_iter->value;
        if (!subtype_node.IsString())
        {
            assert("!invalid format");
            return false;
        }

        subtype = rapidjson_get_string(subtype_node);
    }

    // read event direction

    assert(is_incoming_ == -1);
    const auto incall_node_iter = _node.FindMember("inCall");
    if (incall_node_iter != _node.MemberEnd())
    {
        const auto &incall_node = incall_node_iter->value;
        if (incall_node.IsBool())
        {
            is_incoming_ = (int32_t)incall_node.GetBool();
        }
        else
        {
            assert(!"invalid format");
        }
    }

    const auto is_type_terminate = (type == "terminate");

    const auto is_subtype_missed = (subtype == "missed");
    const auto is_missed_call = (is_type_terminate && is_subtype_missed);
    if (is_missed_call)
    {
        type_ = voip_event_type::missed_call;
        return true;
    }

    const auto is_call_ended = is_type_terminate;
    if (is_call_ended)
    {
        type_ = voip_event_type::call_ended;
        return true;
    }

    const auto is_type_accept = (type == "accept");
    if (is_type_accept && subtype.empty())
    {
        type_ = voip_event_type::accept;
        return true;
    }

    return false;
}

void core::archive::voip_data::serialize(Out coll_helper &_coll) const
{
    assert(type_ > voip_event_type::min);
    assert(type_ < voip_event_type::max);
    _coll.set<voip_event_type>("type", type_);

    assert(!sender_aimid_.empty());
    _coll.set<std::string>("sender_aimid", sender_aimid_);

    assert(!sender_friendly_.empty());
    _coll.set<std::string>("sender_friendly", sender_aimid_);

    assert(duration_sec_ >= -1);
    if (duration_sec_ >= 0)
    {
        _coll.set<int32_t>("duration_sec", duration_sec_);
    }

    assert(is_incoming_ >= -1);
    if (is_incoming_ >= 0)
    {
        _coll.set<bool>("is_incoming", (is_incoming_ > 0));
    }
}

bool core::archive::voip_data::unserialize(const coll_helper &_coll)
{
    assert(!"there is no scenario in which the method is called");
    return true;
}

void core::archive::voip_data::serialize(Out core::tools::tlvpack &_pack) const
{
    assert(type_ != voip_event_type::invalid);

    _pack.push_child(core::tools::tlv(message_fields::mf_voip_event_type, (int32_t)type_));

    assert(!sender_aimid_.empty());
    _pack.push_child(core::tools::tlv(message_fields::mf_voip_sender_aimid, sender_aimid_));

    assert(!sender_friendly_.empty());
    _pack.push_child(core::tools::tlv(message_fields::mf_voip_sender_friendly, sender_friendly_));

    assert(duration_sec_ >= -1);
    if (duration_sec_ >= 0)
    {
        _pack.push_child(core::tools::tlv(message_fields::mf_voip_duration, duration_sec_));
    }

    assert(is_incoming_ >= -1);
    if (is_incoming_ >= 0)
    {
        _pack.push_child(core::tools::tlv(message_fields::mf_voip_is_incoming, is_incoming_));
    }
}

bool core::archive::voip_data::unserialize(const core::tools::tlvpack &_pack)
{
    assert(type_ == voip_event_type::invalid);
    assert(sender_friendly_.empty());
    assert(sender_aimid_.empty());

    const auto tlv_type = _pack.get_item(message_fields::mf_voip_event_type);
    assert(tlv_type);
    if (!tlv_type)
    {
        return false;
    }

    const auto tlv_sender_aimid = _pack.get_item(message_fields::mf_voip_sender_aimid);
    assert(tlv_sender_aimid);
    if (!tlv_sender_aimid)
    {
        return false;
    }

    const auto tlv_sender_friendly = _pack.get_item(message_fields::mf_voip_sender_friendly);
    assert(tlv_sender_friendly);
    if (!tlv_sender_friendly)
    {
        return false;
    }

    type_ = tlv_type->get_value<voip_event_type>();
    const auto is_valid_type = ((type_ > voip_event_type::min) && (type_ < voip_event_type::max));
    assert(is_valid_type);
    if (!is_valid_type)
    {
        type_ = voip_event_type::invalid;
        return false;
    }

    sender_aimid_ = tlv_sender_aimid->get_value<std::string>();
    assert(!sender_aimid_.empty());
    if (sender_aimid_.empty())
    {
        return false;
    }

    sender_friendly_ = tlv_sender_friendly->get_value<std::string>();
    assert(!sender_friendly_.empty());
    if (sender_friendly_.empty())
    {
        return false;
    }

    const auto tlv_duration = _pack.get_item(message_fields::mf_voip_duration);
    if (tlv_duration)
    {
        duration_sec_ = tlv_duration->get_value<int32_t>(-1);
        assert(duration_sec_ >= -1);
    }

    const auto tlv_is_incoming = _pack.get_item(message_fields::mf_voip_is_incoming);
    if (tlv_is_incoming)
    {
        is_incoming_ = tlv_is_incoming->get_value<int32_t>(-1);
        assert(is_incoming_ >= -1);
    }

    return true;
}

void core::archive::voip_data::unserialize_duration(const rapidjson::Value &_node)
{
    duration_sec_ = -1;

    const auto duration_node_iter = _node.FindMember("duration");
    if (duration_node_iter == _node.MemberEnd())
    {
        return;
    }

    const auto &duration_node = duration_node_iter->value;
    if (!duration_node.IsInt())
    {
        assert(!"invalid voip event duration format");
        return;
    }

    duration_sec_ = duration_node.GetInt();

    const auto max_duration = 3600 * 23; // 23 hours in seconds
    const auto is_valid_duration = ((duration_sec_ >= 0) && (duration_sec_ <= max_duration));
    if (!is_valid_duration)
    {
        assert(!"invalid voip event duration value");
        duration_sec_ = -1;
    }
}

//////////////////////////////////////////////////////////////////////////
// chat_data class
//////////////////////////////////////////////////////////////////////////

void core::archive::chat_data::apply_persons(const archive::persons_map &_persons)
{
    auto iter_p = _persons.find(get_sender());

    if (iter_p == _persons.end())
    {
        return;
    }

    set_friendly(iter_p->second.friendly_);
}

int32_t core::archive::chat_data::unserialize(const rapidjson::Value& _node)
{
    auto iter_sender = _node.FindMember("sender");
    auto iter_name = _node.FindMember("name");

    if (iter_sender == _node.MemberEnd() || !iter_sender->value.IsString())
        return -1;

    sender_ = rapidjson_get_string(iter_sender->value);

    if (iter_name != _node.MemberEnd() && iter_name->value.IsString())
        name_ = rapidjson_get_string(iter_name->value);

    // workaround for the server issue
    // https://jira.mail.ru/browse/IMDESKTOP-1923
    boost::replace_last(sender_, "@uin.icq", std::string());

    return 0;
}


void core::archive::chat_data::serialize(core::tools::tlvpack& _pack)
{
    _pack.push_child(core::tools::tlv(message_fields::mf_chat_sender, sender_));
    _pack.push_child(core::tools::tlv(message_fields::mf_chat_name, name_));
    _pack.push_child(core::tools::tlv(message_fields::mf_chat_friendly, friendly_));
}

int32_t core::archive::chat_data::unserialize(core::tools::tlvpack& _pack)
{
    auto tlv_sender = _pack.get_item(message_fields::mf_chat_sender);
    auto tlv_name = _pack.get_item(message_fields::mf_chat_name);
    auto tlv_friendly = _pack.get_item(message_fields::mf_chat_friendly);

    if (!tlv_sender || !tlv_name)
        return -1;

    sender_ = tlv_sender->get_value<std::string>();
    name_ = tlv_name->get_value<std::string>();

    if (tlv_friendly)
        friendly_ = tlv_friendly->get_value<std::string>();

    return 0;
}

void core::archive::chat_data::serialize(icollection* _collection)
{
    coll_helper coll(_collection, false);

    coll.set_value_as_string("sender", sender_);
    coll.set_value_as_string("name", name_);
    coll.set_value_as_string("friendly", friendly_);
}



//////////////////////////////////////////////////////////////////////////
// file_sharing_data
//////////////////////////////////////////////////////////////////////////

file_sharing_data::file_sharing_data(const std::string &_local_path,
                                     const std::string &_uri)
                                     : uri_(_uri)
                                     , local_path_(_local_path)
{
    if (local_path_.empty())
    {
        assert(!uri_.empty());
        assert(starts_with(uri_, "http"));
    }
    else
    {
        assert(core::tools::system::is_exist(
            core::tools::from_utf8(local_path_)
            ));
        assert(uri_.empty());
    }
}

file_sharing_data::file_sharing_data(icollection* _collection)
{
    coll_helper coll(_collection, false);

    if (coll.is_value_exist("uri"))
    {
        assert(!coll.is_value_exist("local_path"));
        uri_ = coll.get_value_as_string("uri");
    }
    else
    {
        assert(!coll.is_value_exist("uri"));
        local_path_ = coll.get_value_as_string("local_path");
    }
}

file_sharing_data::file_sharing_data(const core::tools::tlvpack &_pack)
{
    if (_pack.get_item(message_fields::mf_file_sharing_uri))
    {
        assert(!_pack.get_item(message_fields::mf_file_sharing_local_path));

        uri_ = _pack.get_item(message_fields::mf_file_sharing_uri)->get_value<std::string>();
    }
    else
    {
        local_path_ = _pack.get_item(message_fields::mf_file_sharing_local_path)->get_value<std::string>();
        assert(!local_path_.empty());
    }

    __TRACE(
        "fs",
        "file sharing data deserialized (from db)\n"
        "	uri=<%1%>\n"
        "	local_path=<%2%>",
        uri_ %
        local_path_);
}

void file_sharing_data::serialize(Out icollection* _collection, const std::string &_internal_id, const bool _is_outgoing) const
{
    assert(_collection);

    coll_helper coll(_collection, false);

    if (!uri_.empty())
    {
        assert(local_path_.empty());

        coll.set_value_as_string("uri", uri_);
        serialize_metadata_from_uri(uri_, Out coll);
    }
    else
    {
        assert(!local_path_.empty());
        assert(!_internal_id.empty());
        assert(_is_outgoing);

        coll.set<std::string>("local_path", local_path_);
        coll.set<std::string>("uploading_id", _internal_id);
    }

    coll.set<bool>("outgoing", _is_outgoing);
}

void file_sharing_data::serialize(Out core::tools::tlvpack& _pack) const
{
    assert(_pack.empty());

    if (!uri_.empty())
    {
        assert(local_path_.empty());
        _pack.push_child(core::tools::tlv(message_fields::mf_file_sharing_uri, uri_));
    }
    else
    {
        assert(core::tools::system::is_exist(
            core::tools::from_utf8(local_path_)
        ));

        _pack.push_child(core::tools::tlv(message_fields::mf_file_sharing_local_path, local_path_));
    }
}

bool file_sharing_data::contents_equal(const file_sharing_data& _rhs) const
{
    if (get_local_path() == _rhs.get_local_path())
    {
        return true;
    }

    if (get_uri() == _rhs.get_uri())
    {
        return true;
    }

    return false;
}

const std::string& file_sharing_data::get_local_path() const
{
    return local_path_;
}

const std::string& file_sharing_data::get_uri() const
{
    return uri_;
}

std::string file_sharing_data::to_log_string() const
{
    boost::format result(
        "	uri=<%1%>\n"
        "	local_path=<%2%>"
        );

    result % uri_ % local_path_;

    return result.str();
}

chat_event_data_uptr chat_event_data::make_added_to_buddy_list(const std::string &_sender_aimid)
{
    assert(!_sender_aimid.empty());

    chat_event_data_uptr result(
        new chat_event_data(
        core::chat_event_type::added_to_buddy_list
        )
        );

    assert(result->sender_aimid_.empty());
    result->sender_aimid_ = _sender_aimid;

    return result;
}

chat_event_data_uptr chat_event_data::make_mchat_event(const rapidjson::Value& _node)
{
    assert(_node.IsObject());

    auto sender_aimid = parse_sender_aimid(_node);
    if (sender_aimid.empty())
    {
        return nullptr;
    }

    auto event_iter = _node.FindMember("memberEvent");
    if ((event_iter == _node.MemberEnd()) || !event_iter->value.IsObject())
    {
        return nullptr;
    }

    auto type_iter = event_iter->value.FindMember("type");
    if ((type_iter == _node.MemberEnd()) || !type_iter->value.IsString())
    {
        return nullptr;
    }

    const std::string type_str = rapidjson_get_string(type_iter->value);

    assert(!type_str.empty());
    if (type_str.empty())
    {
        return nullptr;
    }

    auto type = string_2_chat_event(type_str);
    if (type == chat_event_type::invalid)
    {
        return nullptr;
    }

    auto members = read_members(event_iter->value);

    chat_event_data_uptr result(
        new chat_event_data(type)
        );

    result->mchat_.members_ = std::move(members);
    result->sender_aimid_ = std::move(sender_aimid);

    return result;
}

chat_event_data_uptr chat_event_data::make_modified_event(const rapidjson::Value& _node)
{
    assert(_node.IsObject());

    auto sender_aimid = parse_sender_aimid(_node);
    if (sender_aimid.empty())
    {
        return nullptr;
    }

    const auto modified_event_iter = _node.FindMember("modifiedInfo");

    assert(modified_event_iter != _node.MemberEnd());
    if (modified_event_iter == _node.MemberEnd())
    {
        return nullptr;
    }

    const auto &modified_event = modified_event_iter->value;

    assert(modified_event.IsObject());
    if (!modified_event.IsObject())
    {
        return nullptr;
    }

    const auto new_name_iter = modified_event.FindMember("name");
    const auto is_name_modified = (new_name_iter != modified_event.MemberEnd());
    if (is_name_modified)
    {
        const auto &new_name_node = new_name_iter->value;
        if (!new_name_node.IsString())
        {
            return nullptr;
        }

        std::string new_name = rapidjson_get_string(new_name_node);

        chat_event_data_uptr result(
            new chat_event_data(chat_event_type::chat_name_modified));

        assert(result->sender_aimid_.empty());
        result->sender_aimid_ = std::move(sender_aimid);

        if (!new_name.empty())
        {
            assert(result->chat_.new_name_.empty());
            result->chat_.new_name_ = std::move(new_name);
        }

        return result;
    }

    const auto avatar_last_modified_iter = modified_event.FindMember("avatarLastModified");
    const auto is_avatar_modified = (avatar_last_modified_iter != modified_event.MemberEnd());
    if (is_avatar_modified)
    {
        chat_event_data_uptr result(
            new chat_event_data(chat_event_type::avatar_modified));

        assert(result->sender_aimid_.empty());
        result->sender_aimid_ = std::move(sender_aimid);

        return result;
    }

    const auto about_iter = modified_event.FindMember("about");
    const auto is_about_changed = (
        (about_iter != modified_event.MemberEnd()) &&
        about_iter->value.IsString());
    if (is_about_changed)
    {
        chat_event_data_uptr result(
            new chat_event_data(chat_event_type::chat_description_modified)
            );

        result->chat_.new_description_ = rapidjson_get_string(about_iter->value);

        assert(result->sender_aimid_.empty());
        result->sender_aimid_ = std::move(sender_aimid);

        return result;
    }

    const auto rules_iter = modified_event.FindMember("rules");
    const auto is_rules_changed = (
        (rules_iter != modified_event.MemberEnd()) &&
        rules_iter->value.IsString());
    if (is_rules_changed)
    {
        chat_event_data_uptr result(
            new chat_event_data(chat_event_type::chat_rules_modified));

        result->chat_.new_rules_ = rapidjson_get_string(rules_iter->value);

        assert(result->sender_aimid_.empty());
        result->sender_aimid_ = std::move(sender_aimid);

        return result;
    }

    return nullptr;
}

chat_event_data_uptr chat_event_data::make_from_tlv(const tools::tlvpack& _pack)
{
    return chat_event_data_uptr(
        new chat_event_data(_pack)
    );
}

chat_event_data_uptr chat_event_data::make_simple_event(const chat_event_type _type)
{
    assert(_type > chat_event_type::min);
    assert(_type < chat_event_type::max);

    return chat_event_data_uptr(
        new chat_event_data(_type)
    );
}

chat_event_data_uptr chat_event_data::make_generic_event(const rapidjson::Value& _text_node)
{
    assert(_text_node.IsString());

    return make_generic_event(rapidjson_get_string(_text_node));
}

chat_event_data_uptr chat_event_data::make_generic_event(std::string _text)
{
    chat_event_data_uptr result(
        new chat_event_data(chat_event_type::generic));

    result->generic_ = std::move(_text);

    return result;
}

chat_event_data::chat_event_data(const chat_event_type _type)
    : type_(_type)
{
    assert(type_ > chat_event_type::min);
    assert(type_ < chat_event_type::max);
}

chat_event_data::chat_event_data(const tools::tlvpack& _pack)
{
    type_ = _pack.get_item(message_fields::mf_chat_event_type)->get_value<chat_event_type>();
    assert(type_ > chat_event_type::min);
    assert(type_ < chat_event_type::max);

    if (has_sender_aimid())
    {
        sender_friendly_ = _pack.get_item(message_fields::mf_chat_event_sender_friendly)->get_value<std::string>();
        assert(!sender_friendly_.empty());

        auto item = _pack.get_item(message_fields::mf_chat_event_sender_aimid);
        if (item)
        {
            sender_aimid_ = item->get_value<std::string>(std::string());
            assert(!sender_aimid_.empty());
        }
    }

    if (has_mchat_members())
    {
        const auto item = _pack.get_item(message_fields::mf_chat_event_mchat_members);

        assert(item);
        if (item)
        {
            deserialize_mchat_members(item->get_value<tools::tlvpack>());
        }
    }

    if (has_chat_modifications())
    {
        deserialize_chat_modifications(_pack);
    }

    if (has_generic_text())
    {
        generic_ = _pack.get_item(message_fields::mf_chat_event_generic_text)->get_value<std::string>();
        assert(!generic_.empty());
    }
}

void chat_event_data::apply_persons(const archive::persons_map &_persons)
{
    if (has_sender_aimid())
    {
        assert(!sender_aimid_.empty());
        sender_friendly_ = find_person(sender_aimid_, _persons);
        assert(!sender_friendly_.empty());
    }

    if (has_mchat_members())
    {
        auto &friendly_members = mchat_.members_friendly_;
        assert(friendly_members.empty());

        for (auto member_uin : mchat_.members_)
        {
            boost::replace_last(member_uin, "@uin.icq", std::string());
            friendly_members.emplace(find_person(member_uin, _persons));
        }
    }
}

bool chat_event_data::contents_equal(const chat_event_data& _rhs) const
{
    if (get_type() != _rhs.get_type())
    {
        return false;
    }

    return true;
}

void chat_event_data::serialize(
    Out icollection* _collection,
    const bool _is_outgoing) const
{
    assert(_collection);

    coll_helper coll(_collection, false);
    coll.set_value_as_enum("type", type_);

    if (has_sender_aimid())
    {
        coll.set_value_as_string("sender_aimid", sender_aimid_);

        assert(!sender_friendly_.empty());
        coll.set_value_as_string("sender_friendly", sender_friendly_);
    }

    if (has_mchat_members())
    {
        serialize_mchat_members(Out coll);
    }

    if (has_chat_modifications())
    {
        serialize_chat_modifications(Out coll);
    }

    if (has_generic_text())
    {
        assert(!generic_.empty());
        coll.set<std::string>("generic", generic_);
    }
}

void chat_event_data::serialize(Out tools::tlvpack &_pack) const
{
    _pack.push_child(core::tools::tlv(message_fields::mf_chat_event_type, (int32_t)type_));

    if (has_sender_aimid())
    {
        assert(!sender_aimid_.empty());
        _pack.push_child(core::tools::tlv(message_fields::mf_chat_event_sender_aimid, sender_aimid_));

        assert(!sender_friendly_.empty());
        _pack.push_child(core::tools::tlv(message_fields::mf_chat_event_sender_friendly, sender_friendly_));
    }

    if (has_mchat_members())
    {
        serialize_mchat_members(Out _pack);
    }

    if (has_chat_modifications())
    {
        serialize_chat_modifications(Out _pack);
    }

    if (has_generic_text())
    {
        assert(!generic_.empty());
        _pack.push_child(core::tools::tlv(message_fields::mf_chat_event_generic_text, generic_));
    }
}

void chat_event_data::deserialize_chat_modifications(const tools::tlvpack &_pack)
{
    assert(chat_.new_name_.empty());

    if (type_ == chat_event_type::chat_name_modified)
    {
        const auto new_name_item = _pack.get_item(mf_chat_event_new_chat_name);
        assert(new_name_item);

        auto new_name = new_name_item->get_value<std::string>();
        assert(!new_name.empty());

        chat_.new_name_ = std::move(new_name);
    }

    if (type_ == chat_event_type::chat_description_modified)
    {
        const auto item = _pack.get_item(mf_chat_event_new_chat_description);
        assert(item);

        chat_.new_description_ = item->get_value<std::string>();
    }

    if (type_ == chat_event_type::chat_rules_modified)
    {
        const auto item = _pack.get_item(mf_chat_event_new_chat_rules);
        assert(item);

        chat_.new_rules_ = item->get_value<std::string>();
    }
}

void chat_event_data::deserialize_mchat_members(const tools::tlvpack &_pack)
{
    assert(mchat_.members_friendly_.empty());

    for(auto index = 0u; index < _pack.size(); ++index)
    {
        const auto item = _pack.get_item(index);
        assert(item);

        auto member = item->get_value<std::string>();
        assert(!member.empty());

        const auto result_pair = mchat_.members_friendly_.emplace(std::move(member));
        assert(std::get<1>(result_pair));
    }
}

chat_event_type chat_event_data::get_type() const
{
    assert(type_ >= chat_event_type::min);
    assert(type_ <= chat_event_type::max);

    return type_;
}

bool chat_event_data::has_generic_text() const
{
    return (get_type() == chat_event_type::generic);
}

bool chat_event_data::has_mchat_members() const
{
    return ((get_type() == chat_event_type::mchat_add_members) ||
            (get_type() == chat_event_type::mchat_invite) ||
            (get_type() == chat_event_type::mchat_leave) ||
            (get_type() == chat_event_type::mchat_del_members) ||
            (get_type() == chat_event_type::mchat_kicked));
}

bool chat_event_data::has_chat_modifications() const
{
    return ((get_type() == chat_event_type::chat_name_modified) ||
            (get_type() == chat_event_type::chat_description_modified) ||
            (get_type() == chat_event_type::chat_rules_modified));
}

bool chat_event_data::has_sender_aimid() const
{
    return ((get_type() == chat_event_type::added_to_buddy_list) ||
            (get_type() == chat_event_type::chat_name_modified) ||
            (get_type() == chat_event_type::mchat_add_members) ||
            (get_type() == chat_event_type::mchat_invite) ||
            (get_type() == chat_event_type::mchat_leave) ||
            (get_type() == chat_event_type::mchat_del_members) ||
            (get_type() == chat_event_type::mchat_kicked) ||
            (get_type() == chat_event_type::avatar_modified) ||
            (get_type() == chat_event_type::chat_description_modified) ||
            (get_type() == chat_event_type::chat_rules_modified));
}

void chat_event_data::serialize_chat_modifications(Out coll_helper &_coll) const
{
    if (type_ == chat_event_type::chat_name_modified)
    {
        const auto &new_name = chat_.new_name_;
        assert(!new_name.empty());

        _coll.set_value_as_string("chat/new_name", new_name);
    }

    if (type_ == chat_event_type::chat_description_modified)
    {
        _coll.set<std::string>("chat/new_description", chat_.new_description_);
    }

    if (type_ == chat_event_type::chat_rules_modified)
    {
        assert(!chat_.new_rules_.empty());
        _coll.set<std::string>("chat/new_rules", chat_.new_rules_);
    }
}

void chat_event_data::serialize_chat_modifications(Out tools::tlvpack &_pack) const
{
    if (type_ == chat_event_type::chat_name_modified)
    {
        const auto &new_name = chat_.new_name_;
        assert(!new_name.empty());

        _pack.push_child(tools::tlv(message_fields::mf_chat_event_new_chat_name, new_name));
    }

    if (type_ == chat_event_type::chat_description_modified)
    {
        _pack.push_child(tools::tlv(mf_chat_event_new_chat_description, chat_.new_description_));
    }

    if (type_ == chat_event_type::chat_rules_modified)
    {
        _pack.push_child(tools::tlv(mf_chat_event_new_chat_rules, chat_.new_rules_));
    }
}

void chat_event_data::serialize_mchat_members(Out coll_helper &_coll) const
{
    const auto &members_friendly = mchat_.members_friendly_;
    assert(!members_friendly.empty());

    ifptr<iarray> members_array(_coll->create_array());
    members_array->reserve(members_friendly.size());
    for (const auto &friendly : members_friendly)
    {
        assert(!friendly.empty());

        ifptr<ivalue> friendly_value(_coll->create_value());
        friendly_value->set_as_string(friendly.c_str(), (int32_t)friendly.length());

        members_array->push_back(friendly_value.get());
    }

    _coll.set_value_as_array("mchat/members", members_array.get());
}

void chat_event_data::serialize_mchat_members(Out tools::tlvpack &_pack) const
{
    const auto &friendly_members = mchat_.members_friendly_;
    assert(!friendly_members.empty());

    tools::tlvpack members_pack;

    auto member_index = 0;
    for (const auto &member : friendly_members)
    {
        members_pack.push_child(
            tools::tlv(
            member_index++,
            member
            )
            );
    }

    _pack.push_child(tools::tlv(message_fields::mf_chat_event_mchat_members, members_pack));
}

bool chat_event_data::is_type_deleted() const
{
    return type_ == chat_event_type::message_deleted;
}

//////////////////////////////////////////////////////////////////////////
// history_message class
//////////////////////////////////////////////////////////////////////////

history_message_sptr history_message::make_deleted_patch(const int64_t _archive_id)
{
    assert(_archive_id > 0);

    auto result = std::make_shared<history_message>();

    result->msgid_ = _archive_id;
    result->set_deleted(true);
    result->set_patch(true);

    return result;
}

history_message_sptr history_message::make_modified_patch(const int64_t _archive_id)
{
    assert(_archive_id > 0);

    auto result = std::make_shared<history_message>();

    result->msgid_ = _archive_id;
    result->set_modified(true);
    result->set_patch(true);

    result->chat_event_ = chat_event_data::make_simple_event(chat_event_type::message_deleted);

    return result;
}

history_message::history_message(const history_message& _message)
{
    copy(_message);
}

history_message::history_message()
{
    init_default();
}

history_message::~history_message()
{

}

history_message& history_message::operator=(const history_message& _message)
{
    copy(_message);
    return (*this);
}

void history_message::init_default()
{
    msgid_			= -1;
    time_			= 0;
    data_offset_	= -1;
    data_size_		= 0;
    prev_msg_id_	= -1;
}

void history_message::init_file_sharing_from_local_path(const std::string &_local_path)
{
    assert(core::tools::system::is_exist(
        core::tools::from_utf8(_local_path)
        ));
    assert(!file_sharing_);

    file_sharing_ = std::make_unique<core::archive::file_sharing_data>(_local_path, std::string());
}

void history_message::init_file_sharing_from_link(const std::string &_uri)
{
    file_sharing_ = std::make_unique<core::archive::file_sharing_data>(std::string(), _uri);
}

void history_message::init_sticker_from_text(const std::string &_text)
{
    assert(boost::starts_with(_text, "ext:"));

    sticker_ = std::make_unique<core::archive::sticker_data>(_text);
}

const file_sharing_data_uptr& history_message::get_file_sharing_data() const
{
    return file_sharing_;
}

chat_event_data_uptr& history_message::get_chat_event_data()
{
    return chat_event_;
}

voip_data_uptr& history_message::get_voip_data()
{
    return voip_;
}

void history_message::copy(const history_message& _message)
{
    msgid_ = _message.msgid_;
    prev_msg_id_ = _message.prev_msg_id_;
    wimid_ = _message.wimid_;
    internal_id_ = _message.internal_id_;
    time_ = _message.time_;
    text_ = _message.text_;
    data_offset_ = _message.data_offset_;
    data_size_ = _message.data_size_;
    flags_ = _message.flags_;
    sender_friendly_ = _message.sender_friendly_;
    quotes_ = _message.quotes_;
    mentions_ = _message.mentions_;

    sticker_.reset();
    mult_.reset();
    voip_.reset();
    chat_.reset();
    file_sharing_.reset();
    chat_event_.reset();

    if (_message.sticker_)
    {
        sticker_ = std::make_unique<core::archive::sticker_data>(*_message.sticker_);
    }

    if (_message.mult_)
    {
        mult_ = std::make_unique<core::archive::mult_data>(*_message.mult_);
    }

    if (_message.voip_)
    {
        voip_ = std::make_unique<core::archive::voip_data>(*_message.voip_);
    }

    if (_message.chat_)
    {
        chat_ = std::make_unique<core::archive::chat_data>(*_message.chat_);
    }

    if (_message.file_sharing_)
    {
        file_sharing_ = std::make_unique<core::archive::file_sharing_data>(*_message.file_sharing_);
    }

    if (_message.chat_event_)
    {
        chat_event_ = std::make_unique<core::archive::chat_event_data>(*_message.chat_event_);
    }
}


archive::chat_data* history_message::get_chat_data()
{
    return chat_.get();
}

void history_message::set_chat_data(const chat_data& _data)
{
    if (!chat_)
    {
        chat_ = std::make_unique<core::archive::chat_data>(_data);

        return;
    }

    *chat_ = _data;
}

const archive::chat_data* history_message::get_chat_data() const
{
    if (!chat_)
        return nullptr;

    return chat_.get();
}

void history_message::set_prev_msgid(int64_t _value)
{
    assert(_value >= -1);
    assert(!has_msgid() || (_value < get_msgid()));

    prev_msg_id_ = _value;
}

void history_message::serialize(icollection* _collection, const time_t _offset, bool _serialize_message) const
{
    coll_helper coll(_collection, false);

    coll.set_value_as_int64("id", msgid_);
    coll.set_value_as_int64("prev_id", prev_msg_id_);
    coll.set_value_as_int("flags", flags_.value_);
    coll.set<bool>("outgoing", is_outgoing());
    coll.set<bool>("deleted", is_deleted());
    coll.set_value_as_int("time", (int32_t) (time_ + _offset));
    if (_serialize_message)
        coll.set_value_as_string("text", text_);
    coll.set_value_as_string("internal_id", internal_id_);
    coll.set_value_as_string("sender_friendly", sender_friendly_);

    if (chat_event_)
    {
        coll_helper coll_chat_event(coll->create_collection(), true);
        chat_event_->serialize(coll_chat_event.get(), flags_.flags_.outgoing_);
        coll.set_value_as_collection("chat_event", coll_chat_event.get());
    }

    if (file_sharing_)
    {
        __TRACE(
            "fs",
            "serializing file sharing data (for gui)\n"
            "	internal_id=<%1%>\n"
            "%2%",
            internal_id_ % file_sharing_->to_log_string())

            coll_helper coll_file_sharing(coll->create_collection(), true);
        file_sharing_->serialize(coll_file_sharing.get(), internal_id_, flags_.flags_.outgoing_);

        coll.set_value_as_collection("file_sharing", coll_file_sharing.get());
    }

    if (chat_)
    {
        coll_helper coll_chat(coll->create_collection(), true);
        chat_->serialize(coll_chat.get());
        coll.set_value_as_collection("chat", coll_chat.get());
    }

    if (sticker_)
    {
        coll_helper coll_sticker(coll->create_collection(), true);
        sticker_->serialize(coll_sticker.get());
        coll.set_value_as_collection("sticker", coll_sticker.get());
    }

    if (voip_)
    {
        coll.set<iserializable>("voip", *voip_);
    }

    if (mult_)
    {
        coll_helper coll_mult(coll->create_collection(), true);
        mult_->serialize(coll_mult.get());
        coll.set_value_as_collection("mult", coll_mult.get());
    }

    if (!quotes_.empty())
    {
        ifptr<iarray> quotes_array(coll->create_array());
        quotes_array->reserve(quotes_.size());

        for (const auto& q : quotes_)
        {
            coll_helper coll_quote(coll->create_collection(), true);
            q.serialize(coll_quote.get());
            ifptr<ivalue> quote_value(coll->create_value());
            quote_value->set_as_collection(coll_quote.get());
            quotes_array->push_back(quote_value.get());
        }

        coll.set_value_as_array("quotes", quotes_array.get());
    }

    if (!mentions_.empty())
    {
        ifptr<iarray> arr(coll->create_array());
        arr->reserve(mentions_.size());

        for (const auto& p: mentions_)
        {
            coll_helper coll_ment(coll->create_collection(), true);

            coll_ment.set_value_as_string("sn", p.first);
            coll_ment.set_value_as_string("friendly", p.second);

            ifptr<ivalue> val(coll->create_value());
            val->set_as_collection(coll_ment.get());
            arr->push_back(val.get());
        }

        coll.set_value_as_array("mentions", arr.get());
    }
}


void history_message::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack msg_pack;

    // text is the first for fast searching
    msg_pack.push_child(core::tools::tlv(mf_text, (std::string) text_));
    msg_pack.push_child(core::tools::tlv(mf_msg_id, (int64_t) msgid_));
    msg_pack.push_child(core::tools::tlv(mf_prev_msg_id, (int64_t) prev_msg_id_));
    msg_pack.push_child(core::tools::tlv(mf_flags, (uint32_t) flags_.value_));
    msg_pack.push_child(core::tools::tlv(mf_time, (uint64_t) time_));
    msg_pack.push_child(core::tools::tlv(mf_wimid, (std::string) wimid_));
    msg_pack.push_child(core::tools::tlv(mf_internal_id, (std::string) internal_id_));
    msg_pack.push_child(core::tools::tlv(mf_sender_friendly, (std::string) sender_friendly_));

    if (chat_)
    {
        core::tools::tlvpack chat_pack;
        chat_->serialize(chat_pack);
        msg_pack.push_child(core::tools::tlv(mf_chat, chat_pack));
    }

    if (sticker_)
    {
        core::tools::tlvpack sticker_pack;
        sticker_->serialize(sticker_pack);
        msg_pack.push_child(core::tools::tlv(mf_sticker, sticker_pack));
    }

    if (mult_)
    {
        core::tools::tlvpack mult_pack;
        mult_->serialize(mult_pack);
        msg_pack.push_child(core::tools::tlv(mf_mult, mult_pack));
    }

    if (voip_)
    {
        msg_pack.push_child(core::tools::tlv(mf_voip, *voip_));
    }

    if (file_sharing_)
    {
        core::tools::tlvpack file_sharing_pack;
        file_sharing_->serialize(Out file_sharing_pack);
        msg_pack.push_child(core::tools::tlv(mf_file_sharing, file_sharing_pack));
    }

    if (chat_event_)
    {
        core::tools::tlvpack chat_event_pack;
        chat_event_->serialize(Out chat_event_pack);
        msg_pack.push_child(core::tools::tlv(mf_chat_event, chat_event_pack));
    }

    for (const auto& q : quotes_)
    {
        core::tools::tlvpack quote_pack;
        q.serialize(quote_pack);
        msg_pack.push_child(core::tools::tlv(mf_quote, quote_pack));
    }

    for (const auto& p: mentions_)
    {
        core::tools::tlvpack pack;
        pack.push_child(core::tools::tlv(message_fields::mf_mention_sn, p.first));
        pack.push_child(core::tools::tlv(message_fields::mf_mention_friendly, p.second));
        msg_pack.push_child(core::tools::tlv(mf_mention, pack));
    }

    msg_pack.serialize(_data);
}

int32_t history_message::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack msg_pack;

    if (!msg_pack.unserialize(_data))
        return -1;

    for (auto tlv_field = msg_pack.get_first(); tlv_field; tlv_field = msg_pack.get_next())
    {
        switch ((message_fields) tlv_field->get_type())
        {
        case message_fields::mf_msg_id:
            msgid_ = tlv_field->get_value<int64_t>(msgid_);
            break;
        case message_fields::mf_prev_msg_id:
            prev_msg_id_ = tlv_field->get_value<int64_t>(prev_msg_id_);
            break;
        case message_fields::mf_flags:
            flags_.value_ = tlv_field->get_value<uint32_t>(0);
            break;
        case message_fields::mf_time:
            time_ = tlv_field->get_value<uint64_t>(0);
            break;
        case message_fields::mf_wimid:
            wimid_ = tlv_field->get_value<std::string>(std::string());
            break;
        case message_fields::mf_internal_id:
            internal_id_ = tlv_field->get_value<std::string>(std::string());
            break;
        case message_fields::mf_sender_friendly:
            sender_friendly_ = tlv_field->get_value<std::string>(std::string());
            break;
        case message_fields::mf_text:
            text_ = tlv_field->get_value<std::string>(std::string());
            break;
        case message_fields::mf_chat:
            {
                chat_ = std::make_unique<core::archive::chat_data>();
                core::tools::tlvpack pack = tlv_field->get_value<core::tools::tlvpack>();
                chat_->unserialize(pack);
            }
            break;
        case message_fields::mf_sticker:
            {
                sticker_ = std::make_unique<core::archive::sticker_data>();
                core::tools::tlvpack pack = tlv_field->get_value<core::tools::tlvpack>();
                sticker_->unserialize(pack);
            }
            break;
        case message_fields::mf_mult:
            {
                mult_ = std::make_unique<core::archive::mult_data>();
                core::tools::tlvpack pack = tlv_field->get_value<core::tools::tlvpack>();
                mult_->unserialize(pack);
            }
            break;
        case message_fields::mf_voip:
            {
                voip_ = std::make_unique<core::archive::voip_data>();
                const auto pack = tlv_field->get_value<core::tools::tlvpack>();
                if (!voip_->unserialize(pack))
                {
                    assert(!"voip unserialization failed");
                    voip_.reset();
                }
            }
            break;
        case message_fields::mf_file_sharing:
            {
                const auto pack = tlv_field->get_value<core::tools::tlvpack>();
                file_sharing_ = std::make_unique<core::archive::file_sharing_data>(pack);
            }
            break;
        case message_fields::mf_chat_event:
            {
                const auto pack = tlv_field->get_value<core::tools::tlvpack>();
                chat_event_ = chat_event_data::make_from_tlv(pack);
            }
            break;
        case message_fields::mf_quote:
            {
                quote q;
                const auto pack = tlv_field->get_value<core::tools::tlvpack>();
                q.unserialize(pack);
                quotes_.push_back(std::move(q));
            }
            break;
        case message_fields::mf_mention:
            {
                const auto pack = tlv_field->get_value<core::tools::tlvpack>();
                const auto sn = pack.get_item(mf_mention_sn);
                const auto fr = pack.get_item(mf_mention_friendly);
                if (sn && fr)
                {
                    const auto sn_str = sn->get_value<std::string>();
                    const auto fr_str = fr->get_value<std::string>();
                    if (!sn_str.empty() && !fr_str.empty())
                        mentions_.emplace(sn_str, fr_str);
                }
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

int32_t history_message::unserialize(const rapidjson::Value& _node,
                                     const std::string &_sender_aimid)
{
    assert(!_sender_aimid.empty());

    // load basic fields

    for (auto iter_field = _node.MemberBegin(), end = _node.MemberEnd(); iter_field != end ; ++iter_field)
    {
        const std::string name = rapidjson_get_string(iter_field->name);

        if (c_msgid == name)
        {
            msgid_ = iter_field->value.GetInt64();
        }
        else if (c_wimid == name)
        {
            wimid_ = rapidjson_get_string(iter_field->value);
        }
        else if (c_reqid == name)
        {
            internal_id_ = rapidjson_get_string(iter_field->value);
        }
        else if (c_outgoing == name)
        {
            flags_.flags_.outgoing_ = iter_field->value.GetBool();
        }
        else if (c_time == name)
        {
            time_ = iter_field->value.GetInt();
        }
        else if (c_text == name)
        {
            text_ = rapidjson_get_string(iter_field->value);
        }
    }

    const auto parts_iter = _node.FindMember(c_parts);
    if ((parts_iter != _node.MemberEnd()) && parts_iter->value.IsArray())
    {
        text_.clear();
        const auto &parts = parts_iter->value;
        const auto parts_count = parts.Size();
        for (auto i = 0u; i < parts_count; ++i)
        {
            const auto &part = parts[i];
            const auto type_iter = part.FindMember(c_media_type);
            if (type_iter != part.MemberEnd() && type_iter->value.IsString())
            {
                const auto & type = type_iter->value.GetString();
                if (type == c_type_text)
                {
                    const auto iter_text = part.FindMember(c_text);
                    if (iter_text != part.MemberEnd() && iter_text->value.IsString())
                    {
                        text_ = rapidjson_get_string(iter_text->value);
                    }
                    continue;
                }
                else if (type == c_type_sticker)
                {
                    const auto stickerId = part.FindMember(c_stiker_id);
                    if (stickerId != part.MemberEnd() && stickerId->value.IsString())
                    {
                        sticker_ = std::make_unique<core::archive::sticker_data>(rapidjson_get_string(stickerId->value));
                    }
                    continue;
                }
                else if (type == c_type_quote || type == c_type_forward)
                {
                    quote q;
                    q.unserialize(part, type == c_type_forward);
                    quotes_.push_back(std::move(q));
                    continue;
                }
            }
        }
    }

    // try to read a chat event if possible

    const auto event_class = probe_for_chat_event(_node);
    assert(event_class > chat_event_type_class::min);
    assert(event_class < chat_event_type_class::max);

    switch(event_class)
    {
    case chat_event_type_class::unknown:
        break;

    case chat_event_type_class::added_to_buddy_list:
        chat_event_ = chat_event_data::make_added_to_buddy_list(
            _sender_aimid
            );
        break;

    case chat_event_type_class::buddy_reg:
        chat_event_ = chat_event_data::make_simple_event(chat_event_type::buddy_reg);
        break;

    case chat_event_type_class::buddy_found:
        chat_event_ = chat_event_data::make_simple_event(chat_event_type::buddy_found);
        break;

    case chat_event_type_class::birthday:
        chat_event_ = chat_event_data::make_simple_event(chat_event_type::birthday);
        break;

    case chat_event_type_class::mchat:
        chat_event_ = chat_event_data::make_mchat_event(
            _node.FindMember("chat")->value
        );
        break;

    case chat_event_type_class::chat_modified:
        chat_event_ = chat_event_data::make_modified_event(
            _node.FindMember("chat")->value
        );
        break;

    case chat_event_type_class::generic:
        chat_event_ = chat_event_data::make_generic_event(
            _node.FindMember("text")->value
        );
        break;

    default:
        assert(!"unexpected event class");
    }

    if (chat_event_)
    {
        return 0;
    }

    // load other child structures

    for (auto iter_field = _node.MemberBegin(), end = _node.MemberEnd(); iter_field != end; ++iter_field)
    {
        const std::string name = rapidjson_get_string(iter_field->name);

        if (c_sticker == name)
        {
            sticker_ = std::make_unique<core::archive::sticker_data>();
            sticker_->unserialize(iter_field->value);
        }
        else if (c_mult == name)
        {
            mult_ = std::make_unique<core::archive::mult_data>();
            mult_->unserialize(iter_field->value);
        }
        else if (c_voip == name)
        {
            voip_ = std::make_unique<core::archive::voip_data>();
            if (!voip_->unserialize(iter_field->value, _sender_aimid))
            {
                assert(!"voip unserialization failed");
                voip_.reset();
            }
        }
        else if (c_chat == name)
        {
            chat_ = std::make_unique<core::archive::chat_data>();
            chat_->unserialize(iter_field->value);
        }
        else if (c_mentions == name)
        {
            mentions_.clear();
            const auto& mentions = iter_field->value;
            if (mentions.IsArray())
            {
                for (auto iter = mentions.Begin(), m_end = mentions.End(); iter != m_end; ++iter)
                {
                    if (iter->IsString())
                    {
                        const auto mention = rapidjson_get_string(*iter);
                        mentions_.emplace(mention, mention);
                    }
                }
            }
        }
    }

    return 0;
}

void history_message::jump_to_text_field(core::tools::binary_stream& _stream, uint32_t& length)
{
    while (_stream.available())
    {
        if (!core::tools::tlv::try_get_field_with_type(_stream, message_fields::mf_text, length))
            return;
        else if (length != 0)
            return;
    }
}

bool history_message::is_sticker(core::tools::binary_stream& _stream)
{
    uint32_t length;
    while (_stream.available())
    {
        if (!core::tools::tlv::try_get_field_with_type(_stream, message_fields::mf_sticker, length))
            return false;
        else if (length != 0)
            return true;
    }

    return false;
}

int64_t history_message::get_id_field(core::tools::binary_stream& _stream)
{
    uint32_t length = 0;
    while (_stream.available())
    {
        if (!core::tools::tlv::try_get_field_with_type(_stream, message_fields::mf_msg_id, length))
            return -1;
        else if (length != 0)
            return _stream.read<int64_t>();
    }

    return -1;
}

bool history_message::is_outgoing() const
{
    return flags_.flags_.outgoing_;
}

void history_message::set_outgoing(const bool _outgoing)
{
    flags_.flags_.outgoing_ = (int32_t)_outgoing;
}

bool history_message::is_patch() const
{
    return flags_.flags_.patch_;
}

void history_message::set_patch(const bool _patch)
{
    flags_.flags_.patch_ = (uint32_t)_patch;
}

bool history_message::is_deleted() const
{
    return flags_.flags_.deleted_;
}

bool history_message::is_chat_event_deleted() const
{
    return chat_event_ && chat_event_->is_type_deleted();
}

bool history_message::is_modified() const
{
    return flags_.flags_.modified_;
}

void history_message::reset_extended_data()
{
    sticker_.reset();
    mult_.reset();
    voip_.reset();
    file_sharing_.reset();
    chat_event_.reset();
}

void history_message::set_deleted(const bool _deleted)
{
    flags_.flags_.deleted_ = (uint32_t)_deleted;
}

void history_message::set_modified(const bool _modified)
{
    flags_.flags_.modified_ = (uint32_t)_modified;
}

void history_message::apply_header_flags(const message_header &_header)
{
    assert(!_header.is_patch());

    set_deleted(_header.is_deleted());
}

void history_message::apply_modifications(const history_block &_modifications)
{
    for (const auto &modification : _modifications)
    {
        assert(modification->get_msgid() == get_msgid());
        assert(modification->is_modified());

        if (modification->has_text())
        {
            __INFO(
                "delete_history",
                "applying modification\n"
                "    id=<%1%>\n"
                "    old-text=<%2%>\n"
                "    new-text=<%3%>",
                get_msgid() % get_text() % modification->get_text());

            reset_extended_data();

            set_text(modification->get_text());

            continue;
        }

        const auto &chat_event = modification->get_chat_event_data();
        if (chat_event)
        {
            __INFO(
                "delete_history",
                "applying modification\n"
                "    id=<%1%>\n"
                "    new-type=<chat-event>",
                get_msgid());

            reset_extended_data();

            chat_event_ = std::make_unique<core::archive::chat_event_data>(*chat_event);

            continue;
        }
    }
}

const quotes_vec& history_message::get_quotes() const
{
    return quotes_;
}

void history_message::attach_quotes(const quotes_vec& _quotes)
{
    quotes_ = _quotes;
}

const mentions_map& core::archive::history_message::get_mentions() const
{
    return mentions_;
}

void core::archive::history_message::set_mentions(const mentions_map& _mentions)
{
    mentions_ = _mentions;
}

message_flags history_message::get_flags() const
{
    return flags_;
}

message_type history_message::get_type() const
{
    if (is_sms())
    {
        return message_type::sms;
    }

    if (is_sticker())
    {
        return message_type::sticker;
    }

    if (is_file_sharing())
    {
        return message_type::file_sharing;
    }

    if (is_chat_event())
    {
        return message_type::chat_event;
    }

    if (is_voip_event())
    {
        return message_type::voip_event;
    }

    return message_type::base;
}

bool history_message::contents_equal(const history_message& _msg) const
{
    if (get_type() != _msg.get_type())
    {
        return false;
    }

    switch (get_type())
    {
        case message_type::base:
            return (get_text() == _msg.get_text());

        case message_type::file_sharing:
            assert(file_sharing_);
            assert(_msg.file_sharing_);
            return file_sharing_->contents_equal(*_msg.file_sharing_);

        case message_type::chat_event:
            assert(chat_event_);
            assert(_msg.chat_event_);
            return chat_event_->contents_equal(*_msg.chat_event_);

        default:
            return true;
    }

    return true;
}

void history_message::apply_persons_to_quotes(const archive::persons_map & _persons)
{
    for (auto& q : quotes_)
    {
        const auto iter_p = _persons.find(q.get_sender());
        if (iter_p != _persons.end())
            q.set_sender_friendly(iter_p->second.friendly_);
    }
}

void core::archive::history_message::apply_persons_to_mentions(const archive::persons_map & _persons)
{
    for (auto& it: mentions_)
    {
        const auto iter_p = _persons.find(it.first);
        if (iter_p != _persons.end())
        {
            it.second = iter_p->second.friendly_;
        }
    }
}

const std::string& history_message::get_text() const
{
    if (is_sticker())
    {
        assert(sticker_);
        if (sticker_)
            return sticker_->get_id();
    }

    return text_;
}

void history_message::set_text(const std::string& _text)
{
    text_ = _text;
}

bool history_message::has_text() const
{
    return !text_.empty();
}

quote::quote()
    : time_(-1)
    , setId_(-1)
    , stickerId_(-1)
    , msg_id_(-1)
    , is_forward_(false)
{
}


void quote::serialize(icollection* _collection) const
{
    coll_helper helper(_collection, false);
    helper.set_value_as_string("text", text_);
    helper.set_value_as_string("sender", sender_);
    helper.set_value_as_string("senderFriendly", senderFriendly_);
    helper.set_value_as_string("chatId", chat_);
    helper.set_value_as_int("time", time_);
    helper.set_value_as_int64("msg", msg_id_);
    helper.set_value_as_bool("forward", is_forward_);
    if (setId_ != -1)
        helper.set_value_as_int("setId", setId_);
    if (stickerId_ != -1)
        helper.set_value_as_int("stickerId", stickerId_);
    helper.set_value_as_string("stamp", chat_stamp_);
    helper.set_value_as_string("chatName", chat_name_);
}

void quote::serialize(core::tools::tlvpack& _pack) const
{
    if (!text_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_text, text_));

    if (!sender_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_sn, sender_));

    if (!chat_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_chat_id, chat_));

    if (time_ != -1)
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_time, time_));

    if (msg_id_ != -1)
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_msg_id, msg_id_));

    if (!senderFriendly_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_friendly, senderFriendly_));

    if (setId_ != -1)
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_set_id, setId_));

    if (stickerId_ != -1)
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_sticker_id, stickerId_));

    _pack.push_child(core::tools::tlv(message_fields::mf_quote_is_forward, is_forward_));

    if (!chat_stamp_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_chat_stamp, chat_stamp_));

    if (!chat_name_.empty())
        _pack.push_child(core::tools::tlv(message_fields::mf_quote_chat_name, chat_name_));
}

void quote::unserialize(icollection* _coll)
{
    coll_helper helper(_coll, false);

    text_ = helper.get_value_as_string("text");
    sender_ = helper.get_value_as_string("sender");
    senderFriendly_ = helper.get_value_as_string("senderFriendly");
    chat_ = helper.get_value_as_string("chatId");
    time_ = helper.get_value_as_int("time");
    msg_id_ = helper.get_value_as_int64("msg");
    is_forward_ = helper.get_value_as_bool("forward");

    if (helper.is_value_exist("setId"))
        setId_ = helper.get_value_as_int("setId");

    if (helper.is_value_exist("stickerId"))
        stickerId_ = helper.get_value_as_int("stickerId");

    chat_stamp_ = helper.get_value_as_string("stamp");
    chat_name_ = helper.get_value_as_string("chatName");
}

void quote::unserialize(const rapidjson::Value& _node, bool _is_forward)
{
    const auto text_iter = _node.FindMember(c_text);
    if (text_iter != _node.MemberEnd() && text_iter->value.IsString())
        text_ = rapidjson_get_string(text_iter->value);

    const auto sn_iter = _node.FindMember(c_sn);
    if (sn_iter != _node.MemberEnd() && sn_iter->value.IsString())
        sender_ = rapidjson_get_string(sn_iter->value);

    const auto msg_id_iter = _node.FindMember(c_msgid);
    if (msg_id_iter != _node.MemberEnd())
    {
        if (msg_id_iter->value.IsInt64())
        {
            msg_id_ = msg_id_iter->value.GetInt64();
        }
        else if (msg_id_iter->value.IsString())
        {
            std::string id = rapidjson_get_string(msg_id_iter->value);
            std::stringstream ss;
            ss << id;
            ss >> msg_id_;
        }
    }

    const auto time_iter = _node.FindMember(c_time);
    if (time_iter != _node.MemberEnd() && time_iter->value.IsInt())
        time_ = time_iter->value.GetInt();

    const auto sticker_iter = _node.FindMember(c_sticker_id);
    if (sticker_iter != _node.MemberEnd() && sticker_iter->value.IsString())
    {
        auto ids = sticker_data::get_ids(rapidjson_get_string(sticker_iter->value));
        setId_ = ids.first;
        stickerId_ = ids.second;
    }

    is_forward_ = _is_forward;

    const auto chat_iter = _node.FindMember(c_chat);
    if (chat_iter != _node.MemberEnd() && chat_iter->value.IsObject())
    {
        const rapidjson::Value& chat_node = chat_iter->value;

        const auto chat_sn_iter = chat_node.FindMember(c_sn);
        if (chat_sn_iter != chat_node.MemberEnd() && chat_sn_iter->value.IsString())
            chat_ = rapidjson_get_string(chat_sn_iter->value);

        const auto chat_stamp_iter = chat_node.FindMember(c_chat_stamp);
        if (chat_stamp_iter != chat_node.MemberEnd() && chat_stamp_iter->value.IsString())
            chat_stamp_ = rapidjson_get_string(chat_stamp_iter->value);

        const auto chat_cn_iter = chat_node.FindMember(c_chat_name);
        if (chat_cn_iter != chat_node.MemberEnd() && chat_cn_iter->value.IsString())
            chat_name_ = rapidjson_get_string(chat_cn_iter->value);
    }
}

void quote::unserialize(const core::tools::tlvpack &_pack)
{
    auto get_value = [_pack](auto _field, auto _def_value, Out auto _out_ptr)
    {
        const auto item = _pack.get_item(_field);
        if (item)
            *_out_ptr = item->template get_value<std::remove_pointer_t<decltype(_out_ptr)>>(_def_value);
    };
    get_value(message_fields::mf_quote_text,    std::string(),  &text_);
    get_value(message_fields::mf_quote_sn,      std::string(),  &sender_);
    get_value(message_fields::mf_quote_chat_id, std::string(),  &chat_);
    get_value(message_fields::mf_quote_time,    -1,             &time_);
    get_value(message_fields::mf_quote_msg_id,  -1,             &msg_id_);
    get_value(message_fields::mf_quote_friendly, std::string(), &senderFriendly_);
    get_value(message_fields::mf_quote_set_id,  -1,             &setId_);
    get_value(message_fields::mf_quote_is_forward, false,       &is_forward_);
    get_value(message_fields::mf_quote_sticker_id, int32_t(-1), &stickerId_);
    get_value(message_fields::mf_quote_chat_stamp,std::string(),&chat_stamp_);
    get_value(message_fields::mf_quote_chat_name, std::string(),&chat_name_);
}

std::string quote::get_sticker() const
{
     if (setId_ == -1 && stickerId_ == -1)
         return std::string();

     std::stringstream ss_message;
     ss_message << "ext:" << setId_ << ":sticker:" << stickerId_;

     return ss_message.str();
};

namespace
{
    const auto INDEX_DIVISOR = 62;

    typedef std::unordered_map<char, int32_t> ReverseIndexMap;

    const ReverseIndexMap& get_reverse_index_map()
    {
        static ReverseIndexMap map;

        if (!map.empty())
        {
            return map;
        }

        auto index = 0;

        const auto fill_map = [&index](const char from, const char to)
        {
            for (auto ch = from; ch <= to; ++ch, ++index)
            {
                map.emplace(ch, index);
            }
        };

        fill_map('0', '9');
        fill_map('a', 'z');
        fill_map('A', 'Z');

        assert(map.size() == INDEX_DIVISOR);

        return map;
    }

    int32_t calculate_size(const char ch0, const char ch1)
    {
        const auto &map = get_reverse_index_map();

        const auto index0 = map.at(ch0);
        const auto index1 = map.at(ch1);

        auto size = (index0 * INDEX_DIVISOR);
        size += index1;

        return size;
    }

    std::string parse_sender_aimid(const rapidjson::Value &_node)
    {
        auto sender_iter = _node.FindMember("sender");
        if ((sender_iter == _node.MemberEnd()) || !sender_iter->value.IsString())
        {
            return std::string();
        }

        std::string sender_aimid = rapidjson_get_string(sender_iter->value);
        assert(!sender_aimid.empty());

        return sender_aimid;
    }

    void serialize_metadata_from_uri(const std::string &uri, Out coll_helper &coll)
    {
        assert(!uri.empty());

        std::string id;

        if (!tools::parse_new_file_sharing_uri(uri, Out id))
        {
            return;
        }

        assert(!id.empty());

        auto content_type = core::file_sharing_content_type::undefined;

        if (!tools::get_content_type_from_uri(uri, content_type))
        {
            return;
        }

        const auto width = calculate_size(id[1], id[2]);
        const auto height = calculate_size(id[3], id[4]);

        assert(width >= 0);
        assert(height >= 0);

        const auto PREVIEW_SIZE_MIN = 2;
        const auto is_malformed_uri = ((width < PREVIEW_SIZE_MIN) || (height < PREVIEW_SIZE_MIN));
        if (is_malformed_uri)
        {
            // the server-side bug, most probably
            __WARN(
                "fs",
                "malformed preview uri detected\n"
                "	uri=<%1%>\n"
                "	size=<%2%;%3%>",
                uri % width % height
                );
            return;
        }

        coll.set_value_as_int("content_type", (int32_t) content_type);

        coll.set_value_as_int("width", width);
        coll.set_value_as_int("height", height);
    }

    std::string find_person(const std::string &_aimid, const persons_map &_persons)
    {
        assert(!_aimid.empty());

        const auto iter_p = _persons.find(_aimid);
        if (iter_p != _persons.end())
            return iter_p->second.friendly_;
        return _aimid;
    }

    bool is_generic_event(const rapidjson::Value& _node)
    {
        assert(_node.IsObject());

        const auto text_iter = _node.FindMember("text");
        if (text_iter == _node.MemberEnd() ||
            !text_iter->value.IsString() ||
            rapidjson_get_string(text_iter->name).empty())
        {
            return false;
        }

        const auto voip_iter = _node.FindMember("voip");
        if (voip_iter != _node.MemberEnd())
        {
            return false;
        }

        const auto class_iter = _node.FindMember("class");
        if (
            (class_iter != _node.MemberEnd()) &&
            class_iter->value.IsString() &&
            (class_iter->value.GetString() == c_event_class)
            )
        {
            return true;
        }

        const auto chat_node_iter = _node.FindMember("chat");
        if (chat_node_iter == _node.MemberEnd())
        {
            return false;
        }

        const auto &chat_node = chat_node_iter->value;
        if (!chat_node.IsObject())
        {
            return false;
        }

        const auto modified_info_iter = chat_node.FindMember("modifiedInfo");
        if (modified_info_iter == chat_node.MemberEnd())
        {
            return false;
        }

        const auto &modified_info = modified_info_iter->value;
        if (!modified_info.IsObject())
        {
            return false;
        }

        if (modified_info.HasMember("public"))
        {
            return true;
        }

        return false;
    }

    chat_event_type_class probe_for_chat_event(const rapidjson::Value& _node)
    {
        const auto added_to_buddy_list_iter = _node.FindMember("addedToBuddyList");
        if (added_to_buddy_list_iter != _node.MemberEnd())
        {
            return chat_event_type_class::added_to_buddy_list;
        }

        const auto buddy_reg_iter = _node.FindMember("buddyReg");
        if (buddy_reg_iter != _node.MemberEnd())
        {
            return chat_event_type_class::buddy_reg;
        }

        const auto buddy_bday_iter = _node.FindMember("bday");
        if (buddy_bday_iter != _node.MemberEnd())
        {
            return chat_event_type_class::birthday;
        }

        const auto buddy_found_iter = _node.FindMember("buddyFound");
        if (buddy_found_iter != _node.MemberEnd())
        {
            return chat_event_type_class::buddy_found;
        }

        const auto modified_event = probe_for_modified_event(_node);
        if (modified_event != chat_event_type_class::unknown)
        {
            return modified_event;
        }

        if (is_generic_event(_node))
        {
            return chat_event_type_class::generic;
        }

        return chat_event_type_class::unknown;
    }

    chat_event_type_class probe_for_modified_event(const rapidjson::Value& _node)
    {
        assert(_node.IsObject());

        const auto chat_node_iter = _node.FindMember("chat");
        if (chat_node_iter == _node.MemberEnd())
        {
            return chat_event_type_class::unknown;
        }

        const auto &chat_node = chat_node_iter->value;
        if (!chat_node.IsObject())
        {
            return chat_event_type_class::unknown;
        }

        const auto modified_info_iter = chat_node.FindMember("modifiedInfo");
        if (modified_info_iter != chat_node.MemberEnd())
        {
            const auto &modified_info = modified_info_iter->value;

            if (modified_info.IsObject() && (
                    modified_info.HasMember("name") ||
                    modified_info.HasMember("rules") ||
                    modified_info.HasMember("avatarLastModified") ||
                    modified_info.HasMember("about")))
            {
                return chat_event_type_class::chat_modified;
            }
        }

        const auto member_event_iter = chat_node.FindMember("memberEvent");
        if (member_event_iter != chat_node.MemberEnd())
        {
            return chat_event_type_class::mchat;
        }

        return chat_event_type_class::unknown;
    }

    str_set read_members(const rapidjson::Value &_parent)
    {
        assert(_parent.IsObject());

        str_set result;

        const auto members_iter = _parent.FindMember("members");
        if ((members_iter == _parent.MemberEnd()) || !members_iter->value.IsArray())
        {
            return result;
        }

        const auto &members_array = members_iter->value;

        const auto members_count = members_array.Size();
        for (auto index = 0u; index < members_count; ++index)
        {
            const auto &member_value = members_array[index];

            if (!member_value.IsString())
            {
                continue;
            }

            std::string member = rapidjson_get_string(member_value);
            assert(!member.empty());

            if (!member.empty())
            {
                result.emplace(std::move(member));
            }
        }

        return result;
    }

    chat_event_type string_2_chat_event(const std::string &_str)
    {
        static std::map<std::string, chat_event_type> mapping;

        if (mapping.empty())
        {
            mapping.emplace("addMembers", chat_event_type::mchat_add_members);
            mapping.emplace("invite", chat_event_type::mchat_invite);
            mapping.emplace("leave", chat_event_type::mchat_leave);
            mapping.emplace("delMembers", chat_event_type::mchat_del_members);
            mapping.emplace("kicked", chat_event_type::mchat_kicked);
        }

        const auto iter = mapping.find(_str);
        if (iter == mapping.end())
        {
            return chat_event_type::invalid;
        }

        const auto type = std::get<1>(*iter);
        assert(type > chat_event_type::min);
        assert(type < chat_event_type::max);

        return type;
    }
}
