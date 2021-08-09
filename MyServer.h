#pragma once
#include"Server.h"
#include<algorithm>
#include<thread>

/*
* TODO:
* - implement checkNewLaw() -> rules
* - client ux++
*/

class MyServer : public Server {
private:
	enum class GameStates { LOBBY, ROUND, END } STATE  = GameStates::LOBBY;
	enum class Roles { LIBERAL, FASCIST, HITLER };

	// Member variables
	std::unordered_map<std::string, Roles> roles;
	std::string president, chancellor;
	std::vector<std::string> living;
	std::vector<Roles> cards;
	int cardptr;
	int anarchy = 0;
	int placedLiberal = 0, placedFascist = 0;

	// Game methods
	virtual void loop() override;
	void gameStart();
	void gameRound();
	void assignRoles();
	void checkNewLaw(std::string law);

	// Helper functions
	std::string roleToString(Roles role);

	struct Rules {
		struct Rule {
			int Liberals, Fascists;
		};

		static std::unordered_map<int, Rule> getRules() {
			return {
				{ 5,  {3,1} },
				{ 6,  {4,1} },
				{ 7,  {4,2} },
				{ 8,  {5,2} },
				{ 9,  {5,3} },
				{ 10, {6,3} }
			};
		}

	};
};

void MyServer::loop() {
	static bool firstCicle = true;
	while (true) {
		// STATE-SPECIFIC
		switch (STATE) {
		case GameStates::LOBBY:
			checkNewConnections();
			if (firstCicle) {
				std::cin.get();
				std::cout << "Press ENTER to start the game\n";
				firstCicle = false;
				std::thread t([=]() {
					if (std::cin.get() == '\n') {
						gameStart();
					}
				});
				t.detach();
			}
			
			break;

		case GameStates::ROUND:
			gameRound();
			break;

		case GameStates::END:
			std::cout << "Secret roles:";
			for (const auto& r : roles) std::cout << r.first << " was " << roleToString(r.second) << std::endl;
			break;

		default: break;
		}

		// EVERY TICK
		verifyConnections();
	}
}

void MyServer::gameStart() {
	std::cout << "Game starting...\n";
	STATE = GameStates::ROUND;

	for (auto it = clients.begin(); it != clients.end(); it++) living.push_back(it->first);

	assignRoles();

	cards.clear();
	for (int i = 0; i < 6; i++) cards.push_back(Roles::LIBERAL);
	for (int i = 0; i < 11; i++) cards.push_back(Roles::FASCIST);
	std::random_shuffle(cards.begin(), cards.end());
	cardptr = cards.size() - 1;

	president = living.front();

	std::cout << "Players int he lobby:";
	for (std::string s : living) std::cout << " " << s;
	std::cout << std::endl;

	STATE = GameStates::ROUND;
}

void MyServer::gameRound() {
	std::cout << "The president is " << president << std::endl;

	living.erase(std::find(living.begin(), living.end(), president));

	chancellor = clientAskForChoice(president, living, "Choose a chancellor!");
	std::cout << "The chancellor is " << chancellor << ", Vote results: ";
	living.push_back(president);
	auto results = clientAskCrowd(living, { "Yes", "No" }, "Can you agree with the government?" );
	std::cout << "[Yes=" << results["Yes"] << "] [No=" << results["No"] << "]\n";

	if (results["No"] <= results["Yes"]) { // SUCCESSFUL
		std::vector<std::string> three;
		if (cardptr < 2) {
			cardptr = cards.size() - 1;
			std::random_shuffle(cards.begin(), cards.end());
		}
		for (int i = 0; i < 3; i++)
			three.push_back( roleToString(cards[cardptr - i]) );
		cardptr -= 3;

		std::string ans = clientAskForChoice(president, three, "Draft a card!");
		three.erase(std::find(three.begin(), three.end(), ans));

		ans = clientAskForChoice(chancellor, three, "Draft a card!");
		three.erase(std::find(three.begin(), three.end(), ans));

		std::string newLaw = three.front();
		std::cout << "The government has made a " << newLaw << " law\n";
		newLaw == "Liberal" ? placedLiberal++ : placedFascist++;

		checkNewLaw(newLaw);

		anarchy = 0;
	}
	else {
		anarchy++;
		std::cout << "Anarchy raied to " << anarchy << std::endl;
		if (anarchy == 3) {
			std::string last = roleToString( cards.back() );
			last == "Liberal" ? placedLiberal++ : placedFascist++;

			std::cout << "The randomly chosen law is " << last << std::endl;

			cards.pop_back();
			checkNewLaw(last);
		}
	}

	// Next player
	auto presptr = std::find(living.begin(), living.end(), president);
	presptr++;
	if (presptr == living.end()) presptr = living.begin();
	president = *presptr;
}

void MyServer::assignRoles() {
	std::random_shuffle(living.begin(), living.end());
	
	auto rules = Rules::getRules();
	for (int i = 0; i < rules[living.size()].Fascists; i++)
		roles[living[i]] = Roles::FASCIST;
	for (int i = 0; i < rules[living.size()].Liberals; i++)
		roles[ living[i + rules[living.size()].Fascists] ] = Roles::LIBERAL;
	roles[living.back()] = Roles::HITLER;

	std::random_shuffle(living.begin(), living.end());
}

void MyServer::checkNewLaw(std::string law) {
	// TODO: implement new law-based actions AND check winning states
}

std::string MyServer::roleToString(Roles role) {
	switch (role) {
	case MyServer::Roles::LIBERAL:
		return "Liberal";
		break;
	case MyServer::Roles::FASCIST:
		return "Fascist";
		break;
	case MyServer::Roles::HITLER:
		return "Hitler";
		break;
	default:
		break;
	}
	return "";
}
