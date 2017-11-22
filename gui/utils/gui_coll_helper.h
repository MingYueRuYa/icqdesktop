#pragma once

#include "../../corelib/collection_helper.h"

namespace Ui
{
    class gui_coll_helper : public core::coll_helper
    {
    public:

        gui_coll_helper(core::icollection* _collection, bool _autoRelease);

        void set_value_as_qstring(const char* _name, const QString& _value);
    };
}

namespace core
{

    template<>
    inline QString coll_helper::get<QString>(const char *_name) const
    {
        return QString::fromUtf8(get_value_as_string(_name));
    }

    template<>
    inline QString coll_helper::get<QString>(const char *_name, const char *_def) const
    {
        return QString::fromUtf8(get_value_as_string(_name, _def));
    }

    template<>
    inline void coll_helper::set<QString>(const char *_name, const QString &_value)
    {
        set_value_as_string(_name, _value.toStdString());
    }

    template<>
    inline void coll_helper::set<QUrl>(const char *_name, const QUrl &_value)
    {
        set<QString>(_name, _value.toString());
    }

}