#include "App.hpp"

#include <SDL_main.h>

#ifdef __cplusplus
extern "C"
#endif

int main(int, char **)
{
	gem::App app(1280, 720);

	while (!app.isDone())
	{
		app.update();
	}

	return 0;
}