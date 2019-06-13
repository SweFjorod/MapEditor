#pragma once

#include "olcPixelGameEngine.h"
#include <vector>

namespace olc {
	class SprConverter {
	public:
		static olc::Sprite *convertBlownUp(std::string path, size_t pixelSize) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			loadFont();
			Sprite *converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (int x = 0; x < spr.width; x++) {
				for (int y = 0; y < spr.height; y++) {
					Pixel fg = idToColour(spr.GetColour(x, y) & 0xf);
					Pixel bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
					uint64_t glyph = characters[ids[spr.GetGlyph(x, y)]];
					for (unsigned int dx = 0; dx < pixelSize; dx++) {
						for (unsigned int dy = 0; dy < pixelSize; dy++) {
							int sampleX = 8 * dx / pixelSize;
							int sampleY = 8 * dy / pixelSize;
							int samplePos = sampleY * 8 + sampleX;
							converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, glyph & (1 << samplePos) ? fg : bg);
						}
					}
				}
			}
			return converted;
		}

		static olc::Sprite *convertFilledPixels(std::string path, size_t pixelSize, bool fg) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			Sprite *converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (int x = 0; x < spr.width; x++) {
				for (int y = 0; y < spr.height; y++) {
					for (unsigned int dx = 0; dx < pixelSize; dx++) {
						for (unsigned int dy = 0; dy < pixelSize; dy++) {
							if (fg)
								converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, idToColour(spr.GetColour(x, y) & 0xf));
							else
								converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, idToColour((spr.GetColour(x, y) & 0xf0) >> 4));
						}
					}
				}
			}
			return converted;
		}

		static olc::Sprite *convertDumbBlendedPixels(std::string path, size_t pixelSize) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			Sprite *converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (int x = 0; x < spr.width; x++) {
				for (int y = 0; y < spr.height; y++) {
					Pixel fg = idToColour(spr.GetColour(x, y) & 0xf);
					Pixel bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
					fg.r = (fg.r + bg.r) / 2;
					fg.g = (fg.g + bg.g) / 2;
					fg.b = (fg.b + bg.b) / 2;
					for (unsigned int dx = 0; dx < pixelSize; dx++) {
						for (unsigned int dy = 0; dy < pixelSize; dy++) {
							converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, fg);
						}
					}
				}
			}
			return converted;
		}

		static olc::Sprite *convertSmartBlendedPixels(std::string path, size_t pixelSize) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			loadFont();
			Sprite *converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (int x = 0; x < spr.width; x++) {
				for (int y = 0; y < spr.height; y++) {
					Pixel fg = idToColour(spr.GetColour(x, y) & 0xf);
					Pixel bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
					short glyph = spr.GetGlyph(x, y);
					float percent = 0;
					for (int i = 0; i < 64; i++) {
						if ((characters[ids[glyph]] >> i) & 1)
							percent++;
					}
					percent /= 63;
					Pixel c = { (uint8_t)(fg.r * percent + bg.r * (1 - percent)),
						(uint8_t)(fg.g * percent + bg.g * (1 - percent)),
						(uint8_t)(fg.b * percent + bg.b * (1 - percent)),
						255 };
					for (unsigned int dx = 0; dx < pixelSize; dx++) {
						for (unsigned int dy = 0; dy < pixelSize; dy++) {
							converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, c);
						}
					}
				}
			}
			return converted;
		}

	private:
		struct CGESprite {
			int width, height;
			short *glyphs = nullptr;
			short *colours = nullptr;
			CGESprite(std::string sFile)
			{
				delete[] glyphs;
				delete[] colours;
				width = 0;
				height = 0;

				FILE *f = nullptr;
				fopen_s(&f, sFile.c_str(), "rb");
				if (f == nullptr)
					return;

				std::fread(&width, sizeof(int), 1, f);
				std::fread(&height, sizeof(int), 1, f);

				glyphs = new short[width * height];
				colours = new short[width * height];

				std::fread(colours, sizeof(short), width * height, f);
				std::fread(glyphs, sizeof(short), width * height, f);

				std::fclose(f);
			}
			~CGESprite() {
				delete[] glyphs;
				delete[] colours;
			}

			short GetGlyph(int x, int y)
			{
				if (x < 0 || x >= width || y < 0 || y >= height)
					return L' ';
				else
					return glyphs[y * width + x];
			}

			short GetColour(int x, int y)
			{
				if (x < 0 || x >= width || y < 0 || y >= height)
					return 0;
				else
					return colours[y * width + x];
			}
		};

		static Pixel idToColour(char id) {
			const Pixel colours[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_CYAN, DARK_RED, DARK_MAGENTA, DARK_YELLOW, GREY,
				DARK_GREY, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE };
			return colours[id];
		}

		static void loadFont() {
			if (characters.size() != 0)
				return;
			characters.reserve(688);
			ids.reserve(256 * 128);
			std::ifstream file;
			file.open("font.pgex", std::ios::binary);
			if (!file.is_open())
				throw std::exception("Couldn't open font file!");
			for (int i = 0; i < 668; i++) {
				uint64_t character = 0;
				file.read((char *)&character, 8);
				characters.push_back(character);
			}
			for (int i = 0; i < 256 * 128; i++) {
				uint16_t id = 0;
				file.read((char *)&id, 2);
				ids.push_back(id);
			}
			file.close();
		}
		static std::vector<uint64_t> characters;
		static std::vector<uint16_t> ids;
	};

	std::vector<uint64_t> SprConverter::characters;
	std::vector<uint16_t> SprConverter::ids;
}