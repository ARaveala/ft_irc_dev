/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akuburas <akuburas@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/08 09:49:34 by akuburas          #+#    #+#             */
/*   Updated: 2025/01/08 09:49:35 by akuburas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ****************************************************************************/
/*  ROFL:ROFL:ROFL:ROFL 													  */
/*          _^___      										 				  */
/* L     __/   [] \    										 			      */
/* LOL===__        \   			MY ROFLCOPTER GOES BRRRRRR				  	  */
/* L      \________]  					by fdessoy-				  			  */
/*         I   I     			(fdessoy-@student.hive.fi)				  	  */
/*        --------/   										  				  */
/* ****************************************************************************/

# include <iostream>

class Message
{
	private:
		std::string prefix_;
		std::string command_;
		std::string params_;
		std::string suffix_;
		std::string sender_;
		std::string recipient_;
		void parseIncommingMessage(std::string rawMessage);

	public:
		Message(std::string rawMessage);
		std::string getPrefix() const;
		std::string getCommand() const;
		std::string getParams() const;
		std::string getSuffix() const;
		std::string getSender() const;
		std::string getRecipient() const;
		std::string serialize();
};