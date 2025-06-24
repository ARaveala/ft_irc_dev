#ifndef __REPLY_H__
#define __REPLY_H__

#define CRLF "\r\n"

#define RPL_NICK(nickname) ("NICK " + nickname + CRLF)
#define RPL_USER(username) ("USER " + username + " 0 * :" + username + CRLF)
#define KICK_REQUEST(channel, target_nickname) ("KICK " + channel + " " + target_nickname + CRLF)
#define RPL_PRIVMSG(target, message) ("PRIVMSG " + target + " :" + message + CRLF)
#define RPL_PASS(password) ("PASS " + password + CRLF)

#endif