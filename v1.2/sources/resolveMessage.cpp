#include "IrcResources.hpp"

// is this a better option for error handling ?  
/*#define RESOLVE_MESSAGE(msgType, params) \
void resolveMessage(MsgType msgType, const std::vector<std::string>& params)

	if (msgType == MsgType::WELCOME ? WELCOME(params[0]) : \
    (msgType) == MsgType::HOST_INFO ? HOST_INFO(params[0]) : \
    (msgType) == MsgType::SERVER_CREATION ? SERVER_CREATION(params[0]) : \
    (msgType) == MsgType::SERVER_INFO ? SERVER_INFO(params[0]) : \
    (msgType) == MsgType::RPL_NICK_CHANGE ? RPL_NICK_CHANGE(params[0], params[1], params[2]) : \
    (msgType) == MsgType::NICKNAME_IN_USE ? NICKNAME_IN_USE(params[0]) : \
    throw std::runtime_error("Unknown message type"))*/

