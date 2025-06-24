#pragma once

#include <set>
#include <cstdint>
#include <memory>
#include "Client.hpp"

class Client;

class Channel {
private:
    std::string _name;
    std::string _topic;
    std::string _topicSetBy;
    time_t _topicSetAt;
    std::set<std::shared_ptr<Client>> _members;
    std::set<std::shared_ptr<Client>> _operators;
    std::string _key;
    std::set<char> _channelModes;
    bool _isPrivate;
    uint64_t _maxMembers;
    bool _hasMemberLimit;
    bool _isInviteOnly;
    bool _operatorSetsTopic;
    
    void setName(const std::string& name);

public:
    Channel(const std::string& name, const std::string& key, const std::string& topic, bool isPrivate, bool isInviteOnly);
    ~Channel();

    // Getters
    const std::string getName() const;
    const std::string getKey() const;
    const std::string getTopic() const;
	const std::set<std::shared_ptr<Client>>& getMembers() const;
    const std::set<std::shared_ptr<Client>> getOperators() const;
    const bool& getIsPrivate() const;
    const bool& getIsInviteOnly() const;
    std::string getModes() const;
    bool getTopicFlag() const;
    std::string getSetter() const;
    time_t getTopicTime() const;
    uint64_t getNumberMaxMembers() const;
    bool getMaxMembers() const;
    uint64_t getNbMembers() const;

    // Setters
    void setTopic(const std::string& newTopic, const std::string& setter);
    void setKey(const std::string& key);
    void setPrivate(bool isPrivate);
    void setInviteOnly(bool isInviteOnly);
    void setModes(char mode);
    void setTopicFlag(bool operatorSetsTopic);
    void setMaxMembers(bool active);

    // Membership management
    void addMember(std::shared_ptr<Client> client);
    bool removeMember(std::shared_ptr<Client> client);
    bool addOperator(std::shared_ptr<Client> channelOperator, std::shared_ptr<Client> target);
    bool removeOperator(std::shared_ptr<Client> channelOperator, std::shared_ptr<Client> target);
    bool changeKey(std::shared_ptr<Client> channelOperator, std::string newKey);
    void limitMaxMembers(uint64_t limit);

    // Utility
    bool isMember(std::shared_ptr<Client> client) const;
    bool isOperator(std::shared_ptr<Client> client) const;
    bool isChannelEmpty() const;
    bool noOperators() const;
    bool hasMode(char mode) const;
    void removeMode(char mode);
    std::shared_ptr<Client> retrieveClient(std::string username);
};