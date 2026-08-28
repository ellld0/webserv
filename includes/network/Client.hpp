#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
	public:
		Client();
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();
};

#endif
