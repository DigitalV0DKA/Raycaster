/*
 * ray_caster.cc
 *
 *
 * Simple Raycaster
 *
 *
 * Copyright (C) 2013  Bryant Moscon - bmoscon@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution, and in the same 
 *    place and form as other copyright, license and disclaimer information.
 *
 * 3. The end-user documentation included with the redistribution, if any, must 
 *    include the following acknowledgment: "This product includes software 
 *    developed by Bryant Moscon (http://www.bryantmoscon.org/)", in the same 
 *    place and form as other third-party acknowledgments. Alternately, this 
 *    acknowledgment may appear in the software itself, in the same form and 
 *    location as other such third-party acknowledgments.
 *
 * 4. Except as contained in this notice, the name of the author, Bryant Moscon,
 *    shall not be used in advertising or otherwise to promote the sale, use or 
 *    other dealings in this Software without prior written authorization from 
 *    the author.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 */


#include <cmath>

#include "api_interface.h"


// need to write a function to load a world from a file
// for now, just define one

#define world_height 16
#define world_width 16

int world[16][16] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// todo: read window properties from file or from command line
#define window_height 600
#define window_width 800

// enforced frame rate 60 fps max (17 ms per frame is about 60 fps)
#define min_frame_time 17

typedef struct key_state_t {
    bool w;
    bool a;
    bool s;
    bool d;
    bool exit; 
} key_state_t;

typedef struct raycaster_t {
  // player starting location
  double player_x;
  double player_y;
  // direction vector
  double player_dir_x;
  double player_dir_y;
  // camera line vector. these values are added and subtracted from the
  // directon vector to get the actual line segment
  // note: this must be perpendicular to the player
  // direction vector
  // 
  // This also sets up the FOV. FOV is determined by ratio between direction 
  // vector and the camera vector. If ratio is 1:1, FOV will be 90 degrees
  // the angle is 2 * arctangent(camera / direction). Example:
  // 2 * arctan(1.0 / 1.0) = 2 * 45 = 90
  double camera_x;
  double camera_y;
  
  key_state_t key_state;
  
  // how long the current frame took. used to 'sleep' at end of frame
  uint64_t frame_time;
 
} raycaster_t;


static void draw_screen(raycaster_t *data)
{
  // for each column in the window
  for (uint32_t column = 0; column < 800; ++column) {
    // update ray position from player position
    double ray_x = data->player_x;
    double ray_y = data->player_y;
    // normalize camera. convert screen coordinate to camera coordinate (from -1 to 1)
    double camera = 2 * column / double(window_width) - 1;
    // calculate direction of ray. since camera is normalized, this will be too
    double ray_dir_x = data->camera_x * camera + data->player_dir_x;
    double ray_dir_y = data->camera_y * camera + data->player_dir_y;
    
    //convert to world (map) coordinates 
    int world_x = ray_x;
    int world_y = ray_y;
    
    // distances
    double x_dist, y_dist, d_x_dist, d_y_dist, p_wall_dist;
    // delta x and delta y
    int d_x, d_y;
    // is this a wall parallel to the y-axis?
    bool parallel;

    // for drawing the line
    int line_len, line_start, line_end;

    color_t color;
    
    // calculate distance from respective side to respective side
    // You can also do this with angles, but its easier to think about like this.
    // the distance to the next X side, or Y side, is a ratio of the directions. 
    // its easy to visualize if you think about it like this. If you are a ray going
    // at 89 degrees, how long is it going to take to move over 1 unit in the Y direction? 
    // and how long in the X direction? in the X direction, its clearly going to be
    // a large number, and in the y direction its going to be pretty small. You can use the
    // ratio of these 'directions' in conjunction with the formula for calculating a
    // hypotenuse of a right triangle to fine the distance, in terms of squares on the map
    d_x_dist = sqrt((ray_dir_y / ray_dir_x) * (ray_dir_y / ray_dir_x) + 1);
    d_y_dist = sqrt((ray_dir_x / ray_dir_y) * (ray_dir_x / ray_dir_y) + 1);

    // determine the stepping (+1 or -1) based on direction
    // and find the distance to the next 'square' in the map
    // this is for the x-component of the ray direction
    if (ray_dir_x < 0) {
      d_x = -1;
      x_dist = (ray_x - world_x) * d_x_dist;
    } else {
      d_x = 1;
      x_dist = (world_x + 1 - ray_x) * d_x_dist;
    }
    
    // now do the same for the y component
    if (ray_dir_y < 0) {
      d_y = -1;
      y_dist = (ray_y - world_y) * d_y_dist;
    } else {
      d_y = 1;
      y_dist = (world_y + 1 - ray_y) * d_y_dist;
    }

    // now use a Digital Differential Analyzer
    // see http://en.wikipedia.org/wiki/Digital_differential_analyzer_(graphics_algorithm)
    // 
    // basically we have a line end point, and we have a direction, and we want to 
    // find where this line hits a wall, so we use the DDA to find points a long this line and
    // we check for an intersection with a wall or other object in the world/map
    // 
    // we also keep track of what "side" we are going to hit, if its parallel to the camera start
    // or not. When the loop ends, we'll use this variable to set the coloration of the side

    do {
      if (x_dist > y_dist) {
	y_dist += d_y_dist;
	world_y += d_y;
	parallel = true;
      } else {
	x_dist += d_x_dist;
	world_x += d_x;
	parallel = false;

      }
      // we go until we hit a wall, in this case, we expect that anything non-zero is a hit
    } while (!world[world_x][world_y]);
    
    // set color based on what we "hit" with DDA
    if (world[world_x][world_y] == 1) {
      color.r = 255;
      color.g = 0;
      color.b = 0;
    }

    // give walls parallel to our initial camera direction a darker color/shading
    if (parallel) {
      color.r /= 2.5;
      color.g /= 2.5;
      color.b /= 2.5;
    }
    
    // since we know what side was hit, we can determine the exact distance from the camera to the 
    // wall. We need this to determine how high the wall needs to be drawn (really its how high a 
    // single line needs to be, since we are drawing one line at a time...)
    if (!parallel) { 
      // world_x - ray_x + (1 - d_x)/2 is the number of squares we have moved in the world. 
      // we might not be perpendicular, so we need to account for this by dividing
      // by the x component of the ray direction
      p_wall_dist = fabs((world_x - ray_x + (1 - d_x) / 2) / ray_dir_x);
    } else {
      // same for the Y side
      p_wall_dist = fabs((world_y - ray_y + (1 - d_y) / 2) / ray_dir_y);
    }

    // find line length to draw (i.e. height)
    // inverse of wall distance * window height - stuff far away is shorter
    // and stuff that is closer appears larger (perspective).
    line_len = std::abs(int(1.0 / p_wall_dist * window_height));
    
    //calculate start/end pixels of the line. all lines have to be centered at window_height / 2
    line_start = -line_len / 2 + window_height / 2;
    line_end = line_len / 2 + window_height / 2;
    
    // enforce min/max
    // without this, you seg fault :)
    // (i suppose i could add bound checking to the API interface...)
    if (line_start < 0) {
      line_start = 0;
    }
    
    if (line_end >= window_height) {
      line_end = window_height - 1;
    }
    
    //draw the vertical line we just calculated
    vertical_line(column, line_start, line_end, &color);
  }
  
  // done updating each column, draw it!
  refresh();
  clear_screen();
}

static void input(raycaster_t *data)
{
  events_e event = handle_events();
  
  switch (event) {
    case EXIT:
      data->key_state.exit = true;
      break;
    case KEY_W:
      data->key_state.w = !data->key_state.w;
      break;
    case KEY_A:
      data->key_state.a = !data->key_state.a;
      break;
    case KEY_S:
      data->key_state.s = !data->key_state.s;
      break;
    case KEY_D:
      data->key_state.d = !data->key_state.d;
      break;
    case NONE:
      break;
    default:
      // error message of some sort
      break;
  }
}

void player_move(raycaster_t *data)
{
  // speed based on enforced frame rate
  double d_move = min_frame_time / 300.0;
  double d_rot = min_frame_time / 300.0;
  double orig_x, orig_camera_x;

  // forward
  if (data->key_state.w) {
    if (!world[int(data->player_x + data->player_dir_x * d_move)][int(data->player_y)]) {
      data->player_x += data->player_dir_x * d_move;
    }
    
    if (!world[int(data->player_x)][int(data->player_y + data->player_dir_y * d_move)]) {
      data->player_y += data->player_dir_y * d_move;
    }
  }
  
  // backwards
  if (data->key_state.s) {
    if (!world[int(data->player_x - data->player_dir_x * d_move)][int(data->player_y)]) {
      data->player_x -= data->player_dir_x * d_move;
    }
    
    if (!world[int(data->player_x)][int(data->player_y - data->player_dir_y * d_move)]) {
      data->player_y -= data->player_dir_y * d_move;
    }
  }

  // rotation done with the standard rotation matrix
  //
  // [ cosine(a),   -sine(a) ]
  // [   sine(a), -cosine(a) ]
  //   
  // all we have to do is use the correct angle, or in this case, either a +d_rot, or a -d_rot
  // don't forget to rotate the camera too!

  // rotate right
  if (data->key_state.d) {
    orig_x = data->player_dir_x;
    data->player_dir_x = data->player_dir_x * cos(-d_rot) - data->player_dir_y * sin(-d_rot);
    data->player_dir_y = orig_x * sin(-d_rot) + data->player_dir_y * cos(-d_rot);
    orig_camera_x = data->camera_x;
    data->camera_x = data->camera_x * cos(-d_rot) - data->camera_y * sin(-d_rot);
    data->camera_y = orig_camera_x * sin(-d_rot) + data->camera_y * cos(-d_rot);
  }

  // rotate left
  if (data->key_state.a) {
    orig_x = data->player_dir_x;
    data->player_dir_x = data->player_dir_x * cos(d_rot) - data->player_dir_y * sin(d_rot);
    data->player_dir_y = orig_x * sin(d_rot) + data->player_dir_y * cos(d_rot);
    orig_camera_x = data->camera_x;
    data->camera_x = data->camera_x * cos(d_rot) - data->camera_y * sin(d_rot);
    data->camera_y = orig_camera_x * sin(d_rot) + data->camera_y * cos(d_rot);
  }
}


static bool window_close(raycaster_t *data)
{ 
  return (data->key_state.exit);
}

static void frame_start(raycaster_t *data)
{
  data->frame_time = get_ticks();
}

static void enforce_fps(raycaster_t *data)
{
  // calculate the time to sleep
  double frame_time = get_ticks() - data->frame_time;

  // if we are under max frame time, sleep!
  if (frame_time < min_frame_time) {
    sleep_ticks(min_frame_time - frame_time);
  } 
}


int main(int argc, char** argv)
{
  raycaster_t data = {8, 8, -1, 0, 0, 1, {false, false, false, false, false}};

  init();
  init_screen(window_width, window_height, "Simple Raycaster");
  

  while (!window_close(&data)) {
    frame_start(&data);
    draw_screen(&data);
    input(&data);
    player_move(&data);
    enforce_fps(&data);
  }
  
  deinit();
  return (0);
}
