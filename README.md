# SDL_VTXT
A quick and dirty SDL_Renderer wrapper for the awesome [vertext](https://github.com/kevinmkchin/vertext) library by Kevin Chin, which is required. It allows the vertetx objects to be rendered by SDL_Renderer as geometry. It only supports Screen Space coordinates.

This library ALSO REQUIRES [Sean Barrett's stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) library (which is also a single header). Include like so:

```cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define VERTEXT_IMPLEMENTATION
#include "vertext.h"
#define SDL_VTXT_IMPLEMENTATION
#include "SDL_VTXT.h"
```

See example.c file for more details.
This example includes the [Sweet16](https://github.com/kmar/Sweet16Font) and [Open Sans](https://github.com/googlefonts/opensans) fonts.

Public domain.
