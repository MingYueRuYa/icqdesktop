#include "stdafx.h"

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

#include "FileSharingInfo.h"

using namespace core;

namespace HistoryControl
{

    FileSharingInfo::FileSharingInfo(const coll_helper &info)
    {
        IsOutgoing_ = info.get_value_as_bool("outgoing");

        UploadingProcessId_ = QString::fromUtf8(info.get_value_as_string("uploading_id", ""));

        if (info.is_value_exist("local_path"))
        {
            assert(!info.is_value_exist("uri"));
            assert(IsOutgoing_);
            LocalPath_ = QString::fromUtf8(info.get_value_as_string("local_path"));
        }
        else
        {
            Uri_ = QString::fromUtf8(info.get_value_as_string("uri"));
            assert(!Uri_.isEmpty());
        }

        if (!info.is_value_exist("content_type"))
        {
            return;
        }

        ContentType_ = (file_sharing_content_type)info.get_value_as_int("content_type");

        const auto width = info.get_value_as_int("width");
        const auto height = info.get_value_as_int("height");
        assert(width > 0);
        assert(height > 0);
        Size_ = std::make_unique<QSize>(width, height);
    }

    const QString& FileSharingInfo::GetUri() const
    {
        assert(!Uri_.isEmpty());
        return Uri_;
    }

    const QString& FileSharingInfo::GetLocalPath()
    {
        assert(IsOutgoing());

        return LocalPath_;
    }

    QSize FileSharingInfo::GetSize() const
    {
        return Size_ ? *Size_ : QSize();
    }

    bool FileSharingInfo::IsOutgoing() const
    {
        return IsOutgoing_;
    }

    const QString& FileSharingInfo::GetUploadingProcessId() const
    {
        assert(IsOutgoing());

        return UploadingProcessId_;
    }

    bool FileSharingInfo::HasLocalPath() const
    {
        return !LocalPath_.isEmpty();
    }

    bool FileSharingInfo::HasSize() const
    {
        return !!Size_;
    }

    bool FileSharingInfo::HasUri() const
    {
        return !Uri_.isEmpty();
    }

    void FileSharingInfo::SetUri(const QString &uri)
    {
        assert(Uri_.isEmpty());
        assert(IsOutgoing());

        Uri_ = uri;
    }

    QString FileSharingInfo::ToLogString() const
    {
        return ql1s("\toutgoing=<")
        % ql1s(logutils::yn(IsOutgoing_))
        % ql1s(">\n")
        % ql1s("\tlocal_path=<")
        % LocalPath_
        % ql1s(">\n")
        % ql1s("\tuploading_id=<")
        % UploadingProcessId_
        % ql1s(">\n")
        % ql1s("\turi=<")
        % Uri_
        % ql1c('>');
    }

    core::file_sharing_content_type FileSharingInfo::getContentType() const
    {
        return ContentType_;
    }

    void FileSharingInfo::setContentType(core::file_sharing_content_type type)
    {
        ContentType_ = type;
    }
}