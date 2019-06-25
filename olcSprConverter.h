#pragma once

#include "olcPixelGameEngine.h"
#include <vector>

namespace olc {
	class SprConverter {
	public:
		static olc::Sprite *convertBlownUp(const std::string& path, const size_t pixelSize) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			loadFont();
			auto converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (auto x = 0; x < spr.width; x++) {
				for (auto y = 0; y < spr.height; y++) {
					const auto fg = idToColour(spr.GetColour(x, y) & 0xf);
					const auto bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
					const auto glyph = characters[ids[spr.GetGlyph(x, y)]];
					for (unsigned int dx = 0; dx < pixelSize; dx++) {
						for (unsigned int dy = 0; dy < pixelSize; dy++) {
							const int sampleX = dx * 8 / pixelSize;
							const int sampleY = dy * 8 / pixelSize;
							const auto samplePos = sampleY * 8 + sampleX;
							if (glyph & 1 << samplePos) converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, fg);
							else converted->SetPixel(x * pixelSize + dx, y * pixelSize + dy, bg);
						}
					}
				}
			}
			return converted;
		}

		static olc::Sprite *convertFilledPixels(const std::string& path, size_t pixelSize, bool fg) {
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

		static olc::Sprite *convertDumbBlendedPixels(const std::string& path, const size_t pixelSize) {
			CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			auto converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (auto x = 0; x < spr.width; x++) {
				for (auto y = 0; y < spr.height; y++) {
					auto fg = idToColour(spr.GetColour(x, y) & 0xf);
					const auto bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
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

		static olc::Sprite *convertSmartBlendedPixels(const std::string& path, size_t pixelSize) {
			const CGESprite spr(path);
			if (spr.width == 0)
				return nullptr;
			loadFont();
			auto converted = new Sprite(spr.width * pixelSize, spr.height * pixelSize);
			for (auto x = 0; x < spr.width; x++) {
				for (auto y = 0; y < spr.height; y++) {
					const auto fg = idToColour(spr.GetColour(x, y) & 0xf);
					const auto bg = idToColour((spr.GetColour(x, y) & 0xf0) >> 4);
					const auto glyph = spr.GetGlyph(x, y);
					float percent = 0;
					for (auto i = 0; i < 64; i++) {
						if (characters[ids[glyph]] >> i & 1)
							percent++;
					}
					percent /= 63;
					const Pixel c = { static_cast<uint8_t>(fg.r * percent + bg.r * (1 - percent)),
						static_cast<uint8_t>(fg.g * percent + bg.g * (1 - percent)),
						static_cast<uint8_t>(fg.b * percent + bg.b * (1 - percent)),
						static_cast<uint8_t>(glyph != ' ' ? 255 : 0) };
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
			CGESprite(const std::string& sFile)
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

			short GetGlyph(int x, int y) const
			{
				if (x < 0 || x >= width || y < 0 || y >= height)
					return ' ';
				else
					return glyphs[y * width + x];
			}

			short GetColour(int x, int y) const
			{
				if (x < 0 || x >= width || y < 0 || y >= height)
					return 0;
				else
					return colours[y * width + x];
			}
		};

		static std::string ReturnexePath() {
			char result[MAX_PATH];
			return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
		}

		static std::string GetexePath() {
			auto _newpath = ReturnexePath();
			const auto _pos = _newpath.rfind("\\");
			if (_pos != std::string::npos)
				_newpath.erase(_pos + 1);
			return _newpath;
		}

		static Pixel idToColour(const char id) {
			const Pixel colours[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_CYAN, DARK_RED, DARK_MAGENTA, DARK_YELLOW, GREY,
				DARK_GREY, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE };
			return colours[id];
		}

		static void loadFont() {
			if (!characters.empty())
				return;
			characters.reserve(688);
			ids.reserve(256 * 128);
			std::ifstream file;
			const auto _path = GetexePath();
			file.open(_path + "font.pgex", std::ios::binary);
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