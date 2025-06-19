
#include "Server.hpp"


std::set<std::shared_ptr<Client>> Server::getChannelRecipients(std::shared_ptr<Channel> channel, std::shared_ptr<Client> sender, bool skip_sender) {
    std::set<std::shared_ptr<Client>> recipients;
    if (!channel) {
        std::cerr << "Error: getChannelRecipients received null channel pointer.\n";
        return recipients;
    }
    for (const auto& pair : channel->getAllClients()) {
        if (auto current_client_ptr = pair.first.lock()) {
            if (sender && current_client_ptr->getFd() == sender->getFd() && skip_sender) {
                continue; // Skip the sender if requested
            }
            recipients.insert(current_client_ptr);
        } else {
            std::cerr << "WARNING: Expired weak_ptr to client found in channel "
                      << channel->getName() << " member list. Client likely disconnected.\n";
        }
    }
    return recipients;
}

std::set<std::shared_ptr<Client>> Server::getSharedChannelRecipients(std::shared_ptr<Client> sender, bool skip_sender) {
    std::set<std::shared_ptr<Client>> recipients;
    if (!sender) {
        std::cerr << "Error: getSharedChannelRecipients received null sender pointer.\n";
        return recipients;
    }

    std::map<std::string, std::weak_ptr<Channel>> client_joined_channels_copy = sender->getJoinedChannels();
    for (const auto& pair : client_joined_channels_copy) {
        if (std::shared_ptr<Channel> channel_sptr = pair.second.lock()) {
            std::set<std::shared_ptr<Client>> channel_members = getChannelRecipients(channel_sptr, sender, skip_sender);
            recipients.insert(channel_members.begin(), channel_members.end());
        } else {
            std::cerr << "WARNING: weak_ptr to channel '" << pair.first << "' expired in client's joined list.\n";
        }
    }
    return recipients;
}

// --- Refactored broadcastMessage Function ---
// todo doxygen this
void Server::broadcastMessage(const std::string& message_content, std::shared_ptr<Client> sender, std::shared_ptr<Channel> target_channel, // For single channel broadcasts
    bool skip_sender,
    std::shared_ptr<Client> individual_recipient // Optional: for sending to a single client
) {
    if (message_content.empty()) {
        std::cerr << "WARNING: Attempted to broadcast an empty message. Skipping.\n";
        return;
    }
    std::set<std::shared_ptr<Client>> recipients;
    if (individual_recipient) {
        recipients.insert(individual_recipient);
    } else if (target_channel) {
        recipients = getChannelRecipients(target_channel, sender, skip_sender);
    } else if (sender) {
        recipients = getSharedChannelRecipients(sender, skip_sender);
    } else {
        std::cerr << "Error: Invalid call to broadcastMessage. Must provide an individual recipient, a target channel, or a sender.\n";
        return;
    }

    // Common logic for queuing and updating epoll events
    for (const auto& recipientClient : recipients) {
        if (!recipientClient) { // Safety check for null shared_ptr (shouldn't happen with weak_ptr locking)
            std::cerr << "WARNING: Attempted to queue message for a null recipient client.\n";
            continue;
        }
        bool wasEmpty = recipientClient->isMsgEmpty();
        recipientClient->getMsg().queueMessage(message_content);
        if (wasEmpty) {
            std::cout << "it was empty yeah !!!!---------------\n"; // Keep your debug message if desired
            updateEpollEvents(recipientClient->getFd(), EPOLLOUT, true);
        }
        std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << ")\n";
    }
}
