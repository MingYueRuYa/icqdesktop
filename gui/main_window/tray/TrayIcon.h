#pragma once

#include "../../types/message.h"

class QSystemTrayIcon;

namespace Logic
{
    class RecentItemDelegate;
}

namespace Ui
{
#ifdef _WIN32
//    class ToastManager;
#else
    class NotificationCenterManager;
#endif //_WIN32
    class RecentMessagesAlert;
    class MainWindow;
    class ContextMenu;

    enum class AlertType;

    enum TrayPosition
    {
        TOP_LEFT = 0,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTOOM_RIGHT,
    };

    class TrayIcon : public QObject
    {
        Q_OBJECT

    public:
        explicit TrayIcon(MainWindow* parent);
        ~TrayIcon();

        void Hide();
        void forceUpdateIcon();

        void setVisible(bool visible);
        void updateEmailIcon();

    private Q_SLOTS:
        void dlgStates(const QVector<Data::DlgState>&);
        void newMail(const QString&, const QString&, const QString&, const QString&);
        void mailStatus(const QString&, unsigned, bool);
        void messageClicked(const QString& _aimId, const QString& mailId, const qint64 mentionId, const AlertType _alertType);
        void clearNotifications(const QString&);
        void activated(QSystemTrayIcon::ActivationReason);
        void loggedIn();
        void loggedOut(const bool _is_auth_error);
        void myInfo();
        void updateAlertsPosition();
        void onEmailIconClick(QSystemTrayIcon::ActivationReason);
        void mailBoxOpened();
        void logout();
        void mentionMe(const QString& _contact, Data::MessageBuddySptr _mention);

    private:
        void init();
        void initEMailIcon();
        void showEmailIcon();
        void hideEmailIcon();

        bool canShowNotifications(bool isMail) const;
        void openMailBox(const QString& _mailId);
#ifdef _WIN32
        bool toastSupported();
#else
        bool ncSupported();
#endif //_WIN32
        void showMessage(const Data::DlgState& state, const AlertType _alertType);
        TrayPosition getTrayPosition() const;
        void markShowed(const QString&);
        bool canShowNotificationsWin() const;

        QIcon createIcon(const bool _withBase = false);
        void updateIcon(const bool _force = false, const bool _updateOverlay = true);
        void setMacIcon();
        void cleanupOverlayIcon();

    private:

        QSystemTrayIcon* systemTrayIcon_;
        QSystemTrayIcon* emailSystemTrayIcon_;

        ContextMenu* Menu_;

        RecentMessagesAlert* MessageAlert_;
        RecentMessagesAlert* MentionAlert_;
        RecentMessagesAlert* MailAlert_;

        QHash<QString, qint64> ShowedMessages_;
        QStringList Notifications_;
        MainWindow* MainWindow_;

        bool first_start_;

        QIcon Base_;
        QIcon Unreads_;
        QIcon TrayBase_;
        QIcon TrayUnreads_;
        QIcon emailIcon_;

        QString Email_;
        QTimer* InitMailStatusTimer_;
        int UnreadsCount_;
        int MailCount_;

#if defined (_WIN32)
//        std::unique_ptr<ToastManager> ToastManager_;
        ITaskbarList3 *ptbl;
        HICON overlayIcon_;
#elif defined(__APPLE__)
        std::unique_ptr<NotificationCenterManager> NotificationCenterManager_;
#endif //_WIN32
    };
}
