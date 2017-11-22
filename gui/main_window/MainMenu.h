#pragma once

namespace Ui
{
    class ActionButton;
    class CustomButton;
    class ContactAvatarWidget;
    class TextEditEx;
    class CustomButton;
    class LineWidget;
    class LabelEx;

    class BackWidget : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:
        void clicked();
        void resized();

    public:
        BackWidget(QWidget* _parent);

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *e);
        virtual void resizeEvent(QResizeEvent *e);
        virtual void paintEvent(QPaintEvent *e);
    };

    class MainMenu: public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void createGroupChat();
        void addContact();
        void contacts();
        void settings();
        void myProfile();
        void about();
        void contactUs();

    public:
        MainMenu(QWidget* _parent);
        ~MainMenu();

    protected:
        virtual void resizeEvent(QResizeEvent *);
        virtual void showEvent(QShowEvent *);
        virtual void hideEvent(QHideEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private Q_SLOTS:
        void Hide();
        void myInfoUpdated();
        void soundsChecked(int);
        void guiSettingsChanged(QString);
        void signOut();
        void resize();

    private:
        void updateState();

    private:
        ActionButton* CreateGroupchat_;
        ActionButton* AddContact_;
        ActionButton* Contacts_;
        ActionButton* Settings_;
        ActionButton* SignOut_;

        BackWidget* Background_;
        QWidget* Parent_;
        CustomButton* Close_;
        ContactAvatarWidget* Avatar_;
        TextEditEx* Name_;
        TextEditEx* Status_;
        CustomButton* SoundsButton_;
        QCheckBox* SoundsCheckbox_;
        QScrollArea* ScrollArea_;
        QWidget* MainWidget_;
        LineWidget* Line_;
        LabelEx* About_;
        LabelEx* ContactUs_;
    };
}
