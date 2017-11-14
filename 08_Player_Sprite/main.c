#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk/gtk.h>
#include "DashGL/dashgl.h"

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *context);

#define WIDTH 640.0f
#define HEIGHT 480.0f

GLuint program;
GLuint vao;
GLint attribute_texcoord, attribute_coord2d;
GLint uniform_mytexture, uniform_mvp;

struct {
	GLuint vbo;
	GLuint tex;
	mat4 mvp;
} background;

struct {
	GLuint vbo;
	GLuint tex;
	vec3 pos;
	mat4 mvp;
} player;


int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *glArea;

	gtk_init(&argc, &argv);

	// Initialize Window

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Invaders Tutorial");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Initialize GTK GL Area

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);

	// Show widgets

	gtk_widget_show_all(window);
	gtk_main();

	return 0;

}

static void on_realize(GtkGLArea *area) {

	// Debug Message

	g_print("on realize     0\n");

	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "Unknown error\n");
		return;
	}

	const GLubyte *renderer = glGetString(GL_RENDER);
	const GLubyte *version = glGetString(GL_VERSION);

	g_print("Renderer: %s\n", renderer);
	g_print("OpenGL version supported %s\n", version);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Background Sprite

	GLfloat background_vertices[] = {
		0.0, 0.0, 0.0, 0.0, 
		0.0, HEIGHT, 0.0, 1.0,
		WIDTH, HEIGHT, 1.0, 1.0,
		
		0.0, 0.0, 0.0, 0.0,
		WIDTH, HEIGHT, 1.0, 1.0,
		WIDTH, 0.0, 1.0, 0.0
	};
	
	glGenBuffers(1, &background.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, background.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(background_vertices),
		background_vertices,
		GL_STATIC_DRAW
	);
	
	mat4_identity(background.mvp);
	background.tex = shader_load_texture("sprites/background.png");
	
	// Player Sprite

	GLfloat player_vertices[] = {
		-24.0, -24.0, 0.0, 0.0, 
		-24.0,  24.0, 0.0, 1.0,
		 24.0,  24.0, 1.0, 1.0,
		
		-24.0, -24.0, 0.0, 0.0,
		 24.0,  24.0, 1.0, 1.0,
		 24.0, -24.0, 1.0, 0.0
	};
	
	glGenBuffers(1, &player.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, player.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(player_vertices),
		player_vertices,
		GL_STATIC_DRAW
	);
	
	player.pos[0] = WIDTH / 2;
	player.pos[1] = 52.0;
	player.pos[2] = 0.0;
	mat4_translate(player.pos, player.mvp);
	player.tex = shader_load_texture("sprites/player.png");

	GLint compile_ok = GL_FALSE;
	GLint link_ok = GL_FALSE;

	const char *vs = "shader/vertex.glsl";
	const char *fs = "shader/fragment.glsl";
	program = shader_load_program(vs, fs);

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	attribute_name = "uv_coord";
	attribute_texcoord = glGetAttribLocation(program, attribute_name);
	if(attribute_texcoord == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	const char *uniform_name = "orthograph";
	GLint uniform_ortho = glGetUniformLocation(program, uniform_name);
	if(uniform_ortho == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if(uniform_mvp == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	uniform_name = "mytexture";
	uniform_mytexture = glGetUniformLocation(program, uniform_name);
	if(uniform_mytexture == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	glUseProgram(program);

	mat4 orthograph;
	mat4_orthagonal(WIDTH, HEIGHT, orthograph);
	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, orthograph);

}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

	g_print("on render\n");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	glBindVertexArray(vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, background.tex);
	glUniform1i(uniform_mytexture, 0);

	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, background.mvp);

	glEnableVertexAttribArray(attribute_coord2d);
	glEnableVertexAttribArray(attribute_texcoord);

	glBindBuffer(GL_ARRAY_BUFFER, background.vbo);
	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		0
	);

	glVertexAttribPointer(
		attribute_texcoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		(void*)(sizeof(float) * 2)
	);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, player.tex);
	glUniform1i(uniform_mytexture, 0);

	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, player.mvp);

	glBindBuffer(GL_ARRAY_BUFFER, player.vbo);
	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		0
	);

	glVertexAttribPointer(
		attribute_texcoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		(void*)(sizeof(float) * 2)
	);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribute_coord2d);
	glDisableVertexAttribArray(attribute_texcoord);

}
