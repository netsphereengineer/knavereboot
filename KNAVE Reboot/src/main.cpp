#include "main.hpp"
#include <SDL.h>

Engine engine(80,50);

int main(int argc, char *argv[]) {
	engine.load();
    while ( !TCODConsole::isWindowClosed() ) {
    	engine.update();
    	engine.render();
		TCODConsole::flush();
    }
	engine.save();
    return 0;
}

