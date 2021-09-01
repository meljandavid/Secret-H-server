#pragma once
#include"Server.h"
#include"Log.h"
#include<algorithm>
#include<thread>
#include<functional>
#include<cstdio>

/*
* TODO:
* - prevchancellor crashes the server - why?
* - managing left players (waiting for rejoin?) (game logic threadblocking)
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
	void gameRound(std::string president);
	void assignRoles();
	void checkNewLaw(std::string law);
	void prepareNextRound();

	// Helper functions
	std::string roleToString(Roles role);

	// Rule related
	struct Rule {
		int Liberals = 0, Fascists = 0;
		std::vector<std::function<void()>> Actions;
	};
	std::unordered_map<int, Rule> generateRules();
	Rule rule;
public:
	MyServer() { srand(time(0)); }
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
				Log::write("Press ");
				Log::write("ENTER", Log::Colors::INVERTED);
				Log::writeLine(" to start the game");
				firstCicle = false;

				std::thread t( [=]() {
					while(true)
						if (std::cin.get() == '\n' && clients.size() >= 5) {
							gameStart();
							return;
						}
				});
				t.detach();
			}
			
			break;

		case GameStates::ROUND:
			gameRound(president);
			prepareNextRound();
			break;

		case GameStates::END: {
			static bool firstEnd = false;

			if (!firstEnd) {
				firstEnd = true;
				std::string msg = "Secret roles:\n";

				for (const auto& r : roles) {
					msg += r.first + " was " + roleToString(r.second) + "\n";
					Log::write(r.first + " was ");
					Log::writeLine(
						roleToString(r.second),
						r.second == Roles::LIBERAL ? Log::Colors::BLUE : Log::Colors::RED
					);
				}
				Log::writeLine();

				std::vector<std::string> all;
				for (auto it = clients.begin(); it != clients.end(); it++)
					all.push_back(it->first);

				clientInfoGroup(all, msg);
			}

			break;
		}
		default: break;
		}

		// EVERY TICK
		verifyConnections();
	}
}

void MyServer::gameStart() {
	Log::writeLine("game starting...");

	// declaring rules: players -> L, F, Action array
	rule = generateRules()[clients.size()];

	// making a list of players' names
	for (auto it = clients.begin(); it != clients.end(); it++) living.push_back(it->first);

	// lobby info
	Log::writeLine();
	Log::write("Players in the lobby: ");
	for (const std::string& s : living) Log::write(" " + s);
	Log::writeLine();

	assignRoles();

	// generating cards
	cards.clear();
	for (int i = 0; i < 6; i++) cards.push_back(Roles::LIBERAL);
	for (int i = 0; i < 11; i++) cards.push_back(Roles::FASCIST);
	std::random_shuffle(cards.begin(), cards.end());
	cardptr = cards.size() - 1;

	president = living.front();

	STATE = GameStates::ROUND;
}

void MyServer::gameRound(std::string president) {
	// Print some useful game-related data
	Log::writeLine();
	Log::write("[NEW ROUND]", Log::Colors::INVERTED);
	Log::write(" Anarchy:" + std::to_string(anarchy) + " ");
	Log::write("[" + std::to_string(placedFascist) + "]", Log::Colors::RED);
	Log::write(" ");
	Log::writeLine("[" + std::to_string(placedLiberal) + "]", Log::Colors::BLUE);
	Log::writeLine( "The president is " + president );

	// removing the president and the previous chancellor
	living.erase(std::find(living.begin(), living.end(), president));
	/*
	std::string prevchancellor = chancellor;
	auto cptr = std::find(living.begin(), living.end(), prevchancellor);
	if (cptr != living.end()) living.erase(cptr);
	*/

	chancellor = clientAskForChoice(president, living, "Choose a chancellor!");
	Log::write( "The chancellor is " + chancellor + ", Vote results: ");

	living.push_back(president);
	/*
	if (std::find(living.begin(), living.end(), prevchancellor) == living.end())
		living.push_back(prevchancellor);
	*/

	auto results = clientAskCrowd(living, { "Yes", "No" }, "Can you agree with the government?" );


	Log::write("Yes=");
	Log::write(std::to_string(results["Yes"]), Log::Colors::GREEN);
	Log::write(", No=");
	Log::writeLine(std::to_string(results["No"]), Log::Colors::RED);

	if (results["No"] <= results["Yes"]) { // GOVERNMENT FORMED
		// if Hitler was the chancellor
		if (placedFascist >= 3 && roles[chancellor] == Roles::HITLER) {
			Log::writeLine("\n\n");
			Log::writeLine("Hitler was the chancellor!", Log::Colors::RED);
			Log::writeLine();
			clientInfoAll("Hitler was the chancellor!");
			STATE = GameStates::END;
			return;
		}

		// CARD DRAFTING
		std::vector<std::string> three;
		if (cardptr < 2) {
			cardptr = cards.size() - 1;
			std::random_shuffle(cards.begin(), cards.end());
		}
		for (int i = 0; i < 3; i++)
			three.push_back( roleToString(cards[cardptr - i]) );
		cardptr -= 3;

		// President's turn
		std::string ans = clientAskForChoice(president, three, "Draft a card!");
		three.erase(std::find(three.begin(), three.end(), ans));

		// Chancellor's turn
		ans = clientAskForChoice(chancellor, three, "Draft a card!");
		three.erase(std::find(three.begin(), three.end(), ans));

		// Processing the new law
		std::string newLaw = three.front();
		Log::write("The government has made a ");
		Log::write(newLaw, newLaw=="Liberal" ? Log::Colors::BLUE : Log::Colors::RED);
		Log::writeLine(" law");
		newLaw == "Liberal" ? placedLiberal++ : placedFascist++;

		checkNewLaw(newLaw);

		anarchy = 0;
	}
	else { // ANARCHY
		anarchy++;
		Log::writeLine("Anarchy raised to " + std::to_string(anarchy));
		if (anarchy == 3) {
			std::string last = roleToString( cards.back() );
			last == "Liberal" ? placedLiberal++ : placedFascist++;

			Log::write("The randomly chosen law is ");
			Log::writeLine(last, last == "Liberal" ? Log::Colors::BLUE : Log::Colors::RED);

			cards.pop_back();
			checkNewLaw(last);
		}
	}
}

void MyServer::prepareNextRound() {
	auto presptr = std::find(living.begin(), living.end(), president);
	presptr++;
	if (presptr == living.end()) presptr = living.begin();
	president = *presptr;
}

void MyServer::assignRoles() {
	std::random_shuffle(living.begin(), living.end());
	
	// Generating roles
	for (int i = 0; i < rule.Fascists; i++)
		roles[living[i]] = Roles::FASCIST;
	for (int i = 0; i < rule.Liberals; i++)
		roles[ living[i + rule.Fascists] ] = Roles::LIBERAL;
	roles[living.back()] = Roles::HITLER;

	// select fascists
	std::string fascists;
	for (const std::string& s : living) {
		if (roles[s] == Roles::FASCIST) fascists += s;
	}

	// Sending the roles to the players
	for (auto it = clients.begin(); it != clients.end(); it++) {
		sf::Packet p;
		std::string msg = "You are a " + roleToString( roles[it->first] );

		if (roles[it->first] == Roles::FASCIST) msg += "\nYour mates: " + fascists;

		p << "info" << msg;
		it->second->send(p);
	}

	std::random_shuffle(living.begin(), living.end());
}

void MyServer::checkNewLaw(std::string law) {
	if (placedLiberal == 5) {
		std::string msg = "Liberals won!";
		Log::writeLine("\n\n");
		Log::writeLine(msg, Log::Colors::WHITE * 16 + Log::Colors::BLUE);
		Log::writeLine();
		clientInfoAll(msg);
		STATE = GameStates::END;
	}
	else if (placedFascist == 6) {
		std::string msg = "Fascists won!";
		Log::writeLine("\n\n");
		Log::writeLine(msg, Log::Colors::WHITE * 16 + Log::Colors::RED);
		Log::writeLine();
		clientInfoAll(msg);
		STATE = GameStates::END;
	}
	else {
		if (law == "Fascist") rule.Actions[placedFascist]();
	}
}

inline std::string MyServer::roleToString(Roles role) {
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

std::unordered_map<int, MyServer::Rule> MyServer::generateRules() {
	
	// DEFINE ACTIONS

	auto noAction = []() {};

	auto investigate = [=]() {
		std::string p = clientAskForChoice(president, living, "Choose a player to investigate!");
		Log::writeLine("The president just investigated " + p);
		clientInfo(president, p + " is a " + roleToString(roles[p]));
	};

	auto execution = [=]() {
		std::string p = clientAskForChoice(president, living, "Choose a player to kill!");
		std::string msg = president + " killed " + p + "!";
		clientInfoAll(msg);
		Log::write(msg, Log::Colors::RED * 16);
		Log::writeLine();
		living.erase(std::find(living.begin(), living.end(), p));

		if (roles[p] == Roles::HITLER) {
			Log::writeLine("\n\n");
			Log::writeLine("Hitler dead!", Log::Colors::BLUE);
			Log::writeLine();
			clientInfoAll("Hitler was killed");
			STATE = GameStates::END;
		}
	};

	auto peekLaw = [=]() {
		Log::writeLine("The president peeking the top law O.O");
		clientInfo(president, "The next law is " + roleToString(cards.back()));
	};

	auto chooseNext = [=]() {
		std::string p = clientAskForChoice(president, living, "Choose the next president!");
		Log::writeLine(president + " choosed " + p + " for the next president");
		gameRound(p);
	};

	// make rules
	std::unordered_map<int, MyServer::Rule> rules = {
		{ 5,  {3,1} },
		{ 6,  {4,1} },
		{ 7,  {4,2} },
		{ 8,  {5,2} },
		{ 9,  {5,3} },
		{ 10, {6,3} }
	};

	rules[5].Actions = rules[6].Actions = 
		{ noAction, noAction, noAction, peekLaw, execution, execution, noAction };
	rules[7].Actions = rules[8].Actions = 
		{ noAction, noAction, investigate, chooseNext, execution, execution, noAction };
	rules[9].Actions = rules[10].Actions =
		{ noAction, investigate, investigate, chooseNext, execution, execution, noAction };
	

	return rules;
}
