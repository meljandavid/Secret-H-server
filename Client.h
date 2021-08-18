#pragma once

#include<SFML/Network/TcpSocket.hpp>
#include<SFML/Network/Packet.hpp>
#include<SFML/Network/IpAddress.hpp>
#include<iostream>
#include<vector>

class Client {
private:
    sf::TcpSocket socket;
    std::string nick;

    void loop();

public:
	Client() {}
    ~Client();

    void start();
};


void Client::loop() {
    while (true) {
        sf::Packet p;

        // INCOMING REQUEST
        if (socket.receive(p) == sf::Socket::Status::Done) {
            std::string request;
            p >> request;

            if (request == "ask") {
                std::string type;
                p >> type;

                if (type == "choice") {
                    std::string desc;
                    p >> desc;
                    if (desc.size() > 0) std::cout << desc << " ";

                    int argnum;
                    p >> argnum;
                    std::vector<std::string> args(argnum);
                    for (std::string& s : args) p >> s;
                    for (const std::string& s : args) std::cout << s << " ";

                    std::string ans;
                    std::cin >> ans;

                    int intValue;
                    try {
                        intValue = std::stoi(ans);
                    }
                    catch (const std::invalid_argument& ia) {
                        intValue = std::find(args.begin(), args.end(), ans) - args.begin();
                        intValue %= args.size();
                    }

                    sf::Packet resp;
                    resp << intValue;
                    socket.send(resp);
                    std::cout << "\nAnswer sent (" << args[intValue] << ")\n\n";
                }
                else if (type == "answer") {
                    std::string desc;
                    p >> desc;
                    if (desc.size() > 0) std::cout << desc << std::endl;

                    sf::Packet p;
                    /*
                    std::string ans;
                    std::cin >> ans;
                    p << ans;
                    */
                    p << "My Custom Answer";
                    socket.send(p);
                    std::cout << "\nResponse sent\n";
                }
            } // ASK END
            else if(request == "info") {
                std::string msg;
                p >> msg;
                std::cout << msg << std::endl;
            } // INFO END
        }
    }
}

void Client::start() {
    std::cout << "Set a nickname: ";
    std::cin >> nick;;

    sf::Socket::Status status = socket.connect(sf::IpAddress::getLocalAddress(), 53000);

    if (status != sf::Socket::Done) {
        std::cout << "connection failed\n";
        return;
    }
    else {
        sf::Packet p; p << nick;
        if (socket.send(p) != sf::Socket::Done) {
            std::cout << "failed to join\n";
            return;
        }

        char msg[100]; std::size_t rec;
        if (socket.receive(msg, 100, rec) != sf::Socket::Done) {
            std::cout << "no response :/\n";
            return;
        }
        else std::cout << msg << std::endl;

        std::string input;
        std::getline(std::cin, input);

        loop();
    }
}

Client::~Client() {
    std::cout << "leaving\n";
    sf::Packet p;
    p << "left " << nick;
    socket.send(p);
    socket.disconnect();
}