#include <stdlib.h>
#include <GL/glew.h>
#include <gtk/gtk.h>
#include <math.h>
#include "DashGL/dashgl.h"

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *context);
static gboolean on_idle(gpointer data);
static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event);
static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event);

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
	gboolean left;
	gboolean right;
	int num_bullets;
	vec3 *bullets;
	gboolean *active;
	gboolean space;
} player;

struct {
	GLuint vbo;
	GLuint tex;
	mat4 mvp;
	GLfloat dy;
} bullet;

struct {
	GLuint vbo;
	GLuint tex;
	mat4 mvp;
	vec3 *pos;
	int num_enemies;
	float dx, dy;
	gboolean *active;
} enemy;

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
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_keydown), NULL);
	g_signal_connect(window, "key-release-event", G_CALLBACK(on_keyup), NULL);

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

	glewExperimental = GL_TRUE;
	glewInit();

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
		-22.0, -22.0, 0.0, 0.0, 
		-22.0,  22.0, 0.0, 1.0,
		 22.0,  22.0, 1.0, 1.0,
		
		-22.0, -22.0, 0.0, 0.0,
		 22.0,  22.0, 1.0, 1.0,
		 22.0, -22.0, 1.0, 0.0
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
	player.pos[1] = 30.0f;
	player.pos[2] = 0.0;
	player.left = FALSE;
	player.right = FALSE;
	
	mat4_translate(player.pos, player.mvp);
	player.tex = shader_load_texture("sprites/player.png");
	
	player.space = FALSE;
	player.num_bullets = 3;
	player.bullets = malloc(player.num_bullets * sizeof(vec3));
	player.active = malloc(player.num_bullets * sizeof(gboolean));

	int i;
	for(i = 0; i < player.num_bullets; i++) {
		player.active[i] = FALSE;
	}

	// Enemy Sprite

	GLfloat enemy_vertices[] = {
		-32.0, -32.0, 0.0, 0.0,
		-32.0,  32.0, 0.0, 1.0,
		 32.0,  32.0, 1.0, 1.0,

		-32.0, -32.0, 0.0, 0.0,
		 32.0,  32.0, 1.0, 1.0,
		 32.0, -32.0, 1.0, 0.0
	};

	glGenBuffers(1, &enemy.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, enemy.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(enemy_vertices),
		enemy_vertices,
		GL_STATIC_DRAW
	);

	enemy.tex = shader_load_texture("sprites/enemy.png");
	enemy.num_enemies = 10;
	enemy.pos = malloc(enemy.num_enemies * sizeof(vec3));
	enemy.active = malloc(enemy.num_enemies * sizeof(gboolean));
	
	enemy.dx = 3.0f;
	enemy.dy = -10.0f;

	float x = 70.0f;
	float y = HEIGHT - 140.0f;

	for(i = 0; i < enemy.num_enemies; i++) {
		enemy.pos[i][0] = x;
		enemy.pos[i][1] = y;
		enemy.pos[i][2] = 0.0f;
		enemy.active[i] = TRUE;

		x += 90.0f;

		if(i == 4) {
			x = 70.0f;
			y += 90.0f;
		}
	}

	// Bullet Sprite

	GLfloat bullet_vertices[] = {
		-6.0, -12.0, 0.0, 0.0, 
		-6.0,  12.0, 0.0, 1.0,
		 6.0,  12.0, 1.0, 1.0,
		
		-6.0, -12.0, 0.0, 0.0,
		 6.0,  12.0, 1.0, 1.0,
		 6.0, -12.0, 1.0, 0.0
	};
	
	glGenBuffers(1, &bullet.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bullet.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(bullet_vertices),
		bullet_vertices,
		GL_STATIC_DRAW
	);
	
	bullet.tex = shader_load_texture("sprites/bullet.png");
	bullet.dy = 10.0f;

	// Compile Shader Program

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

	g_timeout_add(20, on_idle, (void*)area);

}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

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

	mat4_translate(player.pos, player.mvp);
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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bullet.tex);
	glUniform1i(uniform_mytexture, 0);

	int i;
	for(i = 0; i < player.num_bullets; i++) {
		
		if(!player.active[i]) {
			continue;
		}

		mat4_translate(player.bullets[i], bullet.mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, bullet.mvp);

		glBindBuffer(GL_ARRAY_BUFFER, bullet.vbo);
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

	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, enemy.tex);
	glUniform1i(uniform_mytexture, 0);

	for(i = 0; i < enemy.num_enemies; i++) {

		if(!enemy.active[i]) {
			continue;
		}

		mat4_translate(enemy.pos[i], enemy.mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, enemy.mvp);

		glBindBuffer(GL_ARRAY_BUFFER, enemy.vbo);
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
	}

	glDisableVertexAttribArray(attribute_coord2d);
	glDisableVertexAttribArray(attribute_texcoord);

}

static gboolean on_idle(gpointer data) {

	if(player.right) {
		player.pos[0] += 6.0f;
	}

	if(player.left) {
		player.pos[0] -= 6.0f;
	}

	if(player.pos[0] < 0) {
		player.pos[0] = 0.0f;
	} else if(player.pos[0] > WIDTH) {
		player.pos[0] = WIDTH;
	}

	int i, k;
	for(i = 0; i < player.num_bullets; i++) {
		if(!player.active[i]) {
			continue;
		}
		
		player.bullets[i][1] += bullet.dy;

		if(player.bullets[i][1] < HEIGHT + 12.0f) {
			continue;
		}
		
		player.active[i] = FALSE;

	}
	
	gboolean do_switch = FALSE;

	for(i = 0; i < enemy.num_enemies; i++) {
		
		if(!enemy.active[i]) {
			continue;
		}

		enemy.pos[i][0] += enemy.dx;

		if(enemy.pos[i][0] > WIDTH || enemy.pos[i][0] < 0) {
			do_switch = TRUE;
		}

	}

	if(do_switch) {
		
		enemy.dx = -enemy.dx;

		for(i = 0; i < enemy.num_enemies; i++) {
			enemy.pos[i][1] += enemy.dy;
		}

	}

	float x_dif, y_dif, hypo, radius;

	for(i = 0; i < player.num_bullets; i++) {

		if(!player.active[i]) {
			continue;
		}

		for(k = 0; k < enemy.num_enemies; k++) {
		
			if(!enemy.active[k]) {
				continue;
			}
			
			x_dif = player.bullets[i][0] - enemy.pos[k][0];
			y_dif = player.bullets[i][1] - enemy.pos[k][1];
			
			hypo = pow(x_dif, 2) + pow(y_dif, 2);

			if(hypotf(x_dif, y_dif) > 32.0f) {
				continue;
			}
			
			enemy.dx *= 1.05;

			player.active[i] = FALSE;
			enemy.active[k] = FALSE;

			break;
		}

	}

	gtk_widget_queue_draw(GTK_WIDGET(data));
	return TRUE;

}

static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
		case GDK_KEY_Left:
			player.left = TRUE;
		break;
		case GDK_KEY_Right:
			player.right = TRUE;
		break;
		case GDK_KEY_space:
			
			if(!player.space) {
				int i;
				for(i = 0; i < player.num_bullets; i++) {
					if(player.active[i]) {
						continue;
					}
					
					player.active[i] = TRUE;
					player.bullets[i][0] = player.pos[0];
					player.bullets[i][1] = player.pos[1];
					player.bullets[i][2] = player.pos[2];
					break;
				}
			}

			player.space = TRUE;
		break;
	}

}

static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
		case GDK_KEY_Left:
			player.left = FALSE;
		break;
		case GDK_KEY_Right:
			player.right = FALSE;
		break;
		case GDK_KEY_space:
			player.space = FALSE;
		break;
	}

}
