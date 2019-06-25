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

public:
	explicit Tilemap(Sprite* sprite);

	~Tilemap();

public:
    Sprite *spr;

public:
	void SetSprite(Sprite* sprite);
};

typedef struct EditorResources {
public:
	explicit EditorResources(string path);

	~EditorResources();

public:
	Tilemap* tMap;

public:
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

class MapEditor final : public PixelGameEngine{
private:
	EditorResources* res = nullptr;
	EditorResources* res_button = nullptr;

	int text_mode = 0;
	int input_number = 0;
	int width_sum = 0;
	int height_sum = 0;
	float old_time = 0.0f;
	float new_time = 0.0f;
	bool cursor = false;

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

    TileInfo map_info[713] = {};

    Pixel * bg_col = nullptr;

public:
	static void OpenLevel(const string& type);
	static void SetPaths(const string& type, const string& file_name);
	void EncodeOneStep(const string& filename, vector<unsigned char>& image, unsigned width, unsigned height) const;

public:
    bool OnUserCreate() override {
        sAppName = "Map Editor";

        res = new EditorResources(tilemap_full_path);
		res_button = new EditorResources(exe_path + "edit-button.png");
        bg_col = new Pixel( { 25, 25, 25, 255 } );

        offsetx = -1;
        offsety = -1;

        amount_drawn = 0;

		n_width = 31;
		n_height = 23;

        for (auto& i : map_info) {
	        i.tileNumber = -1;
	        i.solid = false;
        }

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Get mouse position
        auto mouseX = GetMouseX() / tile_size;
        auto mouseY = GetMouseY() / tile_size;
        auto actualMouseX = GetMouseX();
        auto actualMouseY = GetMouseY();

        Clear(*bg_col);

        // Mouse button handling on main area
        if (mouseX <= n_width -1 && mouseY <= n_height -1) {
            // offset eyedropper
            if (GetKey(CTRL).bHeld && GetMouse(0).bHeld) {
	            auto index = mouseY * n_width + mouseX;                
                if (map_info[index].tileNumber != -1) {
	                auto idx = map_info[index].tileNumber;
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
	                auto index = mouseY * n_width + mouseX;
	                auto tileIndex = (offsety * res->tMap->spr->width / tile_size + offsetx) / tile_size;
					map_info[index].tileNumber = tileIndex;
                    map_info[index].solid = false;

                    amount_drawn++;
                }
            }

            // Remove tile
            else if (GetMouse(1).bHeld) {
	            auto index = mouseY * n_width + mouseX;
                if (map_info[index].tileNumber != -1)
                    amount_drawn--;
				map_info[index].tileNumber = -1;
                map_info[index].solid = false;
            }

            // Turn solid state off
            else if (GetKey(SHIFT).bHeld && GetMouse(2).bHeld) {
	            auto index = mouseY * n_width + mouseX;
                map_info[index].solid = false;
            }

            // Turn solid state on
            else if (GetMouse(2).bHeld) {
	            auto index = mouseY * n_width + mouseX;
                map_info[index].solid = true;
            }
        }

		// Draw visible tile map
		if (amount_drawn > 0) {
			// Draw tiles to screen
			for (auto y = 0; y < n_height; y++) {
				for (auto x = 0; x < n_width; x++) {
					auto idx = map_info[y * n_width + x].tileNumber;
					auto sx = idx % 10;
					auto sy = idx / 10;
					if (map_info[y * n_width + x].tileNumber != -1)
						DrawPartialSprite(x * tile_size, y * tile_size, res->tMap->spr, sx * tile_size, sy * tile_size, tile_size, tile_size);
					if (map_info[y * n_width + x].solid)
						DrawRect(x * tile_size, y * tile_size, tile_size - 1, tile_size - 1, RED);

				}
			}

		}

		//DrawPartialSprite(x * TILE_SIZE - fTileOffsetX, y * TILE_SIZE - fTileOffsetY, m_pCurrentMap->pSprite, sx * TILE_SIZE, sy * TILE_SIZE, TILE_SIZE, TILE_SIZE);
			
        if (GetMouseX() / tile_size <= n_width -1 && GetMouseY() / tile_size <= n_height -1) {
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

				if (GetMouseX() >= 632 && GetMouseX() < 632 + res_button->tMap->spr->width &&
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
		}
		// Draw simulated input field
		else {
			text_mode == 1 ? DrawString(5, 375, "New Level Width: ") : DrawString(5, 375, "New Level Height: ");;
			for (auto i = 0; i < 10; i++) {
				if (GetKey(static_cast<Key>(int(K0) + i)).bPressed) input_number += '0' + i;
			}

			if (text_mode != 0) {
				new_time = new_time + fElapsedTime;
				if (new_time > old_time) {
					cursor = !cursor;
					old_time = new_time + 0.5f;
				}

				if (cursor)
					text_mode == 1 ?
						width_sum > 0 ? DrawString(135 + to_string(width_sum).length() * 8, 375, "_") : DrawString(135, 375, "_") :
						height_sum > 0 ? DrawString(145 + to_string(height_sum).length() * 8, 375, "_") : DrawString(145, 375, "_");

				if(width_sum > 0 && text_mode == 1)
					DrawString(135, 375, to_string(width_sum));
				if(height_sum > 0 && text_mode == 2)
					DrawString(145, 375, to_string(height_sum));
				if (input_number != 0) {
					text_mode == 1 ? width_sum = width_sum * 10 + (input_number - 48) : height_sum = height_sum * 10 + (input_number - 48);
					input_number = 0;
				}

				if (GetKey(BACK).bPressed)
					text_mode == 1 ? width_sum = width_sum / 10 : height_sum = height_sum / 10;

				if (GetKey(ENTER).bReleased) {
					text_mode = text_mode + 1;
				}

				if (text_mode >= 3) {
					if(width_sum != 0)
						n_width = width_sum;
					if (height_sum != 0)
						n_height = height_sum;
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
		SetPixelMode(Pixel::MASK);
		DrawSprite(632, 8, res_button->tMap->spr);
		SetPixelMode(Pixel::NORMAL);

        // Draw outline on selected tilemap tile
        if (x1 != NULL)
            DrawRect(x1, y1, x2, y2, YELLOW);

		// Save As
		if (GetKey(F4).bPressed) {
			OpenLevel("Save");

			ofstream out(level_full_path, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << tilemap_full_path;
				out << endl;
				out << n_width << " " << n_height;
				out << endl;
				for (auto y = 0; y < n_height; y++) {
					for (auto x = 0; x < n_width; x++) {
						out << map_info[y * n_width + x].tileNumber << " ";
						out << map_info[y * n_width + x].solid << " ";
					}
				}
			}

			out.close();
		}

		// Save Level
		if (GetKey(F5).bPressed) {
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
						out << map_info[y * n_width + x].tileNumber << " ";
						out << map_info[y * n_width + x].solid << " ";
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
				for (auto y = 0; y < n_height; y++) {
					for (auto x = 0; x < n_width; x++) {
						string boolval;
						data >> map_info[y * n_width + x].tileNumber >> boolval;
						boolval == "1" ? 
							map_info[y * n_width + x].solid = true : 
							map_info[y * n_width + x].solid = false;
						if (map_info[y * n_width + x].tileNumber != -1) {
							amount_drawn++;
						}
					}
				}

				data.close();
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
