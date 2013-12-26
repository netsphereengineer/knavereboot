class Engine {
public :
	enum GameStatus {
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus;
    TCODList<Actor *> actors;
    Actor *player;
	Actor *stairs;
    Map *map;
	int level;
	void nextLevel();
    int fovRadius;
	int screenWidth;
	int screenHeight;
	Gui *gui;
	TCOD_key_t lastKey;
	TCOD_mouse_t mouse;

	Engine(int screenWidth, int screenHeight);
 
    Engine();
    ~Engine();
	Actor *getClosestMonster(int x, int y, float range) const;
	Actor *getActor(int x, int y) const;
	bool pickATile(int *x, int *y, float maxRange = 0.0f);
    void update();
    void render();
	void sendToBack(Actor *actor);
	void init();
	void term();
	void load();	
	void save();
};
 
extern Engine engine;