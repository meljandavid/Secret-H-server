#pragma once

#include"Client.h"

class MyClient : public Client {
public:
	MyClient() {
		setOnMessage([](const std::string& msg) { std::cout << msg << std::endl; });

		setOnChoice([](const std::vector<std::string>& args, const std::string& desc) {
            std::cout << desc << " ";
            for (const std::string& s : args) std::cout << s << " ";

            std::string ans;
            std::cin >> ans;

            int intValue;
            try {
                intValue = std::stoi(ans);
            }
            catch (const std::invalid_argument& ia) {
                intValue = std::find(args.begin(), args.end(), ans) - args.begin();
            }
            return ( intValue % args.size() );
		});

        setOnAnswer([](const std::string& desc) {
            std::cout << desc;
            std::string answer;
            std::cin >> answer;
            return answer;
        });
	}
};