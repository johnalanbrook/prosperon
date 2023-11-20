#include "steamffi.h"

#ifdef STEAM
#include "steam/steam_api_flat.h"
void steaminit()
{
  SteamAPI_Init();
}
#endif
