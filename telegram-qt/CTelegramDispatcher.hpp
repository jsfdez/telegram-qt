/*
    Copyright (C) 2014-2015 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef CTELEGRAMDISPATCHER_HPP
#define CTELEGRAMDISPATCHER_HPP

#include <QObject>

#include <QMap>
#include <QMultiMap>
#include <QPair>
#include <QStringList>
#include <QVector>

#include "TLTypes.hpp"
#include "TelegramNamespace.hpp"

class QTimer;

class CAppInformation;
class CTelegramConnection;

class FileRequestDescriptor
{
public:
    enum Type {
        Invalid,
        Avatar,
        MessageMediaData
    };

    FileRequestDescriptor();

    static FileRequestDescriptor avatarRequest(const TLUser *user);
    static FileRequestDescriptor messageMediaDataRequest(const TLMessage &message);

    inline Type type() const { return m_type; }

    inline quint32 dcId() const { return m_dcId; }
    inline bool isValid() const { return m_type != Invalid; }

    inline TLInputFileLocation inputLocation() const { return m_inputLocation; }

    inline quint32 userId() const { return m_userId; }
    inline quint32 messageId() const { return m_messageId; }

protected:
    void setupLocation(const TLFileLocation &fileLocation);
    Type m_type;
    quint32 m_userId;
    quint32 m_messageId;

    TLInputFileLocation m_inputLocation;
    quint32 m_dcId;

};

class CTelegramDispatcher : public QObject
{
    Q_OBJECT
public:
    enum InitializationStep {
        StepFirst           = 0,
        StepDcConfiguration = 1 << 0,
        StepSignIn          = 1 << 1,
        StepKnowSelf        = 1 << 2,
        StepContactList     = 1 << 3,
        StepChatInfo        = 1 << 4,
        StepUpdates         = 1 << 5,
        StepDone            = StepUpdates | (StepUpdates - 1)
    };

    explicit CTelegramDispatcher(QObject *parent = 0);
    ~CTelegramDispatcher();

    void setAppInformation(const CAppInformation *newAppInfo);

    static qint32 localTypingRecommendedRepeatInterval();

    inline TelegramNamespace::ConnectionState connectionState() const { return m_connectionState; }

    QString selfPhone() const;

    QStringList contactList() const;
    QList<quint32> chatList() const;

    void addContacts(const QStringList &phoneNumbers, bool replace = false);
    void deleteContacts(const QStringList &phoneNumbers);

    QByteArray connectionSecretInfo() const;

    inline quint32 messageReceivingFilterFlags() const { return m_messageReceivingFilterFlags; }
    void setMessageReceivingFilterFlags(quint32 flags);
    void setAcceptableMessageTypes(quint32 types);
    void setAutoReconnection(bool enable);
    void setPingInterval(quint32 ms);

    void initConnection(const QString &address, quint32 port);
    bool restoreConnection(const QByteArray &secret);
    void closeConnection();
    bool logOut();

    void requestPhoneStatus(const QString &phoneNumber);
    void signIn(const QString &phoneNumber, const QString &authCode);
    void signUp(const QString &phoneNumber, const QString &authCode, const QString &firstName, const QString &lastName);

    void requestPhoneCode(const QString &phoneNumber);
    void requestContactAvatar(const QString &contact);
    bool requestMessageMediaData(quint32 messageId);

    quint64 sendMessage(const QString &identifier, const QString &message);
    void setTyping(const QString &identifier, bool typingStatus);
    void setMessageRead(const QString &identifier, quint32 messageId);

    quint32 createChat(const QStringList &phones, const QString chatName);
    bool addChatUser(quint32 publicChatId, const QString &contact, quint32 forwardMessages);

    void setOnlineStatus(bool onlineStatus);
    void checkUserName(const QString &userName);
    void setUserName(const QString &newUserName);

    TelegramNamespace::ContactStatus contactStatus(const QString &phone) const;
    quint32 contactLastOnline(const QString &contact) const;

    QString contactFirstName(const QString &contact) const;
    QString contactLastName(const QString &contact) const;
    QString contactUserName(const QString &contact) const;
    QString contactAvatarToken(const QString &contact) const;

    QString chatTitle(quint32 publicChatId) const;

    bool getChatInfo(TelegramNamespace::GroupChat *outputChat, quint32 publicChatId) const;
    bool getChatParticipants(QStringList *participants, quint32 publicChatId);

signals:
    void connectionStateChanged(TelegramNamespace::ConnectionState status);

    void phoneCodeRequired();
    void authSignErrorReceived(TelegramNamespace::AuthSignError errorCode, const QString &errorMessage);
    void loggedOut(bool result);
    void authorizationErrorReceived();
    void userNameStatusUpdated(const QString &userName, TelegramNamespace::AccountUserNameStatus status);
    void contactListChanged();
    void contactProfileChanged(const QString &contact);
    void phoneStatusReceived(const QString &phone, bool registered, bool invited);

    void avatarReceived(const QString &contact, const QByteArray &data, const QString &mimeType, const QString &avatarToken);
    void messageMediaDataReceived(const QString &contact, quint32 messageId, const QByteArray &data, const QString &mimeType, TelegramNamespace::MessageType type);

    void messageReceived(const QString &contact, const QString &message, TelegramNamespace::MessageType type, quint32 messageId, quint32 flags, quint32 timestamp);
    void chatMessageReceived(quint32 chatId, const QString &contact, const QString &message, TelegramNamespace::MessageType type, quint32 messageId, quint32 flags, quint32 timestamp);

    void contactStatusChanged(const QString &phone, TelegramNamespace::ContactStatus status);
    void contactTypingStatusChanged(const QString &phone, bool typingStatus);
    void contactChatTypingStatusChanged(quint32 publicChatId, const QString &phone, bool typingStatus);

    void sentMessageStatusChanged(const QString &phone, quint64 randomMessageId, TelegramNamespace::MessageDeliveryStatus status);

    void chatAdded(quint32 publichChatId);
    void chatChanged(quint32 publichChatId);

protected slots:
    void whenConnectionAuthChanged(int newState, quint32 dc);
    void whenConnectionStatusChanged(int newStatus, quint32 dc);
    void whenDcConfigurationUpdated(quint32 dc);
    void whenConnectionDcIdUpdated(quint32 connectionId, quint32 newDcId);
    void whenPackageRedirected(const QByteArray &data, quint32 dc);
    void whenWantedActiveDcChanged(quint32 dc);

    void whenFileReceived(const TLUploadFile &file, quint32 fileId);
    void whenUpdatesReceived(const TLUpdates &updates);
    void whenAuthExportedAuthorizationReceived(quint32 dc, quint32 id, const QByteArray &data);

    void whenUsersReceived(const QVector<TLUser> &users);

    void whenContactListReceived(const QList<quint32> &contactList);
    void whenContactListChanged(const QList<quint32> &added, const QList<quint32> &removed);
    void whenUserTypingTimerTimeout();

    void whenStatedMessageReceived(const TLMessagesStatedMessage &statedMessage, quint64 messageId);
    void whenMessageSentInfoReceived(const TLInputPeer &peer, quint64 randomId, quint32 messageId, quint32 pts, quint32 date, quint32 seq);

    void getDcConfiguration();
    void getContacts();
    void getChatsInfo();
    void getUpdatesState();
    void whenUpdatesStateReceived(const TLUpdatesState &updatesState);

    void getDifference();
    void whenUpdatesDifferenceReceived(const TLUpdatesDifference &updatesDifference);

    void whenMessagesChatsReceived(const QVector<TLChat> &chats, const QVector<TLUser> &users);
    void whenMessagesFullChatReceived(const TLChatFull &chat, const QVector<TLChat> &chats, const QVector<TLUser> &users);

protected:
    void setConnectionState(TelegramNamespace::ConnectionState state);

    bool requestFile(const FileRequestDescriptor &requestId);
    void processUpdate(const TLUpdate &update);

    void processMessageReceived(const TLMessage &message);

    void updateChat(const TLChat &newChat);
    void updateFullChat(const TLChatFull &newChat);

    void initConnectionSharedFinal(quint32 activeDc = 0);

    void getUser(quint32 id);
    void getInitialUsers();

    quint64 sendMessages(const TLInputPeer &peer, const QString &message);

    bool filterReceivedMessage(quint32 messageFlags) const;

    quint32 publicChatIdToChatId(quint32 publicChatId) const;
    TLInputPeer publicChatIdToInputPeer(quint32 publicChatId) const;
    TLInputPeer identifierToInputPeer(const QString &identifier) const;
    TLInputUser phoneNumberToInputUser(const QString &phoneNumber) const;
    QString userIdToIdentifier(const quint32 id) const;
    QString chatIdToIdentifier(const quint32 id) const;
    quint32 identifierToUserId(const QString &identifier) const;
    TLUser *identifierToUser(const QString &identifier) const;

    QString userAvatarToken(const TLUser *user) const;

    TelegramNamespace::ContactStatus decodeContactStatus(TLValue status) const;

    CTelegramConnection *activeConnection() const { return m_connections.value(m_activeDc); }

    CTelegramConnection *createConnection(const TLDcOption &dc);
    CTelegramConnection *establishConnectionToDc(quint32 dc);
    void ensureSignedConnection(quint32 dc);

    TLDcOption dcInfoById(quint32 dc) const;

    QString mimeTypeByStorageFileType(TLValue type);
    TelegramNamespace::MessageType telegramMessageTypeToPublicMessageType(TLValue type);

    void setActiveDc(quint32 dc, bool syncWantedDc = true);

    void ensureTypingUpdateTimer(int interval);
    void ensureUpdateState(quint32 pts = 0, quint32 seq = 0, quint32 date = 0);

    void checkStateAndCallGetDifference();

    void continueInitialization(InitializationStep justDone);

    qint32 telegramChatIdToPublicId(quint32 telegramChatId) const;
    quint32 insertTelegramChatId(quint32 telegramChatId);
    bool havePublicChatId(quint32 publicChatId) const;

    quint32 telegramMessageFlagsToPublicMessageFlags(quint32 tgFlags);

    TelegramNamespace::ConnectionState m_connectionState;

    const CAppInformation *m_appInformation;

    quint32 m_messageReceivingFilterFlags;
    quint32 m_acceptableMessageTypes;
    bool m_autoReconnectionEnabled;
    quint32 m_pingInterval;

    quint32 m_initializationState; // InitializationStep flags
    quint32 m_requestedSteps; // InitializationStep flags

    quint32 m_activeDc;
    quint32 m_wantedActiveDc;

    QVector<TLDcOption> m_dcConfiguration;
    QMap<int, CTelegramConnection *> m_connections;

    TLUpdatesState m_updatesState; // Current application update state (may be older than actual server-side message box state)
    TLUpdatesState m_actualState; // State reported by server as actual
    bool m_updatesStateIsLocked; // True if we are (going to) getting updatesDifference.
    bool m_emitOnlyIncomingUnreadMessages;

    QMap<quint32, QPair<quint32,QByteArray> > m_exportedAuthentications; // dc, <id, auth data>
    QMap<quint32, QByteArray> m_delayedPackages; // dc, package data
    QMap<quint32, TLUser*> m_users;

    QMap<quint32, QPair<QString, quint64> >m_messagesMap; // message id to phone and big_random message id

    QMap<quint32, TLMessage> m_knownMediaMessages; // message id, message

    quint32 m_selfUserId;

    QList<quint32> m_contactList;

    // fileId is program-specific handler, not related to Telegram.
    QMap<quint32, FileRequestDescriptor> m_requestedFileDescriptors; // fileId, file request descriptor
    quint32 m_fileRequestCounter;

    QTimer *m_typingUpdateTimer;
    QMap<quint32, int> m_userTypingMap; // user id, typing time (ms)
    QMap<QPair<quint32, quint32>, int> m_userChatTypingMap; // <public chat id, user id>, typing time (ms)
    QMap<QString, int> m_localTypingMap; // phone, typing time (ms)

    TLVector<quint32> m_chatIds; // Telegram chat ids vector. Index is "public chat id".
    QMap<quint64, quint32> m_temporaryChatIdMap; // RPC message (request) id to public chat id map

    QMap<quint32, TLChat> m_chatInfo; // Telegram chat id to Chat map
    QMap<quint32, TLChatFull> m_chatFullInfo; // Telegram chat id to ChatFull map

};

#endif // CTELEGRAMDISPATCHER_HPP
