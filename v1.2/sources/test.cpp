
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

// --- Refactored broadcastMessage Function that is explicitly not too big or complicated ---
/**
 * @brief Broadcasts a message to one or more clients, based on the provided parameters.
 *
 * This function is designed to handle various message broadcasting scenarios:
 * - Sending to a single, specific client.
 * - Sending to all clients within a specified channel.
 * - Sending to all clients in channels shared with a given sender.
 *
 * The function ensures that empty messages are skipped and handles cases where
 * required parameters are missing. It queues the message for each recipient
 * and updates their epoll events to ensure the message is sent.
 *
 * @param message_content The actual message string to be broadcast. Must not be empty.
 * @param sender A shared pointer to the client who is sending the message. This is used
 * to determine shared channels or to skip the sender in a channel broadcast.
 * Can be nullptr if `individual_recipient` is provided and no sender context is needed.
 * @param target_channel A shared pointer to the channel to which the message should be broadcast.
 * Mutually exclusive with `individual_recipient`.
 * @param skip_sender If true, the `sender` will be excluded from the list of recipients
 * when broadcasting to a channel or shared channels. Defaults to false.
 * @param individual_recipient An optional shared pointer to a specific client who should receive the message.
 * If provided, the message is sent only to this client. Mutually exclusive with `target_channel`.
 *
 * @attention
 * - If `message_content` is empty, a warning is logged and the function returns immediately.
 * - One of `individual_recipient`, `target_channel`, or `sender` must be provided to determine recipients.
 * If none are provided, an error is logged.
 * - In the case of `sender` being provided without `target_channel` or `individual_recipient`,
 * the message is broadcast to all clients in channels *shared* with the sender. If the sender
 * is alone in all their channels and `skip_sender` is false, the sender will still receive the message.
 */
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
        if (recipients.empty() && !skip_sender) {
            recipients.insert(sender);  // Ensure sender receives the message even if alone
        }
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
            updateEpollEvents(recipientClient->getFd(), EPOLLOUT, true);
        }
        LOG_DEBUG(" SERVER:: Message queued for FD " + std::to_string(recipientClient->getFd()) + " (" + recipientClient->getNickname() + ")");
        //std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << ")\n";
    }
}