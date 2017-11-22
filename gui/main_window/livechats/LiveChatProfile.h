#pragma once

namespace Data
{
    class ChatInfo;
}

namespace core
{
    enum class group_chat_info_errors;
}

namespace Ui
{
    class ContactAvatarWidget;
    class LiveChatMembersControl;
    class GeneralDialog;
    class TextEditEx;

    class LiveChats : public QObject
    {
        Q_OBJECT

        QWidget* parent_;
        bool connected_;
        GeneralDialog* activeDialog_;
        QString joinedLiveChat_;

    private Q_SLOTS:

        void needJoinLiveChat(const QString& _stamp);
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>& _info);
        void chatInfoFailed(qint64, core::group_chat_info_errors _error);
        void liveChatJoined(const QString& _aimid);

    private:

        qint64 seq_;

    public:

        explicit LiveChats(QWidget* _parent);
        virtual ~LiveChats();
    };

    class LiveChatProfileWidget : public QWidget
    {
        Q_OBJECT

        QString stamp_;
        QVBoxLayout* rootLayout_;
        ContactAvatarWidget* avatar_;
        LiveChatMembersControl* members_;
        TextEditEx* name_;
        int membersCount_;
        int initialiNameHeight_;

        void requestProfile();

    private Q_SLOTS:
        void nameResized(int, int);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        void viewChat(std::shared_ptr<Data::ChatInfo> _info);
        void setStamp(const QString& _stamp);

        LiveChatProfileWidget(QWidget* _parent, const QString& _stamp = QString());
        virtual ~LiveChatProfileWidget();
    };

    class LiveChatErrorWidget : public QWidget
    {
        Q_OBJECT

    private:

        const QString errorText_;

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        LiveChatErrorWidget(QWidget* _parent, const QString& _errorText);
        virtual ~LiveChatErrorWidget();

        void show();
    };
}
