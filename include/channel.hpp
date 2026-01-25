#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class User;  // Forward declaration
enum JoinResult { JOIN_OK, JOIN_INVITE_ONLY, JOIN_BADKEY, JOIN_FULL };

class Channel {
private:
    std::string _name;              // Channel name in lowercase (e.g., "#general")
    std::string _topic;             // Channel topic (optional)
    std::string _topic_setter;      // Nickname of the user who set the topic
    int _topic_set_at;              // Timestamp when the topic was set

    // Members and operators
    std::set<User*> _members;
    std::set<User*> _operators;

    // Mode flags and variables
    size_t _user_limit;       // +l
    bool _invite_only;      // +i
    bool _topic_protection; // +t
    std::string _key;      // +k
    std::set<std::string> _invitation_list; // +i

public:
    Channel(const std::string& name);
    ~Channel();
    
    // General Getters
    std::string getName() const;
    size_t getMemberCount() const;

    // Topic management
    std::string getTopic() const;
    std::string getTopicSetter() const;
    int getTopicSetAt() const;
    void setTopic(const std::string& topic);
    void setTopicSetter(const std::string& setter);
    void setTopicSetAt(int timestamp);
    
    // Mode getters/setters (for MODE command)
    bool isInviteOnly() const;
    bool isTopicProtected() const;
    std::string getKey() const;
    size_t getUserLimit() const;
    void setInviteOnly(bool value);
    void setTopicProtected(bool value);
    void setKey(const std::string& key);
    void setUserLimit(size_t limit);
    void addInvite(const std::string& nickname);
    void removeInvite(const std::string& nickname);

    // Member management
    bool addMember(User* user);
    bool removeMember(User* user);
    bool isMember(User* user) const;
    JoinResult canJoin(User* user, const std::string& key = "") const;
    
    // Operator management
    bool addOperator(User* user);
    bool removeOperator(User* user);
    bool isOperator(User* user) const;
    
    // Get list of members for NAMES reply
    // Returns string like "@john alice bob" (@ prefix for ops)
    std::string getNamesList() const;
    
    // Broadcast message to all members (optionally excluding one user)
    void broadcast(const std::string& message, User* exclude = NULL) const;
};

#endif