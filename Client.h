#pragma once

#include<SFML/Network/TcpSocket.hpp>
#include<SFML/Network/Packet.hpp>
#include<SFML/Network/IpAddress.hpp>
#include<iostream>
#include<vector>
#include<functional>

class Client {
private:
    sf::TcpSocket socket;
    std::string nick;

    std::function<int(const std::vector<std::string>& args, const std::string& desc)> onChoice;
    std::function<std::string(const std::string& desc)> onAnswer;
    std::function<void(const std::string& msg)> onMessage;

    void loop();

protected:
    void setOnChoice(std::function<int(const std::vector<std::string>& args,
        const std::string& desc)> f) { onChoice = f; }

    void setOnAnswer(std::function<std::string(const std::string& desc)> f) { onAnswer = f; }

    void setOnMessage(std::function<void(const std::string& msg)> f) { onMessage = f; }

public:
    Client();
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
                    int argnum;
                    p >> argnum;
                    std::vector<std::string> args(argnum);
                    for (std::string& s : args) p >> s;

                    std::string desc;
                    p >> desc;

                    sf::Packet resp;
                    int answer = onChoice(args, desc);
                    resp << answer;
                    socket.send(resp);
                    std::cout << "\nAnswer sent (" << args[answer] << ")\n\n";
                }
                else if (type == "answer") {
                    std::string desc;
                    p >> desc;

                    sf::Packet p;
                    p << onAnswer(desc);
                    socket.send(p);
                    std::cout << "\nResponse sent\n";
                }
            } // ASK END
            else if(request == "info") {
                std::string msg;
                p >> msg;
                onMessage(msg);
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

Client::Client() {
    onChoice = [](const std::vector<std::string>& args, const std::string& desc) { return 0; };
    onAnswer = [](const std::string& desc) { return "My answer!"; };
    onMessage = [](const std::string& msg) { };
}

Client::~Client() {
    std::cout << "leaving\n";
    sf::Packet p;
    p << "left " << nick;
    socket.send(p);
    socket.disconnect();
}