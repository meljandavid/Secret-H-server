#pragma once

#include<SFML/Network/IpAddress.hpp>
#include<SFML/Network/TcpSocket.hpp>
#include<SFML/Network/TcpListener.hpp>
#include<SFML/Network/Packet.hpp>
#include<SFML/Network/SocketSelector.hpp>
#include<iostream>
#include<unordered_map>
#include<vector>
#include<memory>

class Server {
private:
    const int PORT;

protected:
    sf::TcpListener listener;
    sf::SocketSelector selector;
    std::unordered_map<std::string, std::unique_ptr<sf::TcpSocket>> clients;

    void checkNewConnections();
    void verifyConnections();
    void clientLeft(const std::string& clientName);

    virtual void loop();

public:
	Server(int port = 53000) : PORT(port) {}
    ~Server();

    void start();

    std::string clientAskForChoice(const std::string& clientName,
        const std::vector<std::string>& choices, const std::string& desc = "", float timeout = 0.f);
    std::string clientAskForAnswer(const std::string& clientName, const std::string& desc = "", float timeout = 0.f);
    std::unordered_map<std::string, int> clientAskCrowd(const std::vector<std::string>& people,
        const std::vector<std::string>& choices, const std::string& desc = "", float timeout = 0.f);
    void clientInfo(const std::string& clientName, const std::string& msg);
    void clientInfoGroup(const std::vector<std::string>& people, const std::string& msg);
    void clientInfoAll(const std::string& msg);
};


void Server::checkNewConnections() {
    auto client = std::make_unique<sf::TcpSocket>();
    if (listener.accept(*client) == sf::Socket::Done) {
        std::string name;
        sf::Packet p;
        if (client->receive(p) != sf::Socket::Done) std::cout << "failed to receive data\n";
        else {
            p >> name;
            std::cout << name << " joined! There are " << clients.size()+1 << " players\n";
        }

        char greet[100] = "Welcome in the server!";
        if (client->send(greet, 100) != sf::Socket::Done) std::cout << "failed to send greeting\n";

        selector.add(*client);
        clients[name] = move(client);
    }
}

void Server::verifyConnections() {
    if (selector.wait(sf::seconds(2.f))) {
        for (auto it = clients.begin(); it != clients.end(); it++) {
            if (it->second->getRemoteAddress() == sf::IpAddress::None)
                clientLeft(it->first);

            if (selector.isReady(*it->second)) {
                std::string message;
                sf::Packet packet;
                it->second->receive(packet);
                packet >> message;

                if (message.size() == 0) clientLeft(it->first);
            }
        }
    }
}


void Server::start() {
    listener.setBlocking(false);
    if (listener.listen(PORT) != sf::Socket::Done) {
        std::cout << "Failed to init server\n";
        return;
    }
    else std::cout << "listening...\n";

    loop();
}

void Server::loop() {
    while (true) {
        checkNewConnections();
        verifyConnections();
    }
}

Server::~Server() {
    std::cout << "Server closed\n";
}


void Server::clientLeft(const std::string& clientName) {
    std::cout << clientName << " left :(\n";
    selector.clear();
    for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->first == clientName) {
            clients.erase(it);
        }
        else {
            selector.add(*it->second);
        }
    }
}


// Asks an user for an item's index of the passed vector<string> 
std::string Server::clientAskForChoice(const std::string& clientName,
    const std::vector<std::string>& choices, const std::string& desc, float timeout)
{
    // Construct request
    sf::Packet p;
    p << "ask" << "choice" << choices.size();
    for (const std::string& choice : choices) p << choice;
    p << desc;

    // send it
    clients[clientName]->send(p);

    // Receive answer with timeout
    sf::SocketSelector waitSelector;
    waitSelector.add(* clients[clientName]);
    
    if (timeout > .1f) {
        if (waitSelector.wait(sf::seconds(timeout))) {
            sf::Packet p;
            clients[clientName]->receive(p);
            int response;
            p >> response;
            return choices[ response % choices.size() ];
        }
        else return choices.front();
    }
    else {
        sf::Packet p;
        clients[clientName]->receive(p);
        int response;
        p >> response;
        return choices[ response % choices.size() ];
    }
}


// Asks an user for an anwer (string)
std::string Server::clientAskForAnswer(const std::string& clientName, const std::string& desc, float timeout) {
    sf::Packet p;
    p << "ask" << "answer" << desc;

    // send it
    clients[clientName]->send(p);

    // Receive answer with timeout
    sf::SocketSelector waitSelector;
    waitSelector.add(*clients[clientName]);

    if (timeout > .1f) {
        if (waitSelector.wait(sf::seconds(timeout))) {
            sf::Packet p;
            clients[clientName]->receive(p);
            std::string response;
            p >> response;
            return response;
        }
        else return "";
    }
    else {
        sf::Packet p;
        clients[clientName]->receive(p);
        std::string response;
        p >> response;
        return response;
    }
}


std::unordered_map<std::string, int> Server::clientAskCrowd(const std::vector<std::string>& people,
    const std::vector<std::string>& choices, const std::string& desc, float timeout)
{
    // create result
    std::unordered_map<std::string, int> results;
    for (const std::string& s : choices) results[s] = 0;

    // Construct request
    sf::Packet request;
    request << "ask" << "choice" << choices.size();
    for (const std::string& choice : choices) request << choice;
    request << desc;

    for (const std::string& person : people) {
        clients[person]->send(request);
    }
    
    if (timeout > 0.1f) {
        if (selector.wait(sf::seconds(timeout))) {
            for (const std::string& person : people) {
                sf::Packet p;
                int answer;
                clients[person]->receive(p);
                if (!(p >> answer)) continue;
                results[choices[answer % choices.size()]]++;
            }
        }
    }
    else {
        int pending = people.size();
        while (pending > 0) {
            if (selector.wait(sf::seconds(1.f))) {
                for (const std::string& person : people) {
                    if (selector.isReady(*clients[person])) {
                        sf::Packet p;
                        int answer;
                        clients[person]->receive(p);
                        p >> answer;
                        results[choices[answer % choices.size()]]++;
                        pending--;
                    }
                }
            }
        }
    }

    return results;
}

void Server::clientInfo(const std::string& clientName, const std::string& msg) {
    sf::Packet p;
    p << "info" << msg;
    clients[clientName]->send(p);
}

void Server::clientInfoGroup(const std::vector<std::string>& people, const std::string& msg) {
    for (const std::string& p : people)
        clientInfo(p, msg);
}

void Server::clientInfoAll(const std::string& msg) {
    for (auto it = clients.begin();  it != clients.end(); it++)
        clientInfo(it->first, msg);
}
