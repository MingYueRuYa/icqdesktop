#include "stdafx.h"

#include "platform.h"
#include "utils/utils.h"

#include "fonts.h"

FONTS_NS_BEGIN

namespace
{
    typedef std::unordered_map<FontWeight, QString> QssWeightsMapT;

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font);

    void applyFontWeight(
        const FontFamily _fontFamily,
        const FontWeight _fontWeight,
        const int32_t _sizePx,
        Out QFont &_font);

    const QssWeightsMapT& getCurrentWeightsMap();

    const QssWeightsMapT& getOpenSansWeightsMap();

    const QssWeightsMapT& getSegoeUIWeightsMap();

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontStyle);

    QFont::Weight icqWeight2QtWeight(const FontWeight _internalWeight);

    QString segoeUiFamilyName(const FontWeight _weight);

}

QFont appFont(const int32_t _sizePx)
{
    return appFont(_sizePx, defaultAppFontFamily(), defaultAppFontWeight());
}

QFont appFont(const int32_t _sizePx, const FontFamily _family)
{
    return appFont(_sizePx, _family, defaultAppFontWeight());
}

QFont appFont(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(_sizePx, defaultAppFontFamily(), _weight);
}

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    assert(_sizePx > 0);
    assert(_sizePx < 500);
    assert(_family > FontFamily::MIN);
    assert(_family < FontFamily::MAX);
    assert(_weight > FontWeight::Min);
    assert(_weight < FontWeight::Max);

    QFont result;

    result.setPixelSize(_sizePx);

    applyFontFamily(_family, Out result);

    applyFontWeight(_family, _weight, _sizePx, Out result);

    return result;
}

QFont appFontScaled(const int32_t _sizePx)
{
    return appFont(Utils::scale_value(_sizePx));
}

QFont appFontScaled(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _weight);
}

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _family, _weight);
}

FontFamily defaultAppFontFamily()
{
    if (platform::is_windows_vista_or_late())
    {
        return FontFamily::SEGOE_UI;
    }

    return FontFamily::OPEN_SANS;
}

FontWeight defaultAppFontWeight()
{
    return FontWeight::Normal;
}

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    assert(_sizePx > 0);
    assert(_sizePx < 1000);
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);
    assert(_fontWeight > FontWeight::Min);
    assert(_fontWeight < FontWeight::Max);

    QString result;
    result.reserve(512);

    result = ql1s("font-size: ")
        % QString::number(_sizePx)
        % ql1s("px; font-family: \"")
        % appFontFamilyNameQss(_fontFamily, _fontWeight)
        % ql1c('"');

    const auto weight = evalQssFontWeight(_fontFamily, _fontWeight);
    if (!weight.isEmpty())
        result += (ql1s("; font-weight: ") % weight);

    return result;
}

QString appFontFamilyNameQss(const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);

    switch (_fontFamily)
    {
        case FontFamily::OPEN_SANS:
            return qsl("Open Sans");

        case FontFamily::SEGOE_UI:
            return segoeUiFamilyName(_fontWeight);
    }

    assert(!"unexpected font family");
    return qsl("Comic Sans");
}

QString appFontWeightQss(const FontWeight _weight)
{
    const auto &weightMap = getCurrentWeightsMap();

    const auto iter = weightMap.find(_weight);
    if (iter == weightMap.end())
    {
        assert(!"unknown font weight");
        return defaultAppFontQssWeight();
    }

    const auto &fontWeight = iter->second;

    return fontWeight;
}

QString defaultAppFontQssName()
{
    return appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
}

QString defaultAppFontQssWeight()
{
    const auto &weights = getCurrentWeightsMap();

    auto iter = weights.find(defaultAppFontWeight());
    assert(iter != weights.end());

    return iter->second;
}

QString SetFont(QString _qss)
{
    QString result(std::move(_qss));

    const auto fontFamily = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
    const auto fontFamilyBold = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Bold);
    const auto fontFamilyMedium = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Medium);
    const auto fontFamilyLight = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Light);

    result.replace(ql1s("%FONT_FAMILY%"), fontFamily);
    result.replace(ql1s("%FONT_FAMILY_BOLD%"), fontFamilyBold);
    result.replace(ql1s("%FONT_FAMILY_MEDIUM%"), fontFamilyMedium);
    result.replace(ql1s("%FONT_FAMILY_LIGHT%"), fontFamilyLight);

    const auto fontWeightQss = appFontWeightQss(FontWeight::Normal);
    const auto fontWeightBold = appFontWeightQss(FontWeight::Bold);
    const auto fontWeightMedium = appFontWeightQss(FontWeight::Medium);
    const auto fontWeightLight = appFontWeightQss(FontWeight::Light);

    result.replace(ql1s("%FONT_WEIGHT%"), fontWeightQss);
    result.replace(ql1s("%FONT_WEIGHT_BOLD%"), fontWeightBold);
    result.replace(ql1s("%FONT_WEIGHT_MEDIUM%"), fontWeightMedium);
    result.replace(ql1s("%FONT_WEIGHT_LIGHT%"), fontWeightLight);

    return result;
}

namespace
{
    const QssWeightsMapT& getCurrentWeightsMap()
    {
        if (platform::is_windows_vista_or_late())
            return getSegoeUIWeightsMap();

        return getOpenSansWeightsMap();
    }

    const QssWeightsMapT& getOpenSansWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, qsl("300"));
            weightMap.emplace(FontWeight::Normal, qsl("400"));
            weightMap.emplace(FontWeight::Medium, qsl("500"));
            weightMap.emplace(FontWeight::Bold, qsl("700"));
        }

        return weightMap;
    }

    const QssWeightsMapT& getSegoeUIWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, qsl("200"));
            weightMap.emplace(FontWeight::Normal, qsl("400"));
            weightMap.emplace(FontWeight::Medium, qsl("450"));
            weightMap.emplace(FontWeight::Bold, qsl("800"));
        }

        return weightMap;
    }

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontWeight)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontWeight > FontWeight::Min);
        assert(_fontWeight < FontWeight::Max);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            return QString();
        }

        if (_fontFamily == FontFamily::OPEN_SANS)
        {
            return appFontWeightQss(_fontWeight);
        }

        assert(!"unknown font family / style comnbination");
        return defaultAppFontQssWeight();
    }

    QFont::Weight icqWeight2QtWeight(const FontWeight _internalWeight)
    {
        assert(_internalWeight > FontWeight::Min);
        assert(_internalWeight < FontWeight::Max);

        switch (_internalWeight)
        {
            case FontWeight::Light: return QFont::Weight::Light;

            case FontWeight::Normal: return QFont::Weight::Normal;

            case FontWeight::Medium: return QFont::Weight::Medium;

            case FontWeight::Bold: return QFont::Weight::Bold;
        }

        assert(!"unknown enum value");
        return QFont::Weight::Normal;
    }

    QString segoeUiFamilyName(const FontWeight _weight)
    {
        assert(_weight > FontWeight::Min);
        assert(_weight < FontWeight::Max);

        QString familyName;
        familyName.reserve(1024);

        familyName += ql1s("Segoe UI");

        switch (_weight)
        {
            case FontWeight::Light:
                familyName += ql1s(" Light");
                break;

            case FontWeight::Normal:
                break;

            case FontWeight::Medium:
                familyName += ql1s(" Semibold");
                break;

            case FontWeight::Bold:
                break;

            default:
                assert(!"unknown font weight");
                break;
        }

        return familyName;
    }

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);

        switch (_fontFamily)
        {
            case FontFamily::SEGOE_UI:
                _font.setFamily(qsl("Segoe UI"));
                return;

            case FontFamily::OPEN_SANS:
                _font.setFamily(qsl("Open Sans"));
                return;

            default:
                assert(!"unexpected font family");
                return;
        }
    }

    void applyFontWeight(
        const FontFamily _fontFamily,
        const FontWeight _fontWeight,
        int32_t _sizePx,
        Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontWeight > FontWeight::Min);
        assert(_fontWeight < FontWeight::Max);
        assert(_sizePx > 0);
        assert(_sizePx < 1000);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            switch (_fontWeight)
            {
                case FontWeight::Light:
                    _font.setFamily(qsl("Segoe UI Light"));
                    return;

                case FontWeight::Normal:
                    return;

                case FontWeight::Medium:
                    _font.setFamily(qsl("Segoe UI Semibold"));
                    return;

                case FontWeight::Bold:
                    _font.setWeight(
                        icqWeight2QtWeight(FontWeight::Bold));
                    return;

                default:
                    assert(!"unexpected font style");
                    return;
            }
        }


        if (_fontFamily == FontFamily::OPEN_SANS)
        {
            switch (_fontWeight)
            {
                case FontWeight::Normal:
                    _font.setWeight(QFont::Weight::Normal);
                    return;

                case FontWeight::Light:
                    _font.setWeight(QFont::Weight::Light);
                    return;

                case FontWeight::Medium:
                    _font.setWeight(QFont::Weight::DemiBold);
                    return;

                case FontWeight::Bold:
                    _font.setWeight(icqWeight2QtWeight(FontWeight::Bold));
                    return;
            }

            assert(!"unexpected font style");
            return;
        }

        assert(!"unexpected font family");
        return;
    }
}

FONTS_NS_END
