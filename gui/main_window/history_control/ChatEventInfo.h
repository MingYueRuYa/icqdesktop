#pragma once

namespace core
{
    class coll_helper;

    struct iarray;

    enum class chat_event_type;
}

namespace HistoryControl
{

    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

    class ChatEventInfo
    {
    public:
        static ChatEventInfoSptr Make(
            const core::coll_helper& _info,
            const bool _isOutgoing,
            const QString& _myAimid);

        const QString& formatEventText() const;

        QImage loadEventIcon(const int32_t _sizePx) const;

        core::chat_event_type eventType() const;

        const QString& getSenderFriendly() const;

        bool isOutgoing() const;

    private:
        const core::chat_event_type Type_;

        const bool IsOutgoing_;

        const QString MyAimid_;

        mutable QString FormattedEventText_;

        QString SenderAimid_;

        QString SenderFriendly_;

        QString Generic_;

        struct
        {
            QStringList MembersFriendly_;
        } Mchat_;

        struct
        {
            QString NewName_;
            QString NewDescription_;
            QString NewRules_;
        } Chat_;

        ChatEventInfo(const core::chat_event_type _type, const bool _isOutgoing, const QString& _myAimid);

        QString formatEventTextInternal() const;

        QString formatAddedToBuddyListText() const;

        QString formatAvatarModifiedText() const;

        QString formatBirthdayText() const;

        QString formatBuddyFound() const;

        QString formatBuddyReg() const;

        QString formatChatNameModifiedText() const;

        QString formatGenericText() const;

        QString formatMchatAddMembersText() const;

        QString formatMchatDelMembersText() const;

        QString formatChatDescriptionModified() const;

        QString formatChatRulesModified() const;

        QString formatMchatInviteText() const;

        QString formatMchatKickedText() const;

        QString formatMchatLeaveText() const;

        QString formatMchatMembersList(const bool _activeVoice) const;

        QString formatMessageDeletedText() const;

        bool isMyAimid(const QString& _aimId) const;

        bool hasMultipleMembers() const;

        void setGenericText(const QString& _text);

        void setNewChatRules(const QString& _newChatRules);

        void setNewDescription(const QString& _newDescription);

        void setNewName(const QString& _newName);

        void setSenderInfo(const QString& _aimid, const QString& _friendly);

        void setMchatMembers(const core::iarray& _members);

    };

}