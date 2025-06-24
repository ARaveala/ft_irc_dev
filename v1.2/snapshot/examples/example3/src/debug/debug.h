#ifndef __DEBUG_H__
#define __DEBUG_H__
#include "../headers.h"

class Client;
enum e_debug
{
	SUCCESS,
	FAILED,
	THROW_ERR
};
void	debug(const std::string &message, int flag);
void	debugWhois(std::shared_ptr<Client> client);
#endif