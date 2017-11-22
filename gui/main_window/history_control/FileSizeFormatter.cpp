#include "stdafx.h"

#include "FileSizeFormatter.h"

namespace HistoryControl
{

    QString formatFileSize(const int64_t size)
    {
        assert(size >= 0);

        const auto KiB = 1024;
        const auto MiB = 1024 * KiB;
        const auto GiB = 1024 * MiB;

        if (size >= GiB)
        {
            const auto gibSize = ((double)size / (double)GiB);

            return qsl("%1 GB").arg(gibSize, 0, 'f', 1);
        }

        if (size >= MiB)
        {
            const auto mibSize = ((double)size / (double)MiB);

            return qsl("%1 MB").arg(mibSize, 0, 'f', 1);
        }

        if (size >= KiB)
        {
            const auto kibSize = ((double)size / (double)KiB);

            return qsl("%1 KB").arg(kibSize, 0, 'f', 1);
        }

        return qsl("%1 B").arg(size);
    }

    QString formatProgressText(const int64_t bytesTotal, const int64_t bytesTransferred)
    {
        using namespace HistoryControl;

        assert(bytesTransferred >= -1);
        QString result;

        const auto isBytesTotalValid = (bytesTotal > 0);
        if (!isBytesTotalValid)
        {
            assert(!"invalid bytes counter");
            return result;
        }
        result.reserve(128);

        if (bytesTransferred > 0)
        {
            result += formatFileSize(bytesTransferred);
            result += QT_TRANSLATE_NOOP("chat_page", " of ");
        }

        result += formatFileSize(bytesTotal);

        return result;
    }

}