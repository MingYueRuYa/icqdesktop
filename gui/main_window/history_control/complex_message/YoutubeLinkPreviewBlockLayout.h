#pragma once

#include "ILinkPreviewBlockLayout.h"
#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class LinkPreviewBlock;

class YoutubeLinkPreviewBlockLayout final
    : public GenericBlockLayout
    , public ILinkPreviewBlockLayout
{
public:
    YoutubeLinkPreviewBlockLayout();

    virtual ~YoutubeLinkPreviewBlockLayout() override;

    virtual IItemBlockLayout* asBlockLayout() override;

    virtual QLayout* asQLayout() override;

    const IItemBlockLayout::IBoxModel& getBlockBoxModel() const override;

    virtual QSize getMaxPreviewSize() const override;

    virtual QRect getFaviconImageRect() const override;

    virtual QRect getPreviewImageRect() const override;

    virtual QPoint getSiteNamePos() const override;

    virtual QRect getSiteNameRect() const override;

    virtual QFont getTitleFont() const override;

    virtual QRect getTitleRect() const override;

protected:
    void setActionButtonGeometry(const QRect &previewRect, LinkPreviewBlock &block);

    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

private:
    QRect evaluateFaviconImageRect(const LinkPreviewBlock &block, const QRect &titleGeometry) const;

    QRect evaluatePreviewImageRect(const LinkPreviewBlock &block, const QRect &previewContentLtr) const;

    QRect evaluateSiteNameGeometry(const LinkPreviewBlock &block, const QRect &titleGeometry, const QRect &faviconGeometry) const;

    QRect evaluateTitleLtr(const QRect &previewContentLtr, const QRect &previewImageRect, const bool isPlaceholder);

    int32_t evaluateWidgetHeight(
        const QRect &previewImageGeometry,
        const QRect &faviconGeometry,
        const QRect &siteNameGeometry,
        const bool isPlaceholder);

    QRect setTitleGeometry(LinkPreviewBlock &block, const QRect &titleLtr);

    QRect PreviewImageRect_;

    QRect CurrentTitleGeometry_;

    QRect FaviconImageRect_;

    QRect SiteNameGeometry_;

    QRect TitleGeometry_;

};

UI_COMPLEX_MESSAGE_NS_END