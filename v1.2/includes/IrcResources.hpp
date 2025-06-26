#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower

// Enum for various IRC message types (numeric replies, commands, errors)
enum class MsgType {
	// IRC numeric replies and commands
	NONE = 0,
	RPL_WELCOME,            // 001
	RPL_YOURHOST,           // 002
	RPL_CREATED,            // 003
	RPL_MYINFO,             // 004
	RPL_BOUNCE,             // 005 (or ISUPPORT for server capabilities)
	RPL_LUSERCLIENT,        // 251
	RPL_LUSEROP,            // 252
	RPL_LUSERUNKNOWN,       // 253
	RPL_LUSERCHANNELS,      // 254
	RPL_LUSERME,            // 255
	RPL_ADMINME,            // 256
	RPL_ADMINLOC1,          // 257
	RPL_ADMINLOC2,          // 258
	RPL_ADMINEMAIL,         // 259
	RPL_LOCALUSERS,         // 265
	RPL_GLOBALUSERS,        // 266
    RPL_UMODEIS,            // 221 - Added this for user mode replies
	RPL_WHOISUSER,          // 311
	RPL_WHOISSERVER,        // 312
	RPL_WHOISOPERATOR,      // 313
	RPL_ENDOFWHOIS,         // 318
	RPL_WHOISIDLE,          // 317
	RPL_LIST,               // 322 (single channel entry)
	RPL_LISTEND,            // 323
	RPL_CHANNELMODEIS,      // 324
	RPL_CREATIONTIME,       // 329
	RPL_NOTOPIC,            // 331
	RPL_TOPIC,              // 332
	RPL_TOPICWHOTIME,       // 333
	RPL_INVITING,           // 341
	RPL_NAMREPLY,           // 353
	RPL_ENDOFNAMES,         // 366
	RPL_MOTD,               // 372
	RPL_MOTDSTART,          // 375
	RPL_ENDOFMOTD,          // 376
	RPL_YOUREOPER,          // 381
	RPL_REHASHING,          // 382

	// Error Replies
	ERR_NOSUCHNICK,         // 401
	ERR_NOSUCHSERVER,       // 402
	ERR_NOSUCHCHANNEL,      // 403
	ERR_CANNOTSENDTOCHAN,   // 404
	ERR_TOOMANYCHANNELS,    // 405
	ERR_WASNOSUCHNICK,      // 406
	ERR_TOOMANYTARGETS,     // 407
	ERR_NOORIGIN,           // 409
	ERR_NORECIPIENT,        // 411
	ERR_NOTEXTTOSEND,       // 412
	ERR_UNKNOWNCOMMAND,     // 421
	ERR_NOMOTD,             // 422
	ERR_NONICKNAMEGIVEN,    // 431
	ERR_ERRONEUSNICKNAME,   // 432
	ERR_NICKNAMEINUSE,      // 433
	ERR_NICKCOLLISION,      // 436
	ERR_USERNOTINCHANNEL,   // 441
	ERR_NOTONCHANNEL,       // 442
	ERR_USERONCHANNEL,      // 443
	ERR_NOTREGISTERED,      // 451
	ERR_NEEDMOREPARAMS,     // 461
	ERR_ALREADYREGISTERED,  // 462
	ERR_NOPERMFORHOST,      // 463
	ERR_PASSWDMISMATCH,     // 464
	ERR_YOUREBANNED,        // 465
	ERR_CHANNELISFULL,      // 471
	ERR_UNKNOWNMODE,        // 472
	ERR_INVITEONLYCHAN,     // 473
	ERR_BANNEDFROMCHAN,     // 474
	ERR_BADCHANNELKEY,      // 475
	ERR_NOPRIVILEGES,       // 481
	ERR_CHANOPRIVSNEEDED,   // 482
	ERR_NOOPERHOST,         // 491
	ERR_UMODEUNKNOWNFLAG,   // 501
	ERR_USERSDONTMATCH,     // 502

    // Custom/Application Specific (these are internal flags, not IRC replies)
    NOT_OPERATOR,           // Maps to ERR_CHANOPRIVSNEEDED (482) in messages
    INVALID_NICKNAME_CHAR,
    NICKNAME_TOO_LONG,
    NICKNAME_STARTS_INVALID,
    NICKNAME_ALREADY_IN_USE,
    JOIN_SUCCESS,
    PART_SUCCESS,
    KICK_SUCCESS,
    QUIT_SUCCESS,
    MODE_SUCCESS,
    TOPIC_SUCCESS,
    INVITE_SUCCESS,
    PRIVMSG_SUCCESS
};
