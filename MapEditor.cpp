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
#include <map>
#include <commdlg.h>
#include <iostream>
#include <filesystem>

using namespace olc;
using namespace std;
using namespace std::experimental::filesystem;

constexpr auto TILE_SIZE = 16;

//  Path variables
string Level_FullPath;
path Level_Path;
path Level_Name;
string Tilemap_FullPath;
path Tilemap_Path;
path Tilemap_Name;
path Tilemap_Extension;
path Tilemap_NameNoExtension;

static string GetexePath();
string ReturnexePath();

typedef struct tilemap {
public:
	tilemap(Sprite *sprite) {
        spr = sprite;
    }
    ~tilemap() {
        delete spr;
    }

public:
    Sprite *spr;

public:
    void set_sprite(Sprite *sprite) {
        delete spr;
        spr = sprite;
    }
};

typedef struct editor_resources {
public:
    editor_resources(string path) {
        tMap = new tilemap(new Sprite(path));
    }

    ~editor_resources() {
        delete tMap;
    }

public:
    tilemap *tMap;

public:
    void load_tilemap_file(string path) {
        delete tMap;
        tMap = new tilemap(new Sprite(path));
    }
};

struct tile_info {
	int x;
    int y;
    int xo;
    int yo;
    bool solid;
};


class MapEditor : public PixelGameEngine {
private:
	editor_resources* res;

	int x1;
    int x2;
    int y1;
    int y2;

	int nWidth;
	int nHeight;

    int offsetx;
    int offsety;

	float fCameraPosX;
	float fCameraPosY;

    int amountDrawn;

	string _exePath;

    tile_info mapInfo[713];

    Pixel *bg_col;

	int* m_indices = nullptr;
	bool* m_solids = nullptr;

public:
	void Open_Level(string _type);
	void Set_Paths(string _type, string _fileName);
	void encodeOneStep(string filename, vector<unsigned char>& image, unsigned width, unsigned height);

	int GetIndex(int x, int y);
	bool GetSolid(int x, int y);

public:
    bool OnUserCreate() override {
        sAppName = "Map Editor";

        res = new editor_resources(Tilemap_FullPath);
        bg_col = new Pixel( { 25, 25, 25, 255 } );

        offsetx = -1;
        offsety = -1;

        amountDrawn = 0;

		nWidth = 31;
		nHeight = 23;
		fCameraPosX = 0.0f;
		fCameraPosY = 0.0f;

		m_solids = nullptr;
		m_indices = nullptr;

		_exePath = GetexePath();
		
        for (int i = 0; i < 713; i++) {
			mapInfo[i].x = -1;
            mapInfo[i].y = -1;
            mapInfo[i].xo = -1;
            mapInfo[i].yo = -1;
            mapInfo[i].solid = false;
        }

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Get mouse position
        int mouseX = GetMouseX() / TILE_SIZE;
        int mouseY = GetMouseY() / TILE_SIZE;
		int actualMouseX = GetMouseX();
		int actualMouseY = GetMouseY();

        Clear(*bg_col);

        // Mouse button handling on main area
        if (mouseX <= 30 && mouseY <= 22) {
            // offset eyedropper
            if (GetKey(CTRL).bHeld && GetMouse(0).bHeld) {
                int index = mouseY * nWidth + mouseX;                
                if (mapInfo[index].xo != -1) {
                    offsetx = mapInfo[index].xo;
                    offsety = mapInfo[index].yo;
                    x1 = offsetx + 504;
                    x2 = TILE_SIZE - 1;
                    y1 = offsety + 20;
                    y2 = TILE_SIZE - 1;
                }
            }

            // Place tile
            else if (GetMouse(0).bHeld) {
                if (offsetx != -1 && offsety != -1) {
                    int index = mouseY * nWidth + mouseX;
					int tileIndex = offsety * nWidth + offsetx;
					m_indices[index] = tileIndex;
					m_solids[index] = false;
                    mapInfo[index].x = mouseX;
                    mapInfo[index].y = mouseY;
                    mapInfo[index].xo = offsetx;
                    mapInfo[index].yo = offsety;
                    mapInfo[index].solid = false;

                    amountDrawn++;
                }
            }

            // Remove tile
            else if (GetMouse(1).bHeld) {
                int index = mouseY * nWidth + mouseX;
                if (mapInfo[index].x != -1)
                    amountDrawn--;
                mapInfo[index].x = -1;
                mapInfo[index].y = -1;
                mapInfo[index].xo = -1;
                mapInfo[index].yo = -1;
                mapInfo[index].solid = false;
            }

            // Turn solid state off
            else if (GetKey(SHIFT).bHeld && GetMouse(2).bHeld) {
                int index = mouseY * nWidth + mouseX;
                mapInfo[index].solid = false;
            }

            // Turn solid state on
            else if (GetMouse(2).bHeld) {
                int index = mouseY * nWidth + mouseX;
				m_solids[index] = true;
                mapInfo[index].solid = true;
            }
        }

		// Draw Level
		int nVisibleTilesX = nWidth;
		int nVisibleTilesY = nHeight;

		// Calculate Top-Leftmost visible tile
		float fOffsetX = fCameraPosX - (float)nVisibleTilesX / 2.0f;
		float fOffsetY = fCameraPosY - (float)nVisibleTilesY / 2.0f;

		// Clamp camera to game boundaries
		if (fOffsetX < 0) fOffsetX = 0;
		if (fOffsetY < 0) fOffsetY = 0;
		if (fOffsetX > nWidth - nVisibleTilesX) fOffsetX = nWidth - nVisibleTilesX;
		if (fOffsetY > nHeight - nVisibleTilesY) fOffsetY = nHeight - nVisibleTilesY;

		// Get offsets for smooth movement
		float fTileOffsetX = (fOffsetX - (int)fOffsetX) * TILE_SIZE;
		float fTileOffsetY = (fOffsetY - (int)fOffsetY) * TILE_SIZE;

		// Draw visible tile map
		if (amountDrawn > 0) {
			// Draw tiles to screen
			for (int i = 0; i < nVisibleTilesX * nVisibleTilesY; i++) {
				if (mapInfo[i].x != -1)
					DrawPartialSprite(mapInfo[i].x * TILE_SIZE, mapInfo[i].y * TILE_SIZE, res->tMap->spr, mapInfo[i].xo, mapInfo[i].yo, TILE_SIZE, TILE_SIZE, 1);

				if (mapInfo[i].solid)
					DrawRect(mapInfo[i].x * TILE_SIZE, mapInfo[i].y * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1, RED);
			}
		}

		//DrawPartialSprite(x * TILE_SIZE - fTileOffsetX, y * TILE_SIZE - fTileOffsetY, m_pCurrentMap->pSprite, sx * TILE_SIZE, sy * TILE_SIZE, TILE_SIZE, TILE_SIZE);
			
        if (GetMouseX() / TILE_SIZE <= 30 && GetMouseY() / TILE_SIZE <= 22) {
            // Draw outline where mouse is
            DrawRect(mouseX * TILE_SIZE, mouseY * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1, YELLOW);

            // Draw mouse coords to screen
            DrawString(634, 395, "(" + to_string(mouseX) + "," + to_string(mouseY) + ")", WHITE, 1);
		}
        else {
            if (GetMouse(0).bPressed) {
                // Check if mouse is on top of tilemap
                if (GetMouseX() >= 504 && GetMouseX() < 504 + res->tMap->spr->width &&
                    GetMouseY() >= 20 && GetMouseY() < 20 + res->tMap->spr->height) {
					x1 = ((int)(actualMouseX - 504) / TILE_SIZE) * TILE_SIZE + 504;
                    x2 = TILE_SIZE - 1;
					y1 = ((int)(actualMouseY - 20) / TILE_SIZE) * TILE_SIZE + 20;
                    y2 = x2;

                    offsetx = ((int)(actualMouseX - 504) / TILE_SIZE) * TILE_SIZE;
                    offsety = ((int)(actualMouseY - 20) / TILE_SIZE) * TILE_SIZE;
                }
            }
        }

		// Draw Menu and help
		DrawString(5, 375, "F2 = Save Tilemap F4 = Save As  F5 = Save  F8 = Load Level  F9 = Load Tilemap", WHITE, 1);
		DrawString(5, 395, "[LEFT CLICK] Draw   [RIGHT CLICK] Erase   [CTRL+LEFT CLICK] Select tile", WHITE, 1);
		DrawString(5, 405, "[MIDDLE CLICK] Set collision   [SHIFT+MIDDLE CLICK] Unset collision", WHITE, 1);

        // Draw tilemap to screen
        DrawLine(497, 0, 497, 369, DARK_GREY);
        DrawLine(0, 369, 497, 369, DARK_GREY);
		path _path = Tilemap_Name;
		string _path_string = _path.u8string();
		DrawString(504, 10, "Tilemap: " + _path_string);
		DrawSprite(504, 20, res->tMap->spr);

        // Draw outline on selected tilemap tile
        if (x1 != NULL)
            DrawRect(x1, y1, x2, y2, YELLOW);

		// Save As
		if (GetKey(F4).bPressed) {
			Open_Level("Save");

			ofstream out(Level_FullPath, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << Tilemap_FullPath;
				for (int i = 0; i < 713; i++) {
					out << " " << to_string(mapInfo[i].x) << " " << to_string(mapInfo[i].y) << " " << to_string(mapInfo[i].xo) << " " << to_string(mapInfo[i].yo) << " " << (mapInfo[i].solid ? "1" : "0");
				}
			}

			out.close();
		}

		// Save Level
		if (GetKey(F5).bPressed) {
			if (Level_FullPath == "") 
				Open_Level("Save");

			ofstream out(Level_FullPath, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << Tilemap_FullPath;
				out << nWidth << " " << nHeight;
				for (int x = -1; x < nVisibleTilesX + 1; x++)
				{
					for (int y = -1; y < nVisibleTilesY + 1; y++)
					{
						//out << " " << to_string(mapInfo[i].x) << " " << to_string(mapInfo[i].y) << " " << to_string(mapInfo[i].xo) << " " << to_string(mapInfo[i].yo) << " " << (mapInfo[i].solid ? "1" : "0");
						out << m_indices[GetIndex(x, y)];
						out << m_solids[GetSolid(x, y)];
					}
				}
			}
			out.close();
		}

        // Open Level
		if (GetKey(F8).bPressed) {
			Open_Level("Level");
			ifstream data(Level_FullPath, ios::in | ios::binary);
			if (data.is_open()) {
				string filename;
				data >> filename;
				if (filename == "-1") {
					filename = "";
				}

				if (filename != "") {
					Set_Paths("Tilemap", filename);
					if (Tilemap_Extension == ".png") 
						res->tMap->set_sprite(new Sprite(Tilemap_FullPath));

					if (Tilemap_Extension == ".spr") 
						res->tMap->set_sprite(SprConverter::convertSmartBlendedPixels(Tilemap_FullPath, 1));
				}

				// data >> nWidth >> nHeight;
				//m_indices = new int[nWidth * nHeight];
				//for (int i = 0; i < nWidth * nHeight; i++) {
					//data >> m_indices[i];
					//data >> m_solids[i];
				//}

				for (int i = 0; i < 713; i++) {
					string boolval;
					data >> mapInfo[i].x >> mapInfo[i].y >> mapInfo[i].xo >> mapInfo[i].yo >> boolval;
					if (boolval == "1") 
						mapInfo[i].solid = true;
					else                
						mapInfo[i].solid = false;

					if (mapInfo[i].x != -1)
						amountDrawn++;
				}

				data.close();
			}
		}
		
        // Open tilemap
        if (GetKey(F9).bPressed) {
			Open_Level("Tilemap");
			if (Tilemap_Extension == ".png") {
				res->tMap->set_sprite(new Sprite(Tilemap_FullPath));
			}

			if (Tilemap_Extension == ".spr") {
				res->tMap->set_sprite(SprConverter::convertSmartBlendedPixels(Tilemap_FullPath, 1));
			}
		}

		if (GetKey(F2).bPressed) {
			if (res != nullptr) {
				vector<unsigned char> _spriteToVector;
				int _width = res->tMap->spr->width;
				int _height = res->tMap->spr->height;
				for (int y = 0; y < _height; y++) {
					for (int x = 0; x < _width; x++) {
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).r);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).g);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).b);
						_spriteToVector.push_back(res->tMap->spr->GetPixel(x, y).a);
					}
				}

				string _string = Tilemap_NameNoExtension.string() + ".png";
				encodeOneStep(_string, _spriteToVector, _width, _height);
			}
		}

		return true;
    }
};

static string ReturnexePath() {
	char result[MAX_PATH];
	return string(result, GetModuleFileName(NULL, result, MAX_PATH));
}

static string GetexePath() {
	string _newpath = ReturnexePath();
	size_t _pos = _newpath.rfind("\\");
	if (_pos != string::npos)
		_newpath.erase(_pos + 1);
	return _newpath;
}

int MapEditor::GetIndex(int x, int y)
{
	if (x >= 0 && x < nWidth && y >= 0 && y < nHeight)
		return m_indices[y * nWidth + x];
	else
		return 0;
}

bool MapEditor::GetSolid(int x, int y)
{
	if (x >= 0 && x < nWidth && y >= 0 && y < nHeight)
		return m_solids[y * nWidth + x];
	else
		return true;
}

void MapEditor::encodeOneStep(string filename, vector<unsigned char>& image, unsigned width, unsigned height) {
	//Encode the image
	unsigned error = lodepng::encode(filename, image, width, height);

	//if there's an error, display it
	if (error)
		cout << "encoder error " << error << ": " << lodepng_error_text(error) << endl;
}

void MapEditor::Open_Level(string _type) {
	OPENFILENAME ofn;
	::memset(&ofn, 0, sizeof(ofn));
	char f1[MAX_PATH];
	f1[0] = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = f1;
	ofn.lpstrFile[0] = '\0';
	if (_type == "Tilemap" || _type == "SaveTileMap") { ofn.lpstrFilter = "Tile Files\0*.spr;*.png\0\0"; }
	else { ofn.lpstrFilter = "Level Files\0*.lvl\0\0"; }
	if (_type == "Save") { ofn.lpstrTitle = "Save As"; }
	else { ofn.lpstrTitle = "Select A File"; }
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_NONETWORKBUTTON |
				OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY;

	if (_type == "Save" || _type == "SaveTileMap") {
		if (::GetSaveFileName(&ofn) != FALSE) {
			Set_Paths("Level", f1);
		}
	}

	else {
		if (::GetOpenFileName(&ofn) != FALSE) {
			Set_Paths(_type, f1);
		}
	}
}

void MapEditor::Set_Paths(string _type, string _fileName) {
	if (_type == "Tilemap") {
		Tilemap_FullPath = _fileName;
		Tilemap_Path = path(_fileName).remove_filename();
		Tilemap_Name = path(_fileName).filename();
		Tilemap_Extension = path(_fileName).extension();
		Tilemap_NameNoExtension = path(_fileName).stem();
	}

	if (_type == "Level") {
		Level_FullPath = _fileName;
		Level_Path = path(_fileName).remove_filename();
		Level_Name = path(_fileName).filename();
	}
}

int main() {
    MapEditor mapEditor;
    if (mapEditor.Construct(820, 420, 2, 2))
        mapEditor.Start();

    return 0;
}
