#pragma once

namespace Ui
{
    namespace countries
    {
        struct country
        {
            const int phone_code_;
            const QString name_;
            const QString iso_code_;

            country(
                int _phone_code,
                const QString& _name,
                const QString& _iso_code)
                :
                phone_code_(_phone_code),
                name_(_name),
                iso_code_(_iso_code)
            {
            }
        };

        typedef std::vector<country> countries_list;

        const countries_list& get();
    }

}


