#pragma once

#include "MessagesModel.h"

namespace Ui
{
    class MessagesScrollbar;
    class MessageItem;
    class HistoryControlPageItem;
    class MessagesScrollAreaLayout;

    enum ScrollDerection
    {
        UP = 0,
        DOWN,
    };

    class MessagesScrollArea : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void fetchRequestedEvent(bool _isMoveToBottomIfNeed = true);

        void needCleanup(const QVector<Logic::MessageKey>& keysToUnload);

        void scrollMovedToBottom();

        void messagesSelected();

        void messagesDeselected();

        void updateHistoryPosition(int32_t position, int32_t offset);

        void buttonDownClicked();

        void recreateAvatarRect();

        void itemRead(const qint64 _id, const bool _visible);

    public:
        typedef std::function<bool(Ui::MessageItem*, const bool)> MessageItemVisitor;

        typedef std::function<bool(QWidget*, const bool)> WidgetVisitor;

        typedef std::pair<Logic::MessageKey, QWidget*> PositionWidget;

        typedef std::list<PositionWidget> WidgetsList;

        MessagesScrollArea(QWidget *parent, QWidget *typingWidget);

        void cancelSelection();

        void cancelWheelBufferReset();

        void enumerateMessagesItems(const MessageItemVisitor& visitor, const bool reversed) const;

        void enumerateWidgets(const WidgetVisitor& visitor, const bool reversed) const;

        QWidget* getItemByPos(const int32_t pos) const;

        QWidget* getItemByKey(const Logic::MessageKey &key) const;

        int32_t getItemsCount() const;

        Logic::MessageKeyVector getItemsKeys() const;

        QString getSelectedText() const;

        QVector<Data::Quote> getQuotes() const;

        QVector<Logic::MessageKey> getKeysToUnload() const;

        void insertWidget(const Logic::MessageKey &key, QWidget *widget);

        void insertWidgets(const WidgetsList& _widgets, bool _isMoveToButtonIfNeed, int64_t _mess_id, int64_t _countAfter, Logic::scroll_mode_type _scrollMode);

        bool isScrolling() const;

        bool isSelecting() const;

        bool isViewportFull() const;

        bool containsWidget(QWidget *widget) const;

        void removeWidget(QWidget *widget);

        void replaceWidget(const Logic::MessageKey &key, QWidget *widget);

        bool touchScrollInProgress() const;

        void scrollToBottom();

        void updateItemKey(const Logic::MessageKey &key);
        void scrollTo(const Logic::MessageKey &key, Logic::scroll_mode_type _scrollMode);

        void updateScrollbar();

        bool isScrollAtBottom() const;

        void clearSelection();

        void scroll(ScrollDerection direction, int delta);

        bool contains(const QString& _aimId) const;

        void resumeVisibleItems();

        void suspendVisibleItems();

        void setIsSearch(bool _isSearch);
        bool getIsSearch() const;

        void setMessageId(qint64 _id);
        qint64 getMessageId() const;

        void setRecvLastMessage(bool _value);

        void updateItems();

        void enableViewportShifting(bool enable);

    public slots:
        void notifySelectionChanges();

    protected:
        virtual void mouseMoveEvent(QMouseEvent *e) override;

        virtual void mousePressEvent(QMouseEvent *e) override;

        virtual void mouseReleaseEvent(QMouseEvent *e) override;

        virtual void wheelEvent(QWheelEvent *e) override;
        virtual void hideEvent(QHideEvent*) override;
        virtual bool event(QEvent *e) override;

    private Q_SLOTS:
        void onAnimationTimer();

        void onMessageHeightChanged(QSize, QSize);

        void onSliderMoved(int value);

        void onSliderPressed();

        void onSliderValue(int value);

        void onIdleUserActivityTimeout();

        void onWheelEventResetTimer();

		/// resend from MessagesScrollLayout to HistoryControlPage
		void onUpdateHistoryPosition(int32_t position, int32_t offset);
        void onTimestampTimer();

    public Q_SLOTS:
        void onWheelEvent(QWheelEvent* e);

    private:
        enum class ScrollingMode;

        bool IsSelecting_;

        bool TouchScrollInProgress_;

        int64_t LastAnimationMoment_;

        QPoint LastMouseGlobalPos_;

        QPoint SelectionBeginAbsPos_;

        QPoint SelectionEndAbsPos_;

        ScrollingMode Mode_;

        MessagesScrollbar *Scrollbar_;

        MessagesScrollAreaLayout *Layout_;

        QTimer ScrollAnimationTimer_;
        QTimer timestampTimer_;

        double ScrollDistance_;

        bool Resizing_;

        QPointF PrevTouchPoint_;

        bool IsUserActive_;

        QTimer UserActivityTimer_;

        std::deque<int32_t> WheelEventsBuffer_;

        QTimer WheelEventsBufferResetTimer_;

        void updateMode();

        void applySelection(const bool forShift = false);

        bool enqueWheelEvent(const int32_t delta);

        double evaluateScrollingVelocity() const;

        double evaluateScrollingStep(const int64_t now) const;

        int32_t evaluateWheelHistorySign() const;

        void scheduleWheelBufferReset();

        void startScrollAnimation(const ScrollingMode mode);

        void stopScrollAnimation();

        void resetUserActivityTimer();
        void resetTimestampTimer();

        void eraseContact(QWidget* widget);

        std::set<QString> contacts_;

        bool IsSearching_;

        int scrollValue_;

        qint64 messageId_;
        bool recvLastMessage_;
    };

}
