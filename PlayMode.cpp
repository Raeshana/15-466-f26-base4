#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "gl_compile_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <ink/story.h>
#include <ink/runner.h>
#include <ink/choice.h>
#include <memory.h>

#define FONT_SIZE 36
#define MARGGIN (FONT_SIZE * 0.5)

// Referenced https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// Referenced https://freetype.org/freetype2/docs/tutorial/step1.html
// Referenced PPU466.cpp from project 1

// text vertex shader
const char *text_vertex_shader = R"(
#version 330

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 texCoord;

uniform mat4 OBJECT_TO_CLIP;

void main() {
    gl_Position = OBJECT_TO_CLIP * vec4(Position, 0.0, 1.0);
    texCoord = TexCoord;
}
)";

// text fragment shader
const char *text_fragment_shader = R"(
#version 330

in vec2 texCoord;

uniform sampler2D glyphTexture;

out vec4 fragColor;

void main() {
    float alpha = texture(glyphTexture, texCoord).r;
    fragColor = vec4(1.0, 1.0, 1.0, alpha);
}
)";

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});


Load< Sound::Sample > honk_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("honk.wav"));
});


PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	// text pipeline
	// Abort if files not found/ not working
    if (FT_Init_FreeType( &ft_library )) {
        fprintf (stderr, "Could not find library\n");
        abort();
    }
	const char *fontfile = "C:\\Windows\\Fonts\\Arial.ttf";
    if (FT_New_Face(ft_library, fontfile, 0, &ft_face)) {
        fprintf (stderr, "Font file could be opened and read, but font format is unsupported\n");
        abort();
    }
    if ((FT_Set_Char_Size (ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0))){   
        fprintf (stderr, "Font size could not be set\n");
        abort();
    }

	// Create hb-ft font
    hb_font = hb_ft_font_create(ft_face, NULL);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			if (honk_oneshot) honk_oneshot->stop();
			honk_oneshot = Sound::play_3D(*honk_sample, 0.3f, glm::vec3(4.6f, -7.8f, 6.9f)); //hardcoded position of front of car, from blender
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//move sound to follow leg tip position:
	leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	
	// TEST RENDERING 1 STRING
	const char *text = "jpi";

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

	float pen_x = 100.0f;
	float pen_y = 100.0f;

	for (unsigned int i = 0; i < num_chars; i++) {
		FT_UInt glyph_index = info[i].codepoint;
		
		// Load glyph image into the slot (erase prev)
		if (FT_Load_Glyph (ft_face, glyph_index, FT_LOAD_DEFAULT)) {
			fprintf (stderr, "Error loading glyph image into slot\n");
			abort();
		}

		// Convert to an anti-aliased bitmap
		if (FT_Render_Glyph( ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
			fprintf (stderr, "Error converting glyph to an anti-aliased bitmap\n");
			abort();
		}

		// Glyph slot for easier access
		FT_GlyphSlot slot = ft_face->glyph;

		// For testing
		printf("glyph index: %u\n", glyph_index);
		printf("bitmap: %d x %d\n", slot->bitmap.width, slot->bitmap.rows);
		printf("pitch: %d\n", slot->bitmap.pitch);
		printf("pixel mode: %d\n", slot->bitmap.pixel_mode);
		printf("bitmap_left: %d, bitmap_top: %d\n", slot->bitmap_left, slot->bitmap_top);

		// Built off of ideas in PPU466.cpp from project 1 
		// Create texture for glyph
		GLuint glyph_tex = 0;

		glGenTextures(1, &glyph_tex);
		glBindTexture(GL_TEXTURE_2D, glyph_tex);

		// Freetype's bitmap is one byte per pixel
		// Keep in mind: 1 alpha channel
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		//passing 'nullptr' to TexImage says "allocate memory but don't store anything there":
		// (textures will be uploaded later)
		// GL_RGBA8 to GL_R8 b/c 1 colour channel instead of 4
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, slot->bitmap.width, slot->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
		//make the texture have sharp pixels when magnified:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//when access past the edge, clamp to the edge:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		// vertex
		struct Vertex {
			glm::vec2 Position;
			glm::vec2 TexCoord;
		};

		// need to account for the bitmap starting points
		float pen_x_offset = pen_x + pos[i].x_offset / 64.0f + slot->bitmap_left; // left
		float pen_y_offset = pen_y - pos[i].y_offset / 64.0f + slot->bitmap_top; // bottom

		// build rectangle representing background and sprites (of bitmap):
		std::vector<Vertex> vertices = {
			{glm::vec2(pen_x_offset, pen_y_offset), glm::vec2(0.0f, 1.0f)},
			{glm::vec2(pen_x_offset + slot->bitmap.width, pen_y_offset), glm::vec2(1.0f, 1.0f)},
			{glm::vec2(pen_x_offset + slot->bitmap.width, pen_y_offset + slot->bitmap.rows), glm::vec2(1.0f, 0.0f)},
			{glm::vec2(pen_x_offset, pen_y_offset), glm::vec2(0.0f, 1.0f)},
			{glm::vec2(pen_x_offset + slot->bitmap.width, pen_y_offset + slot->bitmap.rows), glm::vec2(1.0f, 0.0f)},
			{glm::vec2(pen_x_offset, pen_y_offset + slot->bitmap.rows), glm::vec2(0.0f, 0.0f)}
		};

		//vertex_buffer will (eventually) hold vertex data for drawing:
		GLuint vertex_buffer = 0;
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// vao
		GLuint vertex_array = 0;
		glGenVertexArrays(1, &vertex_array);
		glBindVertexArray(vertex_array);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		// attribute array for position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

		// attribute array for text coords
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// vertex shader
		GLuint text_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(text_vertex_shader_id, 1, &text_vertex_shader, nullptr);
		glCompileShader(text_vertex_shader_id);

		// fragment shader
		GLuint text_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(text_fragment_shader_id, 1, &text_fragment_shader, nullptr);
		glCompileShader(text_fragment_shader_id);

		GLuint text_program = glCreateProgram();
		glAttachShader(text_program, text_vertex_shader_id);
		glAttachShader(text_program, text_fragment_shader_id);
		glLinkProgram(text_program);

		glUseProgram(text_program);

		// uniforms for shader programs
		glm::mat4 OBJECT_TO_CLIP = glm::mat4(
			glm::vec4(2.0f / float(drawable_size.x), 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 2.0f / float(drawable_size.y), 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f)
		);
		GLint object_to_clip = glGetUniformLocation(text_program, "OBJECT_TO_CLIP");
		glUniformMatrix4fv(object_to_clip, 1, GL_FALSE, glm::value_ptr(OBJECT_TO_CLIP));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, glyph_tex);

		GLint glyph_texture = glGetUniformLocation(text_program, "glyphTexture");
		glUniform1i(glyph_texture, 0);

		glBindVertexArray(vertex_array);

		// enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// update pen location
		pen_x += pos[i].x_advance / 64.0f;
		pen_y += pos[i].y_advance / 64.0f;
	}

	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
