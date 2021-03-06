#include <stdio.h>
#include "main.hpp"

Destructible::Destructible(float maxHp, float defense, const char *corpseName, int xp, int maxAp) :
maxHp(maxHp), hp(maxHp), defense(defense), corpseName(corpseName), xp(xp), maxAp(maxAp), ap(maxAp) {
}

void Destructible::load(TCODZip &zip) {
	maxHp = zip.getFloat();
	hp = zip.getFloat();
	defense = zip.getFloat();
	corpseName = _strdup(zip.getString());
	maxAp = zip.getInt();
	ap = zip.getInt();
}

void Destructible::save(TCODZip &zip) {
	zip.putFloat(maxHp);
	zip.putFloat(hp);
	zip.putFloat(defense);
	zip.putString(corpseName);
	zip.putInt(maxAp);
	zip.putInt(ap);
}

float Destructible::takeDamage(Actor *owner, float damage) {
	damage -= defense;
	if (damage > 0) {
		hp -= damage;
		if (hp <= 0) {
			die(owner);
		}
	}
	else {
		damage = 0;
	}
	return damage;
}

float Destructible::heal(float amount) {
	hp += amount;
	if (hp > maxHp) {
		amount -= hp - maxHp;
		hp = maxHp;
	}
	return amount;
}

void Destructible::die(Actor *owner) {
	// transform the actor into a corpse!
	owner->ch = '%';
	owner->col = TCODColor::darkRed;
	owner->name = corpseName;
	owner->blocks = false;
	// make sure corpses are drawn before living actors
	engine.sendToBack(owner);
}

MonsterDestructible::MonsterDestructible(float maxHp, float defense, const char *corpseName, int xp, int maxAp) :
Destructible(maxHp, defense, corpseName, xp, maxAp) {
}

PlayerDestructible::PlayerDestructible(float maxHp, float defense, const char *corpseName, int maxAp) :
Destructible(maxHp, defense, corpseName, 0, maxAp) {
}

void MonsterDestructible::die(Actor *owner) {
	// transform it into a nasty corpse! it doesn't block, can't be
	// attacked and doesn't move
	engine.gui->message(TCODColor::lightGrey, "%s is dead. You gain %d exp", owner->name, owner->destructible->xp);
	engine.player->destructible->xp += xp;
	Destructible::die(owner);
}

void PlayerDestructible::die(Actor *owner) {
	engine.gui->message(TCODColor::red, "You died!");
	Destructible::die(owner);
	engine.gameStatus = Engine::DEFEAT;
}

void PlayerDestructible::save(TCODZip &zip) {
	zip.putInt(PLAYER);
	Destructible::save(zip);
}

void MonsterDestructible::save(TCODZip &zip) {
	zip.putInt(MONSTER);
	Destructible::save(zip);
}

Destructible *Destructible::create(TCODZip &zip) {
	DestructibleType type = (DestructibleType)zip.getInt();
	Destructible *destructible = NULL;
	switch (type) {
	case MONSTER: destructible = new MonsterDestructible(0, 0, NULL, 0, 0); break;
	case PLAYER: destructible = new PlayerDestructible(0, 0, NULL, 0); break;
	}
	destructible->load(zip);
	return destructible;
}