#include <iostream>
#include <cassert>
#include "bagel.h"
using namespace std;
using namespace bagel;

void test1() {
	ent_type e0 = World::createEntity();
	assert(e0.id == 0 && "First id is not 0");

	ent_type e1 = World::createEntity();
	assert(e1.id == 1 && "Second id is not 1");

	World::destroyEntity(e0);
	e0 = World::createEntity();
	assert(e0.id == 0 && "Id 0 not recycled after destroy & create");

	cout << "Test 1 passed\n";
}

void run_tests()
{
	test1();
}
