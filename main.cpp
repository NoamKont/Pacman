#include "Pong.h"
using namespace pong;

int main() {
	Pong p;
	if (p.valid())
		p.run();
	return 0;
}
