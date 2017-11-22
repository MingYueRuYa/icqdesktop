#include "stdafx.h"

#include "log.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

using namespace Ui;

namespace
{
    void log(const QString &type, const QString &area, const QString &text);
}

namespace Log
{
    void trace(const QString& area, const QString& text)
    {
        log(qsl("trace"), area, text);
    }

    void info(const QString& area, const QString& text)
    {
        log(qsl("info"), area, text);
    }

    void warn(const QString& area, const QString& text)
    {
        log(qsl("warn"), area, text);
    }

    void error(const QString& area, const QString& text)
    {
        log(qsl("error"), area, text);
    }

}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_content_type arg)
{
    using namespace core;

    assert(arg > file_sharing_content_type::min);
    assert(arg < file_sharing_content_type::max);

    switch (arg)
    {
        case file_sharing_content_type::gif: return (oss << ql1s("gif"));
        case file_sharing_content_type::image: return (oss << ql1s("image"));
        case file_sharing_content_type::ptt: return (oss << ql1s("ptt"));
        case file_sharing_content_type::undefined: return (oss << ql1s("undefined"));
        case file_sharing_content_type::video: return (oss << ql1s("video"));
        default:
            assert(!"unexpected file sharing content type");
    }

    return (oss << ql1s("#unknown"));
}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_function arg)
{
    using namespace core;

    assert(arg > file_sharing_function::min);
    assert(arg < file_sharing_function::max);

    switch(arg)
    {
        case file_sharing_function::check_local_copy_exists:
            oss << ql1s("check_local_copy_exists");
            break;

        case file_sharing_function::download_file:
            oss << ql1s("download_file");
            break;

        case file_sharing_function::download_file_metainfo:
            oss << ql1s("download_file_metainfo");
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << ql1s("download_preview_metainfo");
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
    }

    return oss;
}

namespace
{
    void log(const QString &type, const QString &area, const QString &text)
    {
        assert(!type.isEmpty());
        assert(!area.isEmpty());
        assert(!text.isEmpty());

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("type", type);
        collection.set_value_as_qstring("area", area);
        collection.set_value_as_string("text", text.toStdString());

        Ui::GetDispatcher()->post_message_to_core(qsl("log"), collection.get());
    }
}