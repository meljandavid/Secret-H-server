#include<iostream>

#include"Client.h"
#include"MyServer.h"

int main() {
    bool role;
    std::cout << "SERVER : 0 || CLIENT : 1\n";
    std::cin >> role;

    if (role) {
        Client client;
        client.start();
    }
    else {
        MyServer server;
        server.start();
    }

    system("pause");
}