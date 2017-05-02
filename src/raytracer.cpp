#include "raytracer.h"
#include "glm/ext.hpp"
#include "objloader.h"
#include <glm/glm.hpp>
#include <iostream>
#include <unistd.h>

Raytracer::Raytracer (ProgramOptions& po_) :
	_antialiaser(po_.antialiasing),
	_camera(glm::vec3(1.8,1.2,2), glm::vec3(0,0,0), 90.0, (float)po_.image_width / (float)po_.image_height),
	_sun(glm::vec3(-0.7, -1.5, -1.2)) {
	// Options
	po = po_;

	// Create SLD window
	_window = SDL_CreateWindow(
		"Raytracer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		po.image_width,
		po.image_height,
		0
	);

	// Check that the window was successfully created
	if (_window == NULL) {
		printf("Could not create window: %s\n", SDL_GetError());
		return;
	}

	// Renderer
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0);
	SDL_RenderClear(_renderer);

	_image = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
								po.image_width, po.image_height);
	_box = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
								po.target_size, po.target_size);
	_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

	// Create a box
	SDL_SetRenderTarget(_renderer, _box);
	SDL_SetRenderDrawColor(_renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(_renderer, 0, 0, po.target_size-1, 0);
	SDL_RenderDrawLine(_renderer, 0, 0, 0, po.target_size-1);
	SDL_RenderDrawLine(_renderer, po.target_size-1, 0, po.target_size-1, po.target_size-1);
	SDL_RenderDrawLine(_renderer, po.target_size-1, po.target_size-1, 0, po.target_size-1);
	SDL_RenderPresent(_renderer);
	SDL_SetRenderTarget(_renderer, NULL);

	// Set scene
	objLoader("scene/cube.obj", _cube);

}

void Raytracer::computeImage () {
	SDL_Event event;
	bool isRunning = true;
	bool hasFinished = false;

	// Coordinates
	int x = 0;
	int y = 0;
	SDL_Rect box_rect;
	box_rect.w = po.target_size;
	box_rect.h = po.target_size;

	// Main loop
	while (isRunning) {
		// Events
		while(SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				isRunning = false;
			}

			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
				isRunning = false;
			}
		}

		if (!hasFinished) {
			// Render a zone
			traceZone(x, y);
			x += po.target_size;
			if (x >= po.image_width) {
				x = 0;
				y += po.target_size;
			}
			if (y >= po.image_height) {
				hasFinished = false;
			}

			// Draw a box
			box_rect.x = x;
			box_rect.y = y;
		}

		// Render to screen
		SDL_RenderCopy(_renderer, _image, NULL, NULL);
		SDL_RenderCopy(_renderer, _box, NULL, &box_rect);
		SDL_RenderPresent(_renderer);
	}
}

// > saveImage
//		Write the screen content to a image file
void Raytracer::saveImage (const char* file_name) {
	SDL_Surface *sshot = SDL_CreateRGBSurface(0, po.image_width, po.image_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	SDL_RenderReadPixels(_renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
	SDL_SaveBMP(sshot, file_name);
	SDL_FreeSurface(sshot);
}

// > traceZone
//		Compute a particlar square (X, Y) of the image
void Raytracer::traceZone (int X, int Y) {
	// usleep(3000); // TODO

	void *tmp;
	Uint32 *pixels;
	int pitch;

	Face F(glm::vec3(0,0,0), glm::vec3(1,0,0), glm::vec3(0,1,0));

	// Lock
	SDL_LockTexture(_image, NULL, &tmp, &pitch);
	pixels = (Uint32 *)tmp;

	Uint8 s;
	int sx = std::min(po.target_size, po.image_width - X);
	int sy = std::min(po.target_size, po.image_height - Y);
	// For all pixels
	for (int i = 0; i < sx; i++) {
		for (int j = 0; j < sy; j++) {
			_antialiaser.resetPixelValue();
			// For all samples
			for (glm::vec2 samp : _antialiaser.getPixelSamplesRepartition()) {
				// Get the ray coming from the camera
				float x = (float)(X + i + samp.x) / (float)po.image_width - 0.5;
				float y = (float)(Y + j + samp.y) / (float)po.image_height - 0.5;
				glm::vec3 ray = _camera.getRay(x, y);

				// Test if in triangle
				s = 0;
				for (Face f : _cube) {
					s |= f.isRayThrough(ray, _camera.getPosition());
				}
				s *= 255;

				// Add color to sampling process
				_antialiaser.setSampleValue ({s,s,s,255});
			}

			// Set pixel color
			color_t color = _antialiaser.getPixelValue();
			pixels[(Y + j) * po.image_width + (X + i)] = SDL_MapRGBA(_format,
																	(Uint8)color.x,
																	(Uint8)color.y,
																	(Uint8)color.z,
																	(Uint8)color.t);
		}
	}

	// Unlock
	SDL_UnlockTexture(_image);
}

Raytracer::~Raytracer () {
	// Layout
	SDL_FreeFormat(_format);
	SDL_DestroyTexture(_image);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
}
