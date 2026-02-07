#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Color
{
    unsigned char r, g, b, a;
};

void save_tga(const std::string &filename, int width, int height, const std::vector<Color> &pixels)
{
    std::ofstream ofs(filename, std::ios::binary);
    unsigned char header[18] = {0};
    header[2] = 2; // Uncompressed true-color
    header[12] = width & 0xFF;
    header[13] = (width >> 8) & 0xFF;
    header[14] = height & 0xFF;
    header[15] = (height >> 8) & 0xFF;
    header[16] = 32;   // Bits per pixel
    header[17] = 0x20; // Top-left origin

    ofs.write((char *)header, 18);
    for (const auto &p : pixels)
    {
        // TGA is BGRA
        ofs.put(p.b);
        ofs.put(p.g);
        ofs.put(p.r);
        ofs.put(p.a);
    }
}

void draw_rect(std::vector<Color> &pixels, int w, int h, int x, int y, int rw, int rh, Color c)
{
    for (int j = y; j < y + rh && j < h; ++j)
    {
        for (int i = x; i < x + rw && i < w; ++i)
        {
            pixels[j * w + i] = c;
        }
    }
}

void draw_pixel(std::vector<Color> &pixels, int w, int h, int x, int y, Color c)
{
    if (x >= 0 && x < w && y >= 0 && y < h)
    {
        pixels[y * w + x] = c;
    }
}

int main()
{
    const int W = 32, H = 32;
    Color teal = {0, 255, 200, 255};
    Color red = {255, 100, 100, 255};
    Color white = {200, 200, 200, 255};
    Color transparent = {0, 0, 0, 0};

    auto create_base = [&]() { return std::vector<Color>(W * H, transparent); };

    // Play
    auto play = create_base();
    for (int y = 8; y < 24; ++y)
    {
        int end_x = 8 + (y - 8);
        if (y > 16)
            end_x = 8 + (24 - y);
        for (int x = 8; x <= 8 + (y < 16 ? (y - 8) * 2 : (24 - y) * 2); ++x)
        {
            draw_pixel(play, W, H, x, y, teal);
        }
    }
    save_tga("assets/icons/play.tga", W, H, play);

    // Pause
    auto pause = create_base();
    draw_rect(pause, W, H, 8, 8, 5, 16, teal);
    draw_rect(pause, W, H, 19, 8, 5, 16, teal);
    save_tga("assets/icons/pause.tga", W, H, pause);

    // Next
    auto next = create_base();
    for (int y = 8; y < 24; ++y)
    {
        int len = (y < 16 ? (y - 8) : (24 - y - 1));
        draw_rect(next, W, H, 8, y, len + 1, 1, teal);
    }
    draw_rect(next, W, H, 22, 8, 4, 16, teal);
    save_tga("assets/icons/next.tga", W, H, next);

    // Prev
    auto prev = create_base();
    draw_rect(prev, W, H, 6, 8, 4, 16, teal);
    for (int y = 8; y < 24; ++y)
    {
        int len = (y < 16 ? (y - 8) : (24 - y - 1));
        draw_rect(prev, W, H, 24 - len, y, len + 1, 1, teal);
    }
    save_tga("assets/icons/prev.tga", W, H, prev);

    // Manual Pixel Art Heart for perfect shape
    auto draw_heart = [&](Color c, bool filled)
    {
        std::vector<Color> pixels(W * H, transparent);
        const char *pattern[] = {
            "   @@@@@@    @@@@@@   ", "  @@@@@@@@  @@@@@@@@  ", " @@@@@@@@@@@@@@@@@@@@ ", "@@@@@@@@@@@@@@@@@@@@@@",
            "@@@@@@@@@@@@@@@@@@@@@@", "@@@@@@@@@@@@@@@@@@@@@@", " @@@@@@@@@@@@@@@@@@@@ ", "  @@@@@@@@@@@@@@@@@@  ",
            "   @@@@@@@@@@@@@@@@   ", "    @@@@@@@@@@@@@@    ", "     @@@@@@@@@@@@     ", "      @@@@@@@@@@      ",
            "       @@@@@@@@       ", "        @@@@@@        ", "         @@@@         ", "          @@          "};
        int ph = 16;
        int pw = 22;
        int offsetX = (W - pw) / 2;
        int offsetY = (H - ph) / 2;

        for (int y = 0; y < ph; ++y)
        {
            for (int x = 0; x < pw; ++x)
            {
                if (pattern[y][x] == '@')
                {
                    bool is_border = false;
                    if (y == 0 || y == ph - 1 || x == 0 || x == pw - 1 || (y > 0 && pattern[y - 1][x] == ' ') ||
                        (y < ph - 1 && pattern[y + 1][x] == ' ') || (x > 0 && pattern[y][x - 1] == ' ') ||
                        (x < pw - 1 && pattern[y][x + 1] == ' '))
                    {
                        is_border = true;
                    }

                    if (filled)
                    {
                        draw_pixel(pixels, W, H, x + offsetX, y + offsetY, c);
                    }
                    else if (is_border)
                    {
                        draw_pixel(pixels, W, H, x + offsetX, y + offsetY, white);
                    }
                }
            }
        }
        return pixels;
    };

    auto heart_f_pix = draw_heart(red, true);
    save_tga("assets/icons/heart_filled.tga", W, H, heart_f_pix);

    auto heart_o_pix = draw_heart(white, true);
    save_tga("assets/icons/heart_outline.tga", W, H, heart_o_pix);

    // Base Loop Drawing Helper
    auto draw_loop_base = [&](std::vector<Color> &pixels, Color c)
    {
        // Square loop
        for (int y = 8; y < 24; ++y)
        {
            for (int x = 8; x < 24; ++x)
            {
                if (y == 8 || y == 23 || x == 8 || x == 23)
                {
                    draw_pixel(pixels, W, H, x, y, c);
                }
            }
        }
        // Arrow head at top-right
        draw_rect(pixels, W, H, 21, 6, 5, 2, c);      // Top part
        draw_rect(pixels, W, H, 24, 6, 2, 5, c);      // Side part
        draw_pixel(pixels, W, H, 23, 8, transparent); // Corner break
    };

    // Repeat OFF (Grey/White)
    auto rep_off = create_base();
    draw_loop_base(rep_off, {200, 200, 200, 255});
    save_tga("assets/icons/repeat_off.tga", W, H, rep_off);

    // Repeat ONE (Orange with '1')
    auto rep_one = create_base();
    Color orange = {255, 180, 0, 255};
    draw_loop_base(rep_one, orange);
    // Draw '1' in middle
    draw_rect(rep_one, W, H, 15, 12, 2, 8, orange);
    draw_pixel(rep_one, W, H, 14, 13, orange);
    draw_rect(rep_one, W, H, 14, 20, 4, 1, orange);
    save_tga("assets/icons/repeat_one.tga", W, H, rep_one);

    // Repeat ALL (Teal with 'A')
    auto rep_all = create_base();
    draw_loop_base(rep_all, teal);
    // Draw 'A' in middle
    draw_rect(rep_all, W, H, 14, 12, 4, 1, teal); // Top
    draw_rect(rep_all, W, H, 13, 13, 1, 8, teal); // Left
    draw_rect(rep_all, W, H, 18, 13, 1, 8, teal); // Right
    draw_rect(rep_all, W, H, 14, 16, 4, 1, teal); // Middle bar
    save_tga("assets/icons/repeat_all.tga", W, H, rep_all);

    return 0;
}
