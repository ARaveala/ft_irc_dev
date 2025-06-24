#include <iostream>
#include <vector>

#include "User.hpp"
#include "Channel.hpp"
#include "ChannelManager.hpp"

int main() {
    // Create a ChannelManager instance
    ChannelManager channelManager;

    // Create some dummy users
    User* user1 = new User("Warlock");
    User* user2 = new User("Woof");
    User* user3 = new User("Meow");

    // Create channels using the ChannelManager
    Channel* generalChannel = channelManager.createChannel("#general");
    Channel* devChannel = channelManager.createChannel("#development");
    channelManager.createChannel("#general"); // Try creating again (should fail)

    // Get channels
    Channel* foundGeneral = channelManager.getChannel("#general");
    Channel* nonExistent = channelManager.getChannel("#random");

    if (foundGeneral) {
        std::cout << "\n--- Operations on #general ---" << std::endl;
        std::cout << "Found channel: " << foundGeneral->getName() << std::endl;

        // Add users to channels
        foundGeneral->addUser(user1);
        foundGeneral->addUser(user2);
        foundGeneral->addUser(user1); // Try adding again (should do nothing)

        if (devChannel) {
             devChannel->addUser(user2);
             devChannel->addUser(user3);
        }

        // List users in a channel
        std::cout << "Users in " << foundGeneral->getName() << ":" << std::endl;
        for (User* user : foundGeneral->getUsers()) {
            std::cout << "- " << user->getNickname() << std::endl;
        }

        // Check if user is in channel
        std::cout << "Is Warlock in #general? " << (foundGeneral->isUserInChannel(user1) ? "Yes" : "No") << std::endl;
        std::cout << "Is Meow in #general? " << (foundGeneral->isUserInChannel(user3) ? "Yes" : "No") << std::endl;

        // Set and get topic
        foundGeneral->setTopic("Discussing important things");
        std::cout << "Topic of " << foundGeneral->getName() << ": " << foundGeneral->getTopic() << std::endl;

        // Broadcast a message
        foundGeneral->broadcastMessage("Hello everyone!", user1);

        // Channel operators
        foundGeneral->addOperator(user1); // Warlock becomes operator
        std::cout << "Is Warlock an operator in #general? " << (foundGeneral->isOperator(user1) ? "Yes" : "No") << std::endl;
        std::cout << "Is Woof an operator in #general? " << (foundGeneral->isOperator(user2) ? "Yes" : "No") << std::endl;

        foundGeneral->removeOperator(user1); // Warlock is no longer operator
         std::cout << "Is Warlock an operator in #general after removal? " << (foundGeneral->isOperator(user1) ? "Yes" : "No") << std::endl;

        // Example of removing a user
        foundGeneral->removeUser(user2); // Woof leaves the channel
        std::cout << "Users in " << foundGeneral->getName() << " after Woof left:" << std::endl;
         for (User* user : foundGeneral->getUsers()) {
            std::cout << "- " << user->getNickname() << std::endl;
        }

        // Example of setting/removing modes
        foundGeneral->setMode("+t", user1);
        foundGeneral->removeMode("-t", user1);

    } else {
        std::cout << "Channel #general not found." << std::endl;
    }

    std::cout << "\n--- Channel Management ---" << std::endl;
    // List all channels
    std::cout << "All channels:" << std::endl;
    for (const auto& name : channelManager.listChannels()) {
        std::cout << "- " << name << std::endl;
    }

    // Delete a channel
    channelManager.deleteChannel("#development");
    channelManager.deleteChannel("#random"); // Try deleting non-existent channel

     // List all channels after deletion
    std::cout << "All channels after deletion:" << std::endl;
    for (const auto& name : channelManager.listChannels()) {
        std::cout << "- " << name << std::endl;
    }

    std::cout << "\n--- Cleanup ---" << std::endl;
    // Clean up dummy users (in a real server, this would happen when clients disconnect)
    // The ChannelManager destructor will automatically delete the remaining Channel objects.
    delete user1;
    delete user2;
    delete user3;
    std::cout << "Dummy users cleaned up." << std::endl;


    return 0;
}
