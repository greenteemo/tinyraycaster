#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

# define M_PI 3.14159265358979323846  /* pi */

using namespace std;

uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a=255) {
    /*
     * 31-24 : a
     * 23-16 : b
     * 15- 8 : g
     * 7 - 0 : r
     */
    return (a<<24) + (b<<16) + (g<<8) + r;
}

void unpack_color(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
    r = (color >>  0) & 255;
    g = (color >>  8) & 255;
    b = (color >> 16) & 255;
    a = (color >> 24) & 255;
}

void drop_ppm_image(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h) {
    assert(image.size() == w*h);
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for (size_t i = 0; i < h*w; ++i) {
        uint8_t r, g, b, a;
        unpack_color(image[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
    }
    ofs.close();
}

void draw_block(std::vector<uint32_t> &image, const size_t img_w, const size_t img_h, const size_t img_x, const size_t img_y, const size_t block_w, const size_t block_h, const uint32_t color){
    assert(image.size() == img_h * img_w);
    for(int i = 0; i < block_h; i++){
        for(int j = 0; j < block_w; j++){
            size_t x = img_x + i;
            size_t y = img_y + j;
            if(x >= img_h || y >= img_w)  continue;
            image[x * img_w + y] = color;
        }
    }
}


int main() {
    const size_t img_w = 1024; // image width
    const size_t img_h = 512; // image height
    std::vector<uint32_t> framebuffer(img_w*img_h, pack_color(211, 211, 211)); // the image itself, initialized to lightgray - #d3d3d3  rgb(211,211,211)


    // draw the wall of map
    const size_t map_w = 16;
    const size_t map_h = 16;
    /*
     * x  ^
     *    '
     *    '
     *    '
     *    --->  y 
     */
    const char map[] = "----------------"\
                       "-              -"\
                       "-              -"\
                       "-   -----      -"\
                       "-     -        -"\
                       "-     -        -"\
                       "-     - ----   -"\
                       "-     -    -   -"\
                       "-     -    -   -"\
                       "-     -    -   -"\
                       "-   ---    -   -"\
                       "-          -   -"\
                       "-          -   -"\
                       "-              -"\
                       "-              -"\
                       "----------------";
    assert(sizeof(map) == map_h * map_w + 1);  // +1 for the null terminated string

    const size_t block_w = img_w / (map_w * 2);
    const size_t block_h = img_h / map_h;
    for(int i = 0; i < map_h; i++){
        for(int j = 0; j < map_w; j++){
            if(map[i * map_w + j] != ' '){
                size_t img_x = i * block_h;
                size_t img_y = j * block_w;
                draw_block(framebuffer, img_w, img_h, img_x, img_y, block_w, block_h, pack_color(0, 0, 0));
            }
        }
    }

    // player location
    float player_x = 3.5;
    float player_y = 2.5;
    float player_a = 1.3;
    const float fov = M_PI / 3;   // field of view
    // draw player location
    draw_block(framebuffer, img_w, img_h, player_x * block_h, player_y * block_w, 5, 5, pack_color(255, 0, 0));
    // draw player view
    player_x += 0.08;
    player_y += 0.08;

    for(int frame = 0; frame < 10; frame++){
        player_a += 2 * M_PI / 360;
        vector<uint32_t> framebuffer_copy = framebuffer;

        for(int i = 0; i < img_w / 2; i++){
            float angle = player_a - fov/2 + fov * i / (img_w/2);
            for(float t = 0; t < 16; t+=0.1){
                float map_x = player_x + t * sin(angle);
                float map_y = player_y + t * cos(angle);

                if(map[int(map_y) + int(map_x) * map_w] == '-'){
                    size_t height = img_h / (t * cos(angle - player_a));
                    draw_block(framebuffer_copy, img_w, img_h, img_h/2-height/2, img_w/2+i, 1, height, pack_color(0, 0, 0));
                    break;
                }  
                
                size_t x = map_x * block_h;
                size_t y = map_y * block_w;
                if(framebuffer_copy[y + x * img_w] == pack_color(211, 211, 211))  // no line on the red player location symbol
                    framebuffer_copy[y + x * img_w] = pack_color(255, 255, 255);
            }
        }
        std::stringstream ss;
        ss << "./map/map" << frame << ".ppm";
        drop_ppm_image(ss.str(), framebuffer_copy, img_w, img_h);
    }

    return 0;
}