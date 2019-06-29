struct User {
    std::string name;

    Mailbox mailbox;
};

struct Message {
    User* sender;
    User* recipient;

    bool has_sender_deleted = false;
    bool has_recipient_deleted = false;

    size_t length;
    char content[];
};

struct Mailbox {
    User* user;
    std::vector<Message*> messages;
    int selected_message_index = -1;

    void AddMessage(Message* msg) {
        messages.push_back(msg);
    }

    void DeleteSelected() {
        Message* msg = messages[selected_message_index];

        messages.erase(selected_message_index);
        selected_message_index = -1;

        if (user == msg->sender) {
            msg->has_sender_deleted = true;
        } else {
            msg->has_recipient_deleted = true;
        }

        if (msg->has_sender_deleted && msg->has_recipient_deleted)
            delete msg;
    }

    // then later, somebody adds:
    void ForwardSelectedTo(User* recipient) {
        recipient->mailbox.AddMessage(messages[selected_message_index]);
    }
};
