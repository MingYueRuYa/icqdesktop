#include "stdafx.h"

#include <sstream>
#include <time.h>

#include "get_sms_code.h"
#include "../../../http_request.h"
#include "../../../core.h"
#include "../../../../corelib/enumerations.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;

validate_phone::validate_phone(
    wim_packet_params params,
    const std::string& phone,
    const std::string& locale)
    :
wim_packet(std::move(params)),
    phone_(phone),
    locale_(locale),
    code_length_(0),
    existing_(false)
{
    set_can_change_hosts_scheme(true);
}


validate_phone::~validate_phone()
{
}


int32_t validate_phone::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;
    ss_url << "https://www.icq.com/smsreg/requestPhoneValidation.php?" <<
        "&locale=" << locale_ <<
        "&msisdn=" << phone_ <<
        "&smsFormatType=human" <<
        "&k=" << params_.dev_id_ <<
        "&r=" <<  core::tools::system::generate_guid();


    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    return 0;
}

int32_t core::wim::validate_phone::on_response_error_code()
{
    switch (status_code_)
    {
    case 465:
        return wpie_error_rate_limit;
    default:
        return wpie_get_sms_code_unknown_error;
    }
}

int32_t core::wim::validate_phone::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_trans_id = _data.FindMember("trans_id");
    if (iter_trans_id == _data.MemberEnd() || !iter_trans_id->value.IsString())
        return wpie_http_parse_response;

    trans_id_ = rapidjson_get_string(iter_trans_id->value);

    auto iter_existing = _data.FindMember("existring");
    if (iter_existing != _data.MemberEnd() && iter_existing->value.IsInt())
        existing_ = !!iter_existing->value.GetInt();

    auto iter_code_length = _data.FindMember("code_length");
    if (iter_code_length != _data.MemberEnd() && iter_code_length->value.IsInt())
        code_length_ = iter_code_length->value.GetInt();

    g_core->insert_event(core::stats::stats_event_names::reg_sms_send);
    return 0;
}
