/*
  SDL_VTXT by Gustavo Aranda (https://github.com/gusarba)

  A quick and dirty SDL_Renderer wrapper for the awesome vertext library by Kevin Chin
  (https://github.com/kevinmkchin/vertext). It allows the vertetx objects to be 
  rendered by SDL_Renderer as geometry. It only supports Screen Space coordinates.

  Requires both stb_truetype.h (https://github.com/nothings/stb/blob/master/stb_truetype.h)
  and vertext.h . Do this:
        #define SDL_VTXT_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.
        // i.e. it should look like this:
        #include ...
        #include ...
        #define STB_TRUETYPE_IMPLEMENTATION
        #include "stb_truetype.h"
        #define VERTEXT_IMPLEMENTATION
        #include "vertext.h"
        #define SDL_VTXT_IMPLEMENTATION
        #include "SDL_VTXT.h"

  See the accompanying file "example.c" to see a practical example.

  USAGE:

  Create a SDL_VTXT object like so:
  
    SDL_VTXT* vf = SDL_VTXT_Init(renderer, 24, "../data/sweet16mono.ttf");

  You can set the cursor, add some text, new lines, glyphs, etc...:
  
    SDL_VTXT_SetCursor(vf, 100, 200);
    SDL_VTXT_AppendLine(vf, "The quick brown fox");
    SDL_VTXT_NewLine(vf, 100);
    SDL_VTXT_AppendLine(vf, "jumps over the lazy dog");
    SDL_VTXT_AppendGlyph(vf, '.');

  Those operations will be stored in an internal command buffer that will 
  execute them in the render phase.

  Somewhere before calling SDL_RenderPresent() , call:

    SDL_VTXT_Render(vf, renderer);

  To render the SDL_VTXT object.

 */

#ifndef __SDL_VTXT__
#define __SDL_VTXT__

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This library requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#ifdef SDL_VTXT_STATIC
#define SDL_VTXT_DEF static
#else
#define SDL_VTXT_DEF extern
#endif

/** VTXTOperation is an enum representing the operations that can be done
 *  with a vertext object: add line, add new line and add glyph.
 */ 
typedef enum VTXTOperation {
  kVTXTOperation_AppendLine,
  kVTXTOperation_NewLine,
  kVTXTOperation_AppendGlyph
} VTXTOperation;

/** VTXTCommand is a command containing an operation and the possible arguments
 *  for that operation.
 */
typedef struct VTXTCommand {
  VTXTOperation op;
  const char* line;
  int cursor_x;
  char glyph;
  struct VTXTCommand* next;
} VTXTCommand;

/** SDL_VTXT is the main data type of the library, representing a vertext
 *  object that can be renderer with a SDL_Renderer.
 *  It contains graphic objects like a SDL_Texture and a SDL_Color buffer
 *  that is resized on the fly to match the vertex buffer.
 */
typedef struct SDL_VTXT {
  int text_size;
  int cursor_x;
  int cursor_y;
  SDL_Color text_color;
  vtxt_font font_handle;
  Uint32 pixel_format;
  SDL_Texture* font_tex;
  SDL_Color* color_buffer;
  int color_array_count;
  struct VTXTCommand* commands;
} SDL_VTXT;

/** Creates and initializes a SDL_VTXT object.
 *  @return NULL if failure.
 */
SDL_VTXT_DEF SDL_VTXT* SDL_VTXT_Init(SDL_Renderer* renderer, int text_size, const char* font_filename);

/** Sets the text color of an SDL_VTXT object.
 *
 *  The color tinting is done by changing the color values of the vertices.
 */
SDL_VTXT_DEF int SDL_VTXT_SetColor(SDL_VTXT* vfont, SDL_Color color);

/** Sets the drawing cursor of an SDL_VTXT object.
 */
SDL_VTXT_DEF int SDL_VTXT_SetCursor(SDL_VTXT* vfont, int x, int y);

/** Adds an AppendLine command to a SDL_VTXT object.
 */
SDL_VTXT_DEF int SDL_VTXT_AppendLine(SDL_VTXT* vfont, const char* line);

/** Adds an NewLine command to a SDL_VTXT object.
 */
SDL_VTXT_DEF int SDL_VTXT_NewLine(SDL_VTXT* vfont, int new_x);

/** Adds an AppendGlyph command to a SDL_VTXT object.
 */
SDL_VTXT_DEF int SDL_VTXT_AppendGlyph(SDL_VTXT* vfont, char glyph);

/** Clears all commands from a SDL_VTXT object, effectively rendering nothing.
 */
SDL_VTXT_DEF int SDL_VTXT_Clear(SDL_VTXT* vfont);

/** Renders a SDL_VTXT object as geometry with a SDL_Renderer.
 */
SDL_VTXT_DEF int SDL_VTXT_Render(SDL_VTXT* vfont, SDL_Renderer* renderer);

/** Destroys and frees a SDL_VTXT object. Do not use the object after this call.
 */
SDL_VTXT_DEF int SDL_VTXT_Release(SDL_VTXT* vfont);

#endif  // __SDL_VTXT__

///////////////////// IMPLEMENTATION //////////////////////////

#ifdef SDL_VTXT_IMPLEMENTATION

SDL_VTXT_DEF int SDL_VTXT_ResizeColorBuffer(SDL_VTXT* vfont, int size) {
  if (vfont->color_array_count != size) {
    // Color buffer is dirty. Reallocate
    vfont->color_buffer = (SDL_Color*) SDL_realloc(vfont->color_buffer, sizeof(SDL_Color) * size);
    if (vfont->color_buffer) {
      vfont->color_array_count = size;
    } else {
      // TODO: LOG
      return -1;
    }
  } else {
    return 0;
  }

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_ResetTexture(SDL_VTXT* vfont, SDL_Renderer* renderer, int use_all_channels) {
  vfont->pixel_format = SDL_MasksToPixelFormatEnum(32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  
  vfont->font_tex = SDL_CreateTexture(renderer, 
      vfont->pixel_format,
      SDL_TEXTUREACCESS_STREAMING, 
      vfont->font_handle.font_atlas.width, 
      vfont->font_handle.font_atlas.height);
  
  void *pixels = NULL;
  int pitch = 0;
  
  SDL_LockTexture(vfont->font_tex, NULL, &pixels, &pitch);
  
  int alpha_idx = 0;
  Uint8* rgba = (Uint8*) pixels;
  for (int row = 0; row < vfont->font_handle.font_atlas.height; ++row) {
    for (int col_byte = 0; col_byte < pitch; col_byte += 4) {
      rgba[0] = vfont->font_handle.font_atlas.pixels[alpha_idx];
      if (use_all_channels == 0) {
        rgba[1] = 0xFF;
        rgba[2] = 0xFF;
        rgba[3] = 0xFF;
      } else {
        rgba[1] = vfont->font_handle.font_atlas.pixels[alpha_idx];
        rgba[2] = vfont->font_handle.font_atlas.pixels[alpha_idx];
        rgba[3] = vfont->font_handle.font_atlas.pixels[alpha_idx];
      }
      ++alpha_idx;
      rgba += 4;
    }
  }
  
  SDL_UnlockTexture(vfont->font_tex);

  SDL_SetTextureBlendMode(vfont->font_tex, SDL_BLENDMODE_BLEND);

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_FillColorBuffer(SDL_VTXT* vfont) {
  for (int ii = 0; ii < vfont->color_array_count; ++ii) {
    vfont->color_buffer[ii].r = vfont->text_color.r;
    vfont->color_buffer[ii].g = vfont->text_color.g;
    vfont->color_buffer[ii].b = vfont->text_color.b;
    vfont->color_buffer[ii].a = vfont->text_color.a;
  }

  return 0;
}

SDL_VTXT_DEF SDL_VTXT* SDL_VTXT_Init(SDL_Renderer* renderer, int text_size, const char* font_filename) {
  // Read ttf file into memory
  unsigned char* font_file = NULL;
  SDL_RWops *rw = SDL_RWFromFile(font_filename, "rb");
  if (rw == NULL) {
    SDL_Log("Could not open font file");
    return NULL;
  } else {
    Sint64 res_size = SDL_RWsize(rw);
    unsigned char* res = (unsigned char*)SDL_malloc(res_size + 1);

    Sint64 nb_read_total = 0, nb_read = 1;
    unsigned char* buf = res;
    while (nb_read_total < res_size && nb_read != 0) {
        nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
        nb_read_total += nb_read;
        buf += nb_read;
    }
    SDL_RWclose(rw);
    if (nb_read_total != res_size) {
        SDL_free(res);
        return NULL;
    }

    res[nb_read_total] = '\0';
    font_file = res;
  }

  SDL_VTXT* ret = (SDL_VTXT*) SDL_malloc(sizeof(SDL_VTXT));
  ret->text_size = text_size;
  
    // Create vertext font
  vtxt_init_font(&ret->font_handle, font_file, text_size);

  SDL_free(font_file);  // Does this break anything?

  ret->text_color.r = 0xFF;
  ret->text_color.g = 0xFF;
  ret->text_color.b = 0xFF;
  ret->text_color.a = 0xFF;
  ret->pixel_format = 0;
  ret->font_tex = NULL;
  ret->commands = NULL;
  ret->color_buffer = NULL;
  ret->color_array_count = 0;
  
  SDL_VTXT_ResetTexture(ret, renderer, 0);

  return ret;
}

SDL_VTXT_DEF int SDL_VTXT_SetColor(SDL_VTXT* vfont, SDL_Color color) {
  vfont->text_color = color;

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_SetCursor(SDL_VTXT* vfont, int x, int y) {
  vfont->cursor_x = x;
  vfont->cursor_y = y;

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_AddCommand(SDL_VTXT* vfont, VTXTCommand* cmd) {
  cmd->next = NULL;
  VTXTCommand** n = &vfont->commands;
  while (*n != NULL) {
    n = &((*n)->next);
  }
  *n = cmd;

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_AppendLine(SDL_VTXT* vfont, const char* line) {
  VTXTCommand* cmd = (VTXTCommand*) SDL_malloc(sizeof(VTXTCommand));
  cmd->op = kVTXTOperation_AppendLine;
  cmd->line = line;
  cmd->cursor_x = 0;
  cmd->glyph = '\0';

  return SDL_VTXT_AddCommand(vfont, cmd);
}

SDL_VTXT_DEF int SDL_VTXT_NewLine(SDL_VTXT* vfont, int new_x) {
  VTXTCommand* cmd = (VTXTCommand*) SDL_malloc(sizeof(VTXTCommand));
  cmd->op = kVTXTOperation_NewLine;
  cmd->line = NULL;
  cmd->cursor_x = new_x;
  cmd->glyph = '\0';

  return SDL_VTXT_AddCommand(vfont, cmd);
}

SDL_VTXT_DEF int SDL_VTXT_AppendGlyph(SDL_VTXT* vfont, char glyph) {
  VTXTCommand* cmd = (VTXTCommand*) SDL_malloc(sizeof(VTXTCommand));
  cmd->op = kVTXTOperation_AppendGlyph;
  cmd->line = NULL;
  cmd->cursor_x = 0;
  cmd->glyph = glyph;

  return SDL_VTXT_AddCommand(vfont, cmd);
}

SDL_VTXT_DEF int SDL_VTXT_Clear(SDL_VTXT* vfont) {
  // Release commands
  VTXTCommand* cmd = vfont->commands;
  while (cmd != NULL) {
    VTXTCommand* n = cmd->next;
    SDL_free(cmd);
    cmd = n;
  }
  vfont->commands = NULL; 

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_Render(SDL_VTXT* vfont, SDL_Renderer* renderer) {
  // Move cursor
  vtxt_move_cursor(vfont->cursor_x, vfont->cursor_y);

  // Execute commands
  VTXTCommand* cmd = vfont->commands;
  while (cmd != NULL) {
    switch(cmd->op) {
     case kVTXTOperation_AppendLine: {
       vtxt_append_line(cmd->line, &vfont->font_handle, vfont->text_size);
       break;
     }
     case kVTXTOperation_NewLine: {
       vtxt_new_line(cmd->cursor_x, &vfont->font_handle);
       break;
     }
     case kVTXTOperation_AppendGlyph: {
       vtxt_append_glyph(cmd->glyph, &vfont->font_handle, vfont->text_size);
       break;
     }
    }

    cmd = cmd->next;
  }

  vtxt_vertex_buffer vb = vtxt_grab_buffer();
 
  SDL_VTXT_ResizeColorBuffer(vfont, vb.vertex_count);
  SDL_VTXT_FillColorBuffer(vfont);

  SDL_RenderGeometryRaw(renderer, vfont->font_tex,
      vb.vertex_buffer,
      4*4,
      (const int*)vfont->color_buffer,
      0,
      vb.vertex_buffer+2,
      4*4,
      vb.vertex_count,
      vb.index_buffer,
      vb.indices_array_count,
      4);
    
  vtxt_clear_buffer();

  return 0;
}

SDL_VTXT_DEF int SDL_VTXT_Release(SDL_VTXT* vfont) {
  // Destroy texture
  if (vfont->font_tex) {
    SDL_DestroyTexture(vfont->font_tex);
  }

  SDL_VTXT_Clear(vfont);

  SDL_free(vfont);

  return 0;
}

#undef SDL_VTXT_IMPLEMENTATION
#endif  // SDL_VTXT_IMPLEMENTATION
