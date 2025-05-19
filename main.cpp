#include "Pacman.h"
using namespace pacman;

int main() {
	PacMan p;
	if (p.valid())
		p.run();
	return 0;
}
