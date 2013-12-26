#include "main.hpp"


Engine::Engine(int screenWidth, int screenHeight) : gameStatus(STARTUP), fovRadius(10),
screenWidth(screenWidth), screenHeight(screenHeight), level(1) {
	TCODConsole::setCustomFont("terminal10x16_gs_ro.png", TCOD_FONT_LAYOUT_ASCII_INROW);
	TCODConsole::initRoot(screenWidth, screenHeight, "libtcod C++ tutorial", false);
	gui = new Gui();
}

Engine::~Engine() {
	term();
	delete(gui);
}

void Engine::nextLevel() {
	level++;
	gui->message(TCODColor::lightViolet, "You take a moment to rest, and recover your strength.");
	player->destructible->heal(player->destructible->maxHp / 2);
	gui->message(TCODColor::red, "After a rare moment of peace, you descend\ndeeper into the heart of the dungeon...");
	delete map;
	// delete all actors but player and stairs
	for (Actor **it = actors.begin(); it != actors.end(); it++) {
		if (*it != player && *it != stairs) {
			delete *it;
			it = actors.remove(it);
		}
	}
	// create a new map
	map = new Map(80, 43);
	map->init(true);
	gameStatus = STARTUP;
}

void Engine::term() {
	actors.clearAndDelete();
	if (map) delete map;
	gui->clear();
}

void Engine::save() {
	if (player->destructible->isDead()) {
		TCODSystem::deleteFile("game.sav");
	}
	else {
		TCODZip zip;
		//save the map first
		zip.putInt(map->width);
		zip.putInt(map->height);
		map->save(zip);
		//then the player
		player->save(zip);
		// then the stairs
		stairs->save(zip);
		//then all other actors
		zip.putInt(actors.size() - 2);

		for (Actor **it = actors.begin(); it != actors.end(); it++) {
			if (*it != player && *it != stairs) {
				(*it)->save(zip);
			}
		}
		gui->save(zip);
		zip.saveToFile("game.sav");
	}
}

void Engine::load() {
	engine.gui->menu.clear();
	engine.gui->menu.addItem(Menu::NEW_GAME, "New game");
	if (TCODSystem::fileExists("game.sav")) {
		engine.gui->menu.addItem(Menu::CONTINUE, "Continue");
	}
	engine.gui->menu.addItem(Menu::EXIT, "Exit");
	Menu::MenuItemCode menuItem = engine.gui->menu.pick();
	if (menuItem == Menu::EXIT || menuItem == Menu::NONE) {
		// Exit or window closed
		exit(0);
	}
	else if (menuItem == Menu::NEW_GAME) {
		// New game
		engine.term();
		engine.init();
	}
	else {
		TCODZip zip;
		// continue a saved game
		engine.term();
		zip.loadFromFile("game.sav");
		//load from map
		int width = zip.getInt();
		int height = zip.getInt();
		map = new Map(width, height);
		map->load(zip);
		//then the player
		player = new Actor(0, 0, 0, NULL, TCODColor::white);
		player->load(zip);
		// the stairs
		stairs = new Actor(0, 0, 0, NULL, TCODColor::white);
		stairs->load(zip);
		actors.push(stairs);
		actors.push(player);
		//then all the other actors
		int nbActors = zip.getInt();
		while (nbActors > 0) {
			Actor *actor = new Actor(0, 0, 0, NULL, TCODColor::white);
			actor->load(zip);
			actors.push(actor);
			nbActors--;
		}
			// finally the message log
			gui->load(zip);
		// to force FOV recomputation
		gameStatus = STARTUP;
	}
}

void Engine::init() {
	player = new Actor(40, 25, '@', "player", TCODColor::white);
	player->destructible = new PlayerDestructible(30, 2, "your cadaver");
	player->attacker = new Attacker(5);
	player->ai = new PlayerAi();
	player->container = new Container(26);
	actors.push(player);
	stairs = new Actor(0, 0, '>', "stairs", TCODColor::white);
	stairs->blocks = false;
	stairs->fovOnly = false;
	actors.push(stairs);
	map = new Map(80, 43);
	map->init(true);
	gui->message(TCODColor::red,"Welcome stranger!\nPrepare to perish in the Tombs of the Ancient Kings.");
	gameStatus = STARTUP;
}

Actor *Engine::getClosestMonster(int x, int y, float range) const {
	Actor *closest = NULL;
	float bestDistance = 1e16f;
	for (Actor **iterator = actors.begin(); iterator != actors.end(); iterator++) {
		Actor *actor = *iterator;
		if (actor != player && actor->destructible && !actor->destructible->isDead()) {
			float distance = actor->getDistance(x, y);
			if (distance < bestDistance && distance <= range || range == 0.0f) {
				bestDistance = distance;
				closest = actor;
			}
		}
	}
	return closest;
}

Actor *Engine::getActor(int x, int y) const {
	for (Actor **iterator=actors.begin();
        iterator != actors.end(); iterator++) {
        Actor *actor=*iterator;
        if ( actor->x == x && actor->y ==y && actor->destructible
            && ! actor->destructible->isDead()) {
            return actor;
        }
    }
    return NULL;
}

bool Engine::pickATile(int *x, int *y, float maxRange) {
	while (!TCODConsole::isWindowClosed()) {
		render();
		//highlight the possible max range
		for (int cx = 0; cx < map->width; cx++) {
			for (int cy = 0; cy < map->height; cy++) {
				if (map->isInFov(cx, cy) && (maxRange  == 0 || player->getDistance(cx, cy) <= maxRange)) {
					TCODColor col = TCODConsole::root->getCharBackground(cx, cy);
					col = col * 1.2f;
					TCODConsole::root->setCharBackground(cx, cy, col);
				}
			}
		}
		TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);
		if (map->isInFov(mouse.cx, mouse.cy) && (maxRange == 0 || player->getDistance(mouse.cx, mouse.cy) <= maxRange)) {
			TCODConsole::root->setCharBackground(mouse.cx, mouse.cy, TCODColor::white);
			if (mouse.lbutton_pressed) {
				*x = mouse.cx;
				*y = mouse.cy;
				return true;
			}
		}
		if (mouse.rbutton_pressed || lastKey.vk != TCODK_NONE) {
			return false;
		}
		TCODConsole::flush();
	}
	return false;
}

void Engine::update() {
	if (gameStatus == STARTUP) map->computeFov();
	gameStatus = IDLE;
	TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);
	if (lastKey.vk == TCODK_ESCAPE) {
		save();
		load();
	}
	player->update();
	if (gameStatus == NEW_TURN) {
		for (Actor **iterator = actors.begin(); iterator != actors.end();
			iterator++) {
			Actor *actor = *iterator;
			if (actor != player) {
				actor->update();
			}
		}
	}
}

void Engine::render() {
	TCODConsole::root->clear();
	// draw the map
	map->render();
	// draw the actors
	for (Actor **iterator=actors.begin();
	    iterator != actors.end(); iterator++) {
		Actor *actor=*iterator;
		if (actor != player	&& ((!actor->fovOnly && map->isExplored(actor->x, actor->y))
			|| map->isInFov(actor->x, actor->y))) {
	        actor->render();
	    }
	}
	player->render();
	gui->render();
	// show the player's stats
	TCODConsole::root->print(1, screenHeight - 2, "HP : %d/%d",
		(int)player->destructible->hp, (int)player->destructible->maxHp);
}

void Engine::sendToBack(Actor *actor) {
	actors.remove(actor);
	actors.insertBefore(actor, 0);
}