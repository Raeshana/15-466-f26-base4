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

#define FONT_SIZE 36
#define MARGGIN (FONT_SIZE * 0.5)

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

// Based on PPU466 PPUTileProgram
struct TextProgram {
	TextProgram();
	~TextProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	// vec2 instead of ivec2 since we want floating pts
	GLint Position_vec2 = -1;
	GLint TexCoord_vec2 = -1;

	//Uniform (per-invocation variable) locations:
	GLint OBJECT_TO_CLIP_mat4 = -1;

	//Textures bindings:
	// sampler2D instead of usampler2D
	GLint GLYPH_TEXTURE_sampler2D = -1;
	GLint TEXT_COLOUR_vec4 = -1;
};

TextProgram::TextProgram() {
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"in vec2 Position;\n"
		"in vec2 TexCoord;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * vec4(Position, 0.0, 1.0);\n" // vec4 since position was prev vec4 but is now vec2
		"	texCoord = TexCoord;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D GLYPH_TEXTURE;\n"
		"uniform vec4 TEXT_COLOUR;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	float alpha = texture(GLYPH_TEXTURE, texCoord).r;\n"
		"	fragColor = vec4(TEXT_COLOUR.rgb, TEXT_COLOUR.a * alpha);\n"
		"}\n"
	);

	//look up the locations of vertex attributes:
	Position_vec2 = glGetAttribLocation(program, "Position");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	GLYPH_TEXTURE_sampler2D = glGetUniformLocation(program, "GLYPH_TEXTURE");
	TEXT_COLOUR_vec4 = glGetUniformLocation(program, "TEXT_COLOUR");

	//bind texture units indices to samplers:
	glUseProgram(program);
	glUniform1i(GLYPH_TEXTURE_sampler2D, 0);
	glUseProgram(0);

	GL_ERRORS();
}

TextProgram::~TextProgram() {
	if (program != 0) {
		glDeleteProgram(program);
		program = 0;
	}
}

//Initialize text program and associated buffers:
TextProgram text_program;

// Based on PPU466 PPUDataStream
//Text data is streamed to the GPU (read: uploaded 'just in time') using a few buffers:
struct TextDataStream {
	TextDataStream();
	~TextDataStream();

	//vertex format for convenience:
	struct Vertex {
		Vertex(glm::vec2 const &Position_, glm::vec2 const &TexCoord_)
			: Position(Position_), TexCoord(TexCoord_) {}
		//I generally make class members lowercase, but I make an exception here because
		// I use uppercase for vertex attributes in shader programs and want to match.
		glm::vec2 Position;
		glm::vec2 TexCoord;
	};

	//vertex buffer that will store data stream:
	GLuint vertex_buffer = 0;

	//vertex array object that maps text program attributes to vertex storage:
	GLuint vertex_buffer_for_text_program = 0;
};

TextDataStream::TextDataStream() {
	//vertex_buffer_for_tex_program is a vertex array object that tells the GPU the layout of data in vertex_buffer:
	glGenBuffers(1, &vertex_buffer);
	glGenVertexArrays(1, &vertex_buffer_for_text_program);

	//vertex_buffer will (eventually) hold vertex data for drawing:
	glBindVertexArray(vertex_buffer_for_text_program);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	//Notice how this binding is attaching an integer input to a floating point attribute:
	glVertexAttribPointer(
		text_program.Position_vec2, //attribute
		2, //size
		GL_FLOAT, //type
		GL_FALSE, //normalized
		sizeof(Vertex), //stride
		(void *)offsetof(Vertex, Position) //offset
	);
	glEnableVertexAttribArray(text_program.Position_vec2);

	//the "I" variant binds to an integer attribute:
	glVertexAttribPointer(
		text_program.TexCoord_vec2, //attribute
		2, //size
		GL_FLOAT, //type
		GL_FALSE, //normalized
		sizeof(Vertex), //stride
		(void *)offsetof(Vertex, TexCoord) //offset
	);
	glEnableVertexAttribArray(text_program.TexCoord_vec2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	GL_ERRORS();
}

TextDataStream::~TextDataStream() {
	glDeleteBuffers(1, &vertex_buffer);
	glDeleteVertexArrays(1, &vertex_buffer_for_text_program);
}

TextDataStream text_data_stream;

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

	return;

	// text pipeline
	FT_UInt glyph_index = FT_Get_Char_Index(ft_face, 'l');
	
	// Load glyph image into the slot (erase prev)
	if (FT_Load_Glyph (ft_face, glyph_index, FT_LOAD_DEFAULT)) {
		fprintf (stderr, "Error loading glyph image into slot\n");
		abort();
	}
	
	// Glyph slot for easier access
	FT_GlyphSlot slot = ft_face->glyph;

	// Convert to an anti-aliased bitmap
	if (FT_Render_Glyph( slot, FT_RENDER_MODE_NORMAL)) {
		fprintf (stderr, "Error converting glyph to an anti-aliased bitmap\n");
		abort();
	}

	// Built off of ideas in PPU466.cpp from project 1 
	// Create texture for glyph
	GLuint glyph_tex;

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

	//build rectangle representing background and sprites (of bitmap):
	// std::vector<TextDataStream::Vertex> vertices = {
	// 	TextDataStream::Vertex(glm::vec2(100.0f, 100.0f), glm::vec2(0.0f, 0.0f)),
	// 	TextDataStream::Vertex(glm::vec2(100.0f + slot->bitmap.width, 100.0f), glm::vec2(1.0f, 0.0f)),
	// 	TextDataStream::Vertex(glm::vec2(100.0f + slot->bitmap.width, 100.0f + slot->bitmap.rows),
	// 							glm::vec2(1.0f, 1.0f)),
	// 	TextDataStream::Vertex(glm::vec2(100.0f, 100.0f), glm::vec2(0.0f, 0.0f)),
	// 	TextDataStream::Vertex(glm::vec2(100.0f + slot->bitmap.width, 100.0f + slot->bitmap.rows),
	// 							glm::vec2(1.0f, 1.0f)),
	// 	TextDataStream::Vertex(glm::vec2(100.0f, 100.0f + slot->bitmap.rows), glm::vec2(0.0f, 1.0f))
	// };

	// //upload vertex data:
	// glBindBuffer(GL_ARRAY_BUFFER, text_data_stream.vertex_buffer);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(TextDataStream::Vertex) * vertices.size(), vertices.data(), GL_STREAM_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);

	// // set the shader programs:
	// glUseProgram(text_program.program);

	// // configure attribute streams:
	// glBindVertexArray(text_data_stream.vertex_buffer_for_text_program);

	// // set uniforms for shader programs:
	// glUniform4f(text_program.TEXT_COLOUR_vec4, 1.0f, 1.0f, 1.0f, 1.0f);
	// glm::mat4 OBJECT_TO_CLIP = glm::mat4(
	// 	glm::vec4(2.0f / float(drawable_size.x), 0.0f, 0.0f, 0.0f),
	// 	glm::vec4(0.0f, 2.0f / float(drawable_size.y), 0.0f, 0.0f),
	// 	glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
	// 	glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f)
	// );

	// glUniformMatrix4fv(text_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(OBJECT_TO_CLIP));

	// // bind texture units to proper texture objects:
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, glyph_tex);

	// //now that the pipeline is configured, trigger drawing of rectangle:
	// glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(vertices.size()));

	// //return state to default:
	// glActiveTexture(GL_TEXTURE0);
	// glBindVertexArray(0);
	// glUseProgram(0);

	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
