#include "App.hpp"

#include <SDL_main.h>

#ifdef __cplusplus
extern "C"
#endif

int main(int, char **)
{
	for (gem::App app; !app.isDone(); app.update());

	return 0;
}