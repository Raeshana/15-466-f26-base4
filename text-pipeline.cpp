#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>

// Referenced https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// Referenced https://freetype.org/freetype2/docs/tutorial/step1.html

int main(int argc, char **argv) {
    const char *fontfile; // 1st argument is the fontfile
    const char *text; // 2nd argument is the text

    if (argc < 3) {
        fprintf (stderr, "usage: hello-harfbuzz font-file.ttf text\n");
    }

    // Initialize library and load font face
    FT_Library ft_library; // handle to library
    FT_Face ft_face;       // handle to face object 
    FT_Error ft_error;     // handle to error

    // Abort if files not found/ not working
    if (ft_error = FT_Init_FreeType( &ft_library )) {
        fprintf (stderr, "Could not find library\n");
        abort();
    }
    if (ft_error = FT_New_Face(ft_library, fontfile, 0, &ft_face)) {
        fprintf (stderr, "Font file could be opened and read, but font format is unsupported\n");
        abort();
    }
    if (ft_error = FT_Err_Unknown_File_Format ) {
        fprintf (stderr, "Font file could be not be opened or read, or is broken\n");
        abort();
    }

    // Create hb-ft font
    hb_font_t *hb_font = hb_ft_font_create(ft_face, NULL);

    // Create hb-buffer and populate
	hb_buffer_t *hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    // Shape text
    hb_shape(hb_font, hb_buffer, NULL, 0);

    // Get glyph info and pos out of the buffer
    unsigned int len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

	std::cout << "It worked?" << std::endl;
}
