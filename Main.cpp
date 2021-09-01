#include<iostream>

#include"MyClient.h"
#include"MyServer.h"

int main() {
    int role;
    std::cout << "SERVER : 0 || CLIENT : 1\n";
    std::cin >> role;

    if (role == 1) {
        MyClient client;
        client.start();
    }
    else if(role == 0) {
        MyServer server;
        server.start();
    }
    else {
        Client client;
        client.start();
    }

    system("pause");
}