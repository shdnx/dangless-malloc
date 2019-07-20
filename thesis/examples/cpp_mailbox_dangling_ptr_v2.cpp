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
            std::cerr << "Message destroyed: " << mContent << "\n";
            delete this;
        }
    }

    void Dump(std::ostream& os) const {
        os << mContent;
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

    void Dump(std::ostream& os) const {
        for (Message* msg : mMessages) {
            os << " - ";
            msg->Dump(os);
            os << "\n";
        }
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

        std::cerr << "Send [" << mName << " => " << recipient.mName << "] " << msg->mContent << "\n";

        mOutbox.AddMessage(msg);
        recipient.mInbox.AddMessage(msg);

        return msg;
    }

    void ForwardMessage(User& recipient, Message* msg) {
        std::cerr << "Forward [" << mName << " => " << recipient.mName << "] " << msg->mContent << "\n";

        // TODO: do this properly later
        recipient.mInbox.AddMessage(msg);
    }

    void DeleteReceivedMessage(Message* msg) {
        std::cerr << "[" << mName << "] recipient deleting message: " << msg->mContent << "\n";

        msg->mHasRecipientDeleted = true;
        mInbox.DeleteMessage(msg);
    }

    void DeleteSentMessage(Message* msg) {
        std::cerr << "[" << mName << "] sender deleting message: " << msg->mContent << "\n";

        msg->mHasSenderDeleted = true;
        mOutbox.DeleteMessage(msg);
    }

    void Dump(std::ostream& os) const {
        os << "\n{{ User " << mName << " }}\n";

        os << "\n[Inbox]\n";
        mInbox.Dump(os);

        os << "\n[Outbox]\n";
        mOutbox.Dump(os);
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

    alice.Dump(std::cerr);
    bob.Dump(std::cerr);
    cecile.Dump(std::cerr);
    daniel.Dump(std::cerr);

    return 0;
}
