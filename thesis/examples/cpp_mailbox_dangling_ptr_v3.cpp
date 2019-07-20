#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

class Message {
public:
    const std::string mContent;

    bool mHasSenderDeleted = false;
    bool mHasRecipientDeleted = false;

    explicit Message(std::string content)
        :   mContent{std::move(content)}
    {}

    void OnDeleted() {
        if (mHasSenderDeleted && mHasRecipientDeleted) {
            delete this;
        }
    }
};

class Mailbox {
public:
    std::vector<Message*> mMessages;

    void AddMessage(Message* msg) {
        mMessages.push_back(msg);
    }

    void DeleteMessage(Message* msg) {
        mMessages.erase(std::find(mMessages.begin(), mMessages.end(), msg));

        msg->OnDeleted();
    }
};

class User {
public:
    const std::string mName;

    Mailbox mInbox;
    Mailbox mOutbox;

    explicit User(std::string name)
        :   mName{std::move(name)}
    {}

    Message* SendMessage(User& recipient, std::string content) {
        Message* msg = new Message{std::move(content)};

        mOutbox.AddMessage(msg);
        recipient.mInbox.AddMessage(msg);

        return msg;
    }

    void ForwardMessage(User& recipient, Message* msg) {
        // TODO: do this properly later
        recipient.mInbox.AddMessage(msg);
    }

    void DeleteReceivedMessage(Message* msg) {
        msg->mHasRecipientDeleted = true;
        mInbox.DeleteMessage(msg);
    }

    void DeleteSentMessage(Message* msg) {
        msg->mHasSenderDeleted = true;
        mOutbox.DeleteMessage(msg);
    }
};

int main() {
    User alice{"Alice"};
    User bob{"Bob"};
    User cecile{"Cecile"};
    User daniel{"Daniel"};

    Message* oopsMessage = bob.SendMessage(alice, "Hey, look at this funny gif: <image>");
    bob.DeleteSentMessage(oopsMessage);

    alice.SendMessage(cecile, "Haha, look at this funny gif!");
    alice.ForwardMessage(cecile, oopsMessage);

    alice.SendMessage(bob, "HAHA that's pretty awesome");
    alice.DeleteReceivedMessage(oopsMessage);

    bob.SendMessage(daniel, "My PIN code is 6666");

    std::cerr << "Alice's inbox:\n";
    for (const Message* msg : cecile.mInbox.mMessages) {
        std::cerr << msg->mContent << "\n";
    }

    return 0;
}
