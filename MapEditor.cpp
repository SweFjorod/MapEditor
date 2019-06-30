/* 
MapEditor.cpp

MapEditor for One Lone Coders RPG game by SweFjorod.

Thanks to One Lone Coder for the olcPixelGameEngine
Thanks to Gorbit99 for the olcSprConverter.h
Thanks to Lode Vandevenne for the lodepng so I could encode png files.
*/

#include "olcPixelGameEngine.h"
#include "olcSprConverter.h"
#include "lodepng.h"

#include <fstream>
#include <sstream>
#include <commdlg.h>
#include <iostream>
#include <filesystem>
#include <utility>

using namespace olc;
using namespace std;
using namespace std::experimental::filesystem;

constexpr auto tile_size = 16;

//  Path variables
string level_full_path;
path level_path;
path level_name;
string tilemap_full_path;
path tilemap_path;
path tilemap_name;
path tilemap_extension;
path tilemap_name_no_extension;

//static string GetExePath();
string ReturnExePath();

typedef struct Tilemap {
	explicit Tilemap(Sprite* sprite);
	~Tilemap();
    Sprite *spr;
	void SetSprite(Sprite* sprite);
};

typedef struct EditorResources {
	explicit EditorResources(string path);
	~EditorResources();
	Tilemap* tMap;
	void LoadTilemapFile(string path);
};

struct TileInfo {
	int tileNumber;
    bool solid;
};

static string GetExePath() {
	auto newpath = ReturnExePath();
	const auto pos = newpath.rfind('\\');
	if (pos != string::npos)
		newpath.erase(pos + 1);
	return newpath;
}

class MapEditor : public PixelGameEngine{
	EditorResources* res = nullptr;
	EditorResources* res_button = nullptr;

	int text_mode = 0;
	int input_number = 0;
	int width_sum = 0;
	int height_sum = 0;
	float old_time = 0.0f;
	float new_time = 0.0f;
	bool cursor = false;
	static const int canvas_width = 31;
	static const int canvas_height = 23;
	float f_camera_pos_x = 0.0f;
	float f_camera_pos_y = 0.0f;

	int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;

	int n_width = 0;
	int n_height = 0;

    int offsetx = 0;
    int offsety = 0;

    int amount_drawn = 0;

	string exe_path = GetExePath();

	TileInfo* map_info = {};
	TileInfo* canvas = {};

    Pixel * bg_col = nullptr;

public:
	static void OpenLevel(const string& type);
	static void SetPaths(const string& type, const string& file_name);
	void EncodeOneStep(const string& filename, vector<unsigned char>& image, unsigned width, unsigned height) const;
	void LoadConvert() const;
	void ResizeConvert() const;
	void ResizeMapInfo();
	void PopulateTileInfo();

    bool OnUserCreate() override {
        sAppName = "Map Editor";

        res = new EditorResources(tilemap_full_path);
		res_button = new EditorResources(exe_path + "edit-button.png");
        bg_col = new Pixel( { 25, 25, 25, 255 } );

        offsetx = -1;
        offsety = -1;

        amount_drawn = 0;

		n_width = canvas_width;
		n_height = canvas_height;

		PopulateTileInfo();

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Get mouse position
        auto mouseX = GetMouseX() / tile_size;
        auto mouseY = GetMouseY() / tile_size;
        auto actualMouseX = GetMouseX();
        auto actualMouseY = GetMouseY();
		auto _nWidthOffset = to_string(n_width).length() * 8;
		auto _widthSumOffset = to_string(width_sum).length() * 8;
		auto _heightSumOffset = to_string(height_sum).length() * 8;

        Clear(*bg_col);

        // Mouse button handling in canvas area
        if (mouseX <= n_width -1 && mouseY <= n_height -1) {
            // offset eyedropper
            if (GetKey(CTRL).bHeld && GetMouse(0).bHeld) {
	            auto index = mouseY * n_width + mouseX;                
                if (canvas[index].tileNumber != -1) {
	                auto idx = canvas[index].tileNumber;
	                auto sx = idx % 10;
	                auto sy = idx / 10;
                    offsetx = sx * tile_size;
                    offsety = sy * tile_size;
                    x1 = offsetx + 504;
                    x2 = tile_size - 1;
                    y1 = offsety + 40;
                    y2 = tile_size - 1;
                }
            }

            // Place tile
            else if (GetMouse(0).bHeld) {
                if (offsetx != -1 && offsety != -1) {
					auto index2 = mouseY * canvas_width + mouseX;
	                auto tileIndex = (offsety * res->tMap->spr->width / tile_size + offsetx) / tile_size;
					canvas[index2].tileNumber = tileIndex;
                    amount_drawn++;
                }
            }

            // Remove tile
            else if (GetMouse(1).bHeld) {
				auto index = mouseY * n_width + mouseX;
				auto index2 = mouseY * canvas_width + mouseX;
            	if (canvas[index].tileNumber != -1)
                    amount_drawn--;
				canvas[index2].tileNumber = -1;
				canvas[index2].solid = false;
            }

            // Turn solid state off
            else if (GetKey(SHIFT).bHeld && GetMouse(2).bHeld) {
	            auto index = mouseY * canvas_width + mouseX;
				canvas[index].solid = false;
            }

            // Turn solid state on
            else if (GetMouse(2).bHeld) {
	            auto index = mouseY * canvas_width + mouseX;
				canvas[index].solid = true;
            }
        }
/*
        auto nVisibleTilesX = canvas_width;
        auto nVisibleTilesY = canvas_height;

		// Calculate Top-Leftmost visible tile
        auto fOffsetX = f_camera_pos_x - float(nVisibleTilesX) / 2.0f;
        auto fOffsetY = f_camera_pos_y - float(nVisibleTilesY) / 2.0f;

		// Clamp camera to game boundaries
		if (fOffsetX < 0) fOffsetX = 0;
		if (fOffsetY < 0) fOffsetY = 0;
		if (fOffsetX > float(n_width) - nVisibleTilesX) fOffsetX = n_width - nVisibleTilesX;
		if (fOffsetY > float(n_height) - nVisibleTilesY) fOffsetY = n_height - nVisibleTilesY;

		// Get offsets for smooth movement
        auto fTileOffsetX = (fOffsetX - int(fOffsetX)) * tile_size;
        auto fTileOffsetY = (fOffsetY - int(fOffsetY)) * tile_size;
*/
		// Draw visible tile map
		if (amount_drawn > 0) {
			// Draw tiles to screen
			for (auto y = 0; y < canvas_height; y++) {
				for (auto x = 0; x < canvas_width; x++) {
					auto idx = canvas[y * canvas_width + x].tileNumber;
					auto sx = idx % 10;
					auto sy = idx / 10;
					if (canvas[y * canvas_width + x].tileNumber != -1) {
						DrawPartialSprite(x * tile_size, y * tile_size, res->tMap->spr, sx * tile_size, sy * tile_size, tile_size, tile_size);
						if (canvas[y * canvas_width + x].solid)
							DrawRect(x * tile_size, y * tile_size, tile_size - 1, tile_size - 1, RED);
					}
				}
			}
		}

        if (GetMouseX() / tile_size <= canvas_width -1 && GetMouseY() / tile_size <= canvas_height -1) {
            // Draw outline where mouse is
            DrawRect(mouseX * tile_size, mouseY * tile_size, tile_size - 1, tile_size - 1, YELLOW);

            // Draw mouse coords to screen
            DrawString(634, 395, "(" + to_string(mouseX) + "," + to_string(mouseY) + ")", WHITE, 1);
		}
        else {
            if (GetMouse(0).bPressed) {
                // Check if mouse is on top of tilemap
                if (GetMouseX() >= 504 && GetMouseX() < 504 + res->tMap->spr->width &&
                    GetMouseY() >= 40 && GetMouseY() < 40 + res->tMap->spr->height) {
					x1 = static_cast<int>(actualMouseX - 504) / tile_size * tile_size + 504;
                    x2 = tile_size - 1;
					y1 = static_cast<int>(actualMouseY - 40) / tile_size * tile_size + 40;
                    y2 = x2;

                    offsetx = static_cast<int>(actualMouseX - 504) / tile_size * tile_size;
                    offsety = static_cast<int>(actualMouseY - 40) / tile_size * tile_size;
                }

				if (GetMouseX() >= 616 + _nWidthOffset && GetMouseX() < 616 + _nWidthOffset + res_button->tMap->spr->width &&
					GetMouseY() >= 8 && GetMouseY() < 8 + res_button->tMap->spr->height) {
					width_sum = 0;
					height_sum = 0;
					input_number = 0;
					text_mode = 1;
				}
            }
        }

		// Draw Menu and help
		if (text_mode == 0) {
			DrawString(5, 375, "F2 = Save Tilemap F4 = Save As  F5 = Save  F8 = Load Level  F9 = Load Tilemap", WHITE, 1);
			DrawString(5, 395, "[LEFT CLICK] Draw   [RIGHT CLICK] Erase   [CTRL+LEFT CLICK] Select tile", WHITE, 1);
			DrawString(5, 405, "[MIDDLE CLICK] Set collision   [SHIFT+MIDDLE CLICK] Unset collision", WHITE, 1);

			SetPixelMode(Pixel::MASK);
			DrawSprite(616 + _nWidthOffset, 8, res_button->tMap->spr);
			SetPixelMode(Pixel::NORMAL);

		}
		// Draw simulated input field
		else {
			if (text_mode == 1) 
				DrawString(5, 375, "New Level Width: ");
			else 
				DrawString(5, 375, "New Level Height: ");;

			for (auto i = 0; i < 10; i++) {
				if (GetKey(static_cast<Key>(int(K0) + i)).bPressed) input_number += '0' + i;
			}

			if (text_mode != 0) {
				// Draw simulated cursor
				new_time = new_time + fElapsedTime;
				if (new_time > old_time) {
					cursor = !cursor;
					old_time = new_time + 0.5f;
				}

				if (cursor)
					if (text_mode == 1) 
						DrawString(width_sum > 0 ? 135 + _widthSumOffset : 135, 375, "_");
					else 
						DrawString(height_sum > 0 ? 145 + _heightSumOffset : 145, 375, "_");

				// Draw numbers to simulated input field
				if(width_sum > 0 && text_mode == 1)
					DrawString(135, 375, to_string(width_sum));
				if(height_sum > 0 && text_mode == 2)
					DrawString(145, 375, to_string(height_sum));

				if (input_number != 0) {
					if (text_mode == 1) 
						width_sum = width_sum * 10 + input_number - 48;
					else 
						height_sum = height_sum * 10 + input_number - 48;
					input_number = 0;
				}

				if (GetKey(BACK).bPressed)
					text_mode == 1 ? width_sum /= 10 : height_sum /= 10;

				if (GetKey(ENTER).bPressed) {
					text_mode++;
				}

				if (text_mode >= 3) {
					if(width_sum != 0)
						n_width = width_sum;
					if (height_sum != 0)
						n_height = height_sum;

					width_sum = 0;
					height_sum = 0;
					ResizeMapInfo();
					text_mode = 0;
				}
			}
		}

        // Draw tilemap to screen
        DrawLine(497, 0, 497, 369, DARK_GREY);
        DrawLine(0, 369, 497, 369, DARK_GREY);
        auto _path = tilemap_name;
        auto _path_string = _path.u8string();
		DrawString(504, 10, "Level Width : " + to_string(n_width));
		DrawString(504, 20, "Level Height: " + to_string(n_height));
		DrawString(504, 30, "Tilemap: " + _path_string);
		DrawSprite(504, 40, res->tMap->spr);

        // Draw outline on selected tilemap tile
        if (x1 != NULL)
            DrawRect(x1, y1, x2, y2, YELLOW);

		// Save As
		if (GetKey(F4).bPressed) {
			ResizeConvert();
			OpenLevel("Save");
			ofstream out(level_full_path, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << tilemap_full_path;
				out << endl;
				out << n_width << " " << n_height;
				out << endl;
				for (auto y = 0; y < n_height; y++) {
					for (auto x = 0; x < n_width; x++) {
						const auto index = y * n_width + x;
						out << map_info[index].tileNumber << " ";
						out << map_info[index].solid << " ";
					}
				}
			}

			out.close();
		}

		// Save Level
		if (GetKey(F5).bPressed) {
			ResizeConvert();
			if (level_full_path.empty()) 
				OpenLevel("Save");

			ofstream out(level_full_path, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << tilemap_full_path;
				out << endl;
				out << n_width << " " << n_height;
				out << endl;
				for (auto y = 0; y < n_height; y++) {
					for (auto x = 0; x < n_width; x++) {
						const auto index = y * n_width + x;
						out << map_info[index].tileNumber << " ";
						out << map_info[index].solid << " ";
					}
				}
			}
			
			out.close();
		}

        // Open Level
		if (GetKey(F8).bPressed) {
			OpenLevel("Level");
			ifstream data(level_full_path, ios::in | ios::binary);
			if (data.is_open()) {
				data >> tilemap_full_path;
				data >> n_width >> n_height;
				if (tilemap_full_path == "-1") {
					tilemap_full_path = "";
				}

				if (!tilemap_full_path.empty()) {
					SetPaths("Tilemap", tilemap_full_path);
					if (tilemap_extension == ".png") 
						res->tMap->SetSprite(new Sprite(tilemap_full_path));
					if (tilemap_extension == ".spr") 
						res->tMap->SetSprite(SprConverter::convertSmartBlendedPixels(tilemap_full_path, 1));
				}

				ResizeMapInfo();
				for (auto y = 0; y < n_height; y++) {
					for (auto x = 0; x < n_width; x++) {
						string boolval;
						const auto index = y * n_width + x;
						data >> map_info[index].tileNumber >> boolval;
						if (boolval == "1") 
							map_info[index].solid = true;
						else 
							map_info[index].solid = false;

						if (map_info[index].tileNumber != -1) {
							amount_drawn++;
						}
					}
				}

				data.close();
				LoadConvert();
			}
		}
		
        // Open tilemap
        if (GetKey(F9).bPressed) {
			OpenLevel("Tilemap");
			if (tilemap_extension == ".png") {
				res->tMap->SetSprite(new Sprite(tilemap_full_path));
			}

			if (tilemap_extension == ".spr") {
				res->tMap->SetSprite(SprConverter::convertSmartBlendedPixels(tilemap_full_path, 1));
			}
		}

		if (GetKey(F2).bPressed) {
			if (res != nullptr) {
				vector<unsigned char> _spriteToVector;
				auto _width = res->tMap->spr->width;
				auto _height = res->tMap->spr->height;
				for (auto y = 0; y < _height; y++) {
					for (auto x = 0; x < _width; x++) {
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).r);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).g);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).b);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).a);
					}
				}

				auto _string = tilemap_name_no_extension.string() + ".png";
				EncodeOneStep(_string, _spriteToVector, _width, _height);
			}
		}

		return true;
    }
};

static string ReturnExePath() {
	char result[MAX_PATH];
	return string(result, GetModuleFileName(nullptr, result, MAX_PATH));
}

Tilemap::Tilemap(Sprite* sprite) {
	spr = sprite;
}

Tilemap::~Tilemap() {
	delete spr;
}

void Tilemap::SetSprite(Sprite* sprite) {
	delete spr;
	spr = sprite;
}

EditorResources::EditorResources(string path) {
	tMap = new Tilemap(new Sprite(std::move(path)));
}

EditorResources::~EditorResources() {
	delete tMap;
}

void EditorResources::LoadTilemapFile(string path) {
	delete tMap;
	tMap = new Tilemap(new Sprite(std::move(path)));
}

void MapEditor::EncodeOneStep(const string& filename, vector<unsigned char>& image, const unsigned width, const unsigned height) const {
	//Encode the image
	const auto error = lodepng::encode(filename, image, width, height);

	//if there's an error, display it
	if (error)
		cout << "encoder error " << error << ": " << lodepng_error_text(error) << endl;
}

void MapEditor::LoadConvert() const {
	for (auto y = 0; y < canvas_height; y++) {
		for (auto x = 0; x < canvas_width; x++) {
			const auto index = y * n_width + x;
			const auto index2 = y * canvas_width + x;
			if (amount_drawn > 0) {
				if (map_info[index].tileNumber != -1 && x < n_width && y < n_height) {
					canvas[index2].tileNumber = map_info[index].tileNumber;
					canvas[index2].solid = map_info[index].solid;
				}
				else {
					canvas[index2].tileNumber = -1;
					canvas[index2].solid = false;
				}
			}
		}
	}
}

void MapEditor::ResizeConvert() const {
	for (auto y = 0; y < n_height; y++) {
		for (auto x = 0; x < n_width; x++) {
			const auto index = y * n_width + x;
			const auto index2 = y * canvas_width + x;

			if (canvas[index2].tileNumber != -1 && x < canvas_width && y < canvas_height) {
				map_info[index].tileNumber = canvas[index2].tileNumber;
				map_info[index].solid = canvas[index2].solid;
			}
			else {
				map_info[index].tileNumber = -1;
				map_info[index].solid = false;
			}
		}
	}
}

void MapEditor::ResizeMapInfo() {
	delete[] map_info;
	map_info = new TileInfo[n_width * n_height];
	ResizeConvert();
}

void MapEditor::PopulateTileInfo() {
	delete[] canvas;
	delete[] map_info;
	canvas = new TileInfo[canvas_width * canvas_height];
	map_info = new TileInfo[canvas_width * canvas_height];
	for (auto i = 0; i < canvas_width * canvas_height; i++) {
		canvas[i].tileNumber = -1;
		canvas[i].solid = false;

		map_info[i].tileNumber = -1;
		map_info[i].solid = false;
	}
}

void MapEditor::OpenLevel(const string& type) {
	OPENFILENAME ofn;
	::memset(&ofn, 0, sizeof ofn);
	char f1[MAX_PATH];
	f1[0] = 0;
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = f1;
	ofn.lpstrFile[0] = '\0';
	if (type == "Tilemap" || type == "SaveTileMap") { ofn.lpstrFilter = "Tile Files\0*.spr;*.png\0\0"; }
	else { ofn.lpstrFilter = "Level Files\0*.lvl\0\0"; }
	if (type == "Save") { ofn.lpstrTitle = "Save As"; }
	else { ofn.lpstrTitle = "Select A File"; }
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_NONETWORKBUTTON |
				OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY;

	if (type == "Save" || type == "SaveTileMap") {
		if (::GetSaveFileName(&ofn) != FALSE) {
			SetPaths("Level", f1);
		}
	}

	else {
		if (::GetOpenFileName(&ofn) != FALSE) {
			SetPaths(type, f1);
		}
	}
}

void MapEditor::SetPaths(const string& type, const string& file_name) {
	if (type == "Tilemap") {
		tilemap_full_path = file_name;
		tilemap_path = path(file_name).remove_filename();
		tilemap_name = path(file_name).filename();
		tilemap_extension = path(file_name).extension();
		tilemap_name_no_extension = path(file_name).stem();
	}

	if (type == "Level") {
		level_full_path = file_name;
		level_path = path(file_name).remove_filename();
		level_name = path(file_name).filename();
	}
}

int main() {
    MapEditor mapEditor;
    if (mapEditor.Construct(820, 420, 2, 2))
        mapEditor.Start();

    return 0;
}
