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

// Path variables
string level_full_path;
path level_path;
path level_name;
string tilemap_full_path;
path tilemap_path;
path tilemap_name;
path tilemap_extension;
path tilemap_name_no_extension;

// Mouse variables
int mouseX;
int mouseY;

// Camera variables
bool cameraChanged;

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
	static const int camera_width = 31;
	static const int camera_height = 23;
	int cameraOffsetX = 0;
	int cameraOffsetY = 0;

	int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;

	int n_width = 0;
	int n_height = 0;

    int offsetx = 0;
    int offsety = 0;

	int backup_width = 0;
	int backup_height = 0;

	string exe_path = GetExePath();

	TileInfo* map_info = {};
	TileInfo* camera = {};
	TileInfo* backup = {};

    Pixel * bg_col = nullptr;

public:
	static void OpenLevel(const string& type);
	static void SetPaths(const string& type, const string& file_name);
	void EncodeOneStep(const string& filename, vector<unsigned char>& image, unsigned width, unsigned height) const;
	void ResizeConvert() const;
	void RestoreMapInfo();
	void PopulateTileInfo();
	void CheckKeyPressed();
	void SimulatedInputField(float fElapsedTime);
	void MouseClickingInCameraView();
	void UpdateCamera() const;
	void BackupMapInfo();

    bool OnUserCreate() override {
        sAppName = "Map Editor";

        res = new EditorResources(tilemap_full_path);
		res_button = new EditorResources(exe_path + "edit-button.png");
        bg_col = new Pixel( { 25, 25, 25, 255 } );

        offsetx = -1;
        offsety = -1;

		n_width = 31;
		n_height = 23;
		cameraChanged = true;

		PopulateTileInfo();

        return true;
    }

    bool OnUserUpdate(const float fElapsedTime) override {
		const auto actualMouseX = GetMouseX();
		const auto actualMouseY = GetMouseY();
		mouseX = actualMouseX / tile_size;
		mouseY = actualMouseY / tile_size;

		const auto _nWidthOffset = to_string(n_width).length() * 8;

        Clear(*bg_col);

        // Mouse button handling in camera area
        if (mouseX < n_width && mouseX < camera_width && mouseY < n_height && mouseY < camera_height) {
			MouseClickingInCameraView();
        }

		// Draw visible tiles to screen
		for (auto y = 0; y < camera_height; y++) {
			for (auto x = 0; x < camera_width; x++) {
				const auto idx = camera[y * camera_width + x].tileNumber;
				const auto sx = idx % 10;
				const auto sy = idx / 10;
				if (camera[y * camera_width + x].tileNumber != -1) {
					DrawPartialSprite(x * tile_size, y * tile_size, res->tMap->spr, sx * tile_size, sy * tile_size, tile_size, tile_size);
					if (camera[y * camera_width + x].solid)
						DrawRect(x * tile_size, y * tile_size, tile_size - 1, tile_size - 1, RED);
				}
			}
		}

		DrawString(740, 395, "(" + to_string(actualMouseX) + "," + to_string(actualMouseY) + ")", WHITE, 1);
		
    	if (actualMouseX / tile_size < camera_width && actualMouseY / tile_size < camera_height) {
	        // Draw outline where mouse is
	        DrawRect(mouseX * tile_size, mouseY * tile_size, tile_size - 1, tile_size - 1, YELLOW);

	        // Draw mouse coords to screen
	        DrawString(740, 405, "(" + to_string(mouseX + cameraOffsetX) + "," + to_string(mouseY + cameraOffsetY) + ")", WHITE, 1);
        }
        else {
	        if (GetMouse(0).bPressed) {
		        // Check if mouse is on top of tilemap
		        if (actualMouseX >= 504 && actualMouseX < 504 + res->tMap->spr->width &&
			        actualMouseY >= 40 && actualMouseY < 40 + res->tMap->spr->height) {
			        x1 = (actualMouseX - 504) / tile_size * tile_size + 504;
			        x2 = tile_size - 1;
			        y1 = (actualMouseY - 40) / tile_size * tile_size + 40;
			        y2 = x2;

			        offsetx = (actualMouseX - 504) / tile_size * tile_size;
			        offsety = (actualMouseY - 40) / tile_size * tile_size;
		        }

		        // Check if mouse is on top of Edit button
		        if (actualMouseX >= 616 + _nWidthOffset && actualMouseX < 616 + _nWidthOffset + res_button->tMap->spr->width &&
			        actualMouseY >= 8 && actualMouseY < 8 + res_button->tMap->spr->height) {
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

		} else
			SimulatedInputField(fElapsedTime);

        // Draw tilemap to screen
        DrawLine(497, 0, 497, 369, DARK_GREY);
        DrawLine(0, 369, 497, 369, DARK_GREY);
        const auto _path = tilemap_name;
        const auto _path_string = _path.u8string();
		DrawString(504, 10, "Level Width : " + to_string(n_width));
		DrawString(504, 20, "Level Height: " + to_string(n_height));
		DrawString(504, 30, "Tilemap: " + _path_string);
		DrawSprite(504, 40, res->tMap->spr);

        // Draw outline on selected tilemap tile
        if (x1 != NULL)
            DrawRect(x1, y1, x2, y2, YELLOW);

		CheckKeyPressed();
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

void MapEditor::MouseClickingInCameraView() {
	// offset eyedropper
	if (GetKey(CTRL).bHeld && GetMouse(0).bHeld) {
		const auto index = mouseY * n_width + mouseX;
		if (map_info[index].tileNumber != -1) {
			const auto idx = map_info[index].tileNumber;
			const auto sx = idx % 10;
			const auto sy = idx / 10;
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
			const auto index = (mouseY + cameraOffsetY) * n_width + mouseX + cameraOffsetX;
			const auto tileIndex = (offsety * res->tMap->spr->width / tile_size + offsetx) / tile_size;
			map_info[index].tileNumber = tileIndex;
			UpdateCamera();
		}
	}

	// Remove tile
	else if (GetMouse(1).bHeld) {
		const auto index = (mouseY + cameraOffsetY) * n_width + mouseX + cameraOffsetX;
		map_info[index].tileNumber = -1;
		map_info[index].solid = false;
		UpdateCamera();
	}

	// Turn solid state off
	else if (GetKey(SHIFT).bHeld && GetMouse(2).bHeld) {
		const auto index = (mouseY + cameraOffsetY) * n_width + mouseX + cameraOffsetX;
		map_info[index].solid = false;
		UpdateCamera();
	}

	// Turn solid state on
	else if (GetMouse(2).bHeld) {
		const auto index = (mouseY + cameraOffsetY) * n_width + mouseX + cameraOffsetX;
		map_info[index].solid = true;
		UpdateCamera();
	}
}


void MapEditor::SimulatedInputField(const float fElapsedTime) {
	const auto _widthSumOffset = to_string(width_sum).length() * 8;
	const auto _heightSumOffset = to_string(height_sum).length() * 8;

	// Draw simulated input field
	if (text_mode == 1)
		DrawString(5, 375, "New Level Width: ");
	else
		DrawString(5, 375, "New Level Height: ");;

	for (auto i = 0; i < 10; i++) {
		if (GetKey(static_cast<Key>(int(K0) + i)).bPressed) input_number += '0' + i;
		if (GetKey(static_cast<Key>(int(NP0) + i)).bPressed) input_number += '0' + i;
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
		if (width_sum > 0 && text_mode == 1)
			DrawString(135, 375, to_string(width_sum));
		if (height_sum > 0 && text_mode == 2)
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
			BackupMapInfo();
			if (width_sum != 0)
				n_width = width_sum;
			if (height_sum != 0)
				n_height = height_sum;

			width_sum = 0;
			height_sum = 0;
			RestoreMapInfo();
			text_mode = 0;
		}
	}
}

void MapEditor::CheckKeyPressed() {
	if (GetKey(RIGHT).bPressed) {
		if (cameraOffsetX + camera_width < n_width) {
			cameraOffsetX++;
			UpdateCamera();
		}
	}

	if (GetKey(LEFT).bPressed) {
		if (cameraOffsetX > 0) {
			cameraOffsetX--;
			UpdateCamera();
		}
	}

	if (GetKey(DOWN).bPressed) {
		if (cameraOffsetY + camera_height < n_height) {
			cameraOffsetY++;
			UpdateCamera();
		}
	}

	if (GetKey(UP).bPressed) {
		if (cameraOffsetY > 0) {
			cameraOffsetY--;
			UpdateCamera();
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

			delete[] map_info;
			map_info = new TileInfo[n_width * n_height];

			for (auto y = 0; y < n_height; y++) {
				for (auto x = 0; x < n_width; x++) {
					string boolval;
					const auto index = y * n_width + x;
					data >> map_info[index].tileNumber >> boolval;
					if (boolval == "1")
						map_info[index].solid = true;
					else
						map_info[index].solid = false;
				}
			}

			data.close();
			UpdateCamera();
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
}

void MapEditor::UpdateCamera() const {
	for (auto y = 0; y < camera_height; y++) {
		for (auto x = 0; x < camera_width; x++) {
			const auto index = (y + cameraOffsetY) * n_width + x + cameraOffsetX;
			const auto index2 = y * camera_width + x;
			if (map_info[index].tileNumber != -1 && x < n_width && y < n_height) {
				camera[index2].tileNumber = map_info[index].tileNumber;
				camera[index2].solid = map_info[index].solid;
			}
			else {
				camera[index2].tileNumber = -1;
				camera[index2].solid = false;
			}
		}
	}
}


void MapEditor::EncodeOneStep(const string& filename, vector<unsigned char>& image, const unsigned width, const unsigned height) const {
	//Encode the image
	const auto error = lodepng::encode(filename, image, width, height);

	//if there's an error, display it
	if (error)
		cout << "encoder error " << error << ": " << lodepng_error_text(error) << endl;
}

void MapEditor::ResizeConvert() const {
	for (auto y = 0; y < n_height; y++) {
		for (auto x = 0; x < n_width; x++) {
			const auto index = y * n_width + x;
			const auto index2 = y * camera_width + x;

			if (camera[index2].tileNumber != -1 && x < camera_width && y < camera_height) {
				map_info[index].tileNumber = camera[index2].tileNumber;
				map_info[index].solid = camera[index2].solid;
			}
			else {
				map_info[index].tileNumber = -1;
				map_info[index].solid = false;
			}
		}
	}
}

void MapEditor::BackupMapInfo() {
	backup_height = n_height;
	backup_width = n_width;
	backup = new TileInfo[backup_height * backup_width];
	for (auto i = 0; i < backup_height * backup_width; i++) 
		backup[i] = map_info[i];
}

void MapEditor::RestoreMapInfo() {
	delete[] map_info;
	map_info = new TileInfo[n_width * n_height];
	for (auto y = 0; y < n_height; y++) {
		for (auto x = 0; x < n_width; x++) {
			const auto index = y * n_width + x;
			const auto index2 = y * backup_width + x;
			if (y < n_height && y < backup_height && x < n_width && x < backup_width) {
				map_info[index] = backup[index2];
			}
			else {
				map_info[index].tileNumber = -1;
				map_info[index].solid = false;
			}
		}
	}

	delete[] backup;
	backup_height = 0;
	backup_width = 0;
	UpdateCamera();
}

void MapEditor::PopulateTileInfo() {
	delete[] camera;
	delete[] map_info;
	camera = new TileInfo[camera_width * camera_height];
	map_info = new TileInfo[n_width * n_height];
	for (auto i = 0; i < n_width * n_height; i++) {
		camera[i].tileNumber = -1;
		camera[i].solid = false;

		map_info[i].tileNumber = -1;
		map_info[i].solid = false;
	}
}

void MapEditor::OpenLevel(const string& type) {
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof ofn);
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
