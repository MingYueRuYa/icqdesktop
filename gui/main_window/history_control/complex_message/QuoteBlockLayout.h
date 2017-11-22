#pragma once

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class QuoteBlockLayout final : public GenericBlockLayout
{
public:
    QuoteBlockLayout();

    virtual ~QuoteBlockLayout() override;

    const IItemBlockLayout::IBoxModel& getBlockBoxModel() const override;

    virtual QRect getBlockGeometry() const override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &widgetGeometry) override;

private:
    QRect evaluateContentLtr(const QRect &widgetGeometry) const;

    QRect setTextControlGeometry(const QRect &contentLtr);

    QRect CurrentTextCtrlGeometry_;

};

UI_COMPLEX_MESSAGE_NS_END