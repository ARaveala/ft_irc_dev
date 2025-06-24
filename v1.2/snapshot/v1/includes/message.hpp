

// https://modern.ircdocs.horse/#message-format
//  When reading messages from a stream, read the incoming data into a buffer.
// Only parse and process a message once you encounter the \r\n at the end of it.
// If you encounter an empty message, silently ignore it.
// When sending messages, ensure that a pair of \r\n characters follows every
// single message your software sends out

class message{
	private
		std::string from (array)
		std::string to (array)
		std::string subject
		std::string content

	public
		send_message
		recieve_message
		parse_message
}