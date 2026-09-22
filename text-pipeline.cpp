#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>

#define FONT_SIZE 36
#define MARGGIN (FONT_SIZE * 0.5)

// Referenced https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// Referenced https://freetype.org/freetype2/docs/tutorial/step1.html

int main(int argc, char **argv) {
    const char *fontfile; // 1st argument is the fontfile
    const char *text; // 2nd argument is the text

    fontfile = argv[1];
    text = argv[2];

    if (argc < 3) {
        fprintf (stderr, "usage: hello-harfbuzz font-file.ttf text\n");
        return 1;
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
    if ((ft_error = FT_Set_Char_Size (ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0))){   
        fprintf (stderr, "Font size could not be set\n");
        abort();
    }
    // if (ft_error = FT_Err_Unknown_File_Format ) {
    //     fprintf (stderr, "Font file could be not be opened or read, or is broken\n");
    //     abort();
    // }

    // Create hb-ft font
    hb_font_t *hb_font = hb_ft_font_create(ft_face, NULL);

    // Create hb-buffer and populate
	hb_buffer_t *hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    // Shape text
    hb_shape(hb_font, hb_buffer, NULL, 0);

    // Get glyph info and pos out of the buffer
    unsigned int num_chars = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    // Looks up the glyph index corresponding to the given charcode in the charmap that is currently selected for the face
    FT_GlyphSlot slot = ft_face->glyph;

    // Pen position
    int pen_x, pen_y, n;
    pen_x = 300;
    pen_y = 200;

    for (n = 0; n < num_chars; n++) {
        FT_UInt glyph_index;

        // Retrieve glyph id (from harfbuzz info)
        glyph_index = info[n].codepoint;
        
        // Load glyph image into the slot (erase prev)
        if (ft_error = FT_Load_Glyph (ft_face, glyph_index, FT_LOAD_DEFAULT)) {
            fprintf (stderr, "Error loading glyph image into slot\n");
            abort();
        }

        // Convert to an anti-aliased bitmap
        if (ft_error = FT_Render_Glyph( ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf (stderr, "Error converting glyph to an anti-aliased bitmap\n");
            abort();
        }

        std::cout << "width: " << ft_face->glyph->bitmap.width;
        std::cout << "height: " << ft_face->glyph->bitmap.rows;

        // // Built off of ideas in PPU466.cpp from project 1 
        // // Create texture for glyph
        // GLuint glyph_tex;

        // glGenTextures(1, &glyph_tex);
        // glBindTexture(GL_TEXTURE_2D, glyph_tex);

        // //passing 'nullptr' to TexImage says "allocate memory but don't store anything there":
        // // (textures will be uploaded later)
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, slot->bitmap.width, slot->bitmap.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
        // //make the texture have sharp pixels when magnified:
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // //when access past the edge, clamp to the edge:
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glBindTexture(GL_TEXTURE_2D, 0);

        // Increment pen position (based on harfbuzz pos)
        pen_x += pos[n].x_advance;
        pen_y += pos[n].y_advance;
    }
    
	std::cout << "It worked?" << std::endl;
}
