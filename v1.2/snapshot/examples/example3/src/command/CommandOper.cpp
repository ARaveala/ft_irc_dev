#include "Command.h"

void Command::handleOper(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleOper") << std::endl;
		return;
	}
	std::string requested_nick = client_ptr->getNickname();
	std::string client_ip = client_ptr->getIpAddress();
	std::string client_host = client_ptr->getHostname();
	int client_fd = client_ptr->getFd();
	std::vector <t_opers> operator_file = server_ptr_->getOperatorsFile();
	std::vector<std::string> params = msg.getParameters();
	if (params.size() != 2)
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), msg.getCommand()));
		return;
	}
	std::string user_input_nick = params[0];
	std::string user_input_pass = params[1];	
	if (requested_nick != user_input_nick)
	{
		server_ptr_->sendResponse(client_fd, ERR_NOOPERHOST(server_ptr_->getServerHostname(), requested_nick));
		return;
	}
	else
	{
		std::vector<t_opers>::iterator it;
		for (it = operator_file.begin(); it != operator_file.end();it++)
		{
			if (it->nick == user_input_nick)
			{
				if (it->password == user_input_pass)
				{
					if (it->hostmask == client_host || it->hostmask == client_ip)
					{
						server_ptr_->sendResponse(client_fd, RPL_YOUREOPER(server_ptr_->getServerHostname(), requested_nick));
						client_ptr->setModeLocalOp(true);
						return;
					}
					else
					{
						server_ptr_->sendResponse(client_fd, ERR_NOOPERHOST(server_ptr_->getServerHostname(), requested_nick));
						return;
					}
				}
				else
				{
					server_ptr_->sendResponse(client_fd, ERR_PASSWDMISMATCH(server_ptr_->getServerHostname(), requested_nick));
					return;
				}
			}
		}
		if (it == operator_file.end())
		{
			server_ptr_->sendResponse(client_fd, ERR_NOOPERHOST(server_ptr_->getServerHostname(), requested_nick));
			return;
		}
	}
}