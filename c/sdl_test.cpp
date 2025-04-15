#include <SDL2/SDL.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  // Suppress unused parameter warnings
  (void)argc;
  (void)argv;

  std::cout << "Attempting SDL_Init(0)..." << std::endl;
  int result = SDL_Init(0);
  std::cout << "SDL_Init(0) returned: " << result << std::endl;

  if (result != 0) {
    const char* error = SDL_GetError();
    // Explicitly check for NULL or empty string
    if (error && *error) {
      std::cout << "SDL_GetError() returned: \"" << error << "\"" << std::endl;
    }
    else {
      std::cout << "SDL_GetError() returned: "
                << (error ? "Empty String" : "NULL") << std::endl;
    }
    std::cerr << "Error: SDL_Init(0) failed." << std::endl;
    return 1;
  }

  std::cout << "SDL_Init(0) succeeded." << std::endl;
  SDL_Quit();  // Clean up SDL
  return 0;
}