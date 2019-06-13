/* 
MapEditor.cpp

MapEditor for One Lone Coders RPG game by SweFjorod.

Thanks to One Lone Coder for the olcPixelGameEngine
Thanks to Gorbit99 for the olcSprConverter.h
*/

#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"
#include "olcSprConverter.h"

#include <fstream>
#include <sstream>
#include <map>
#include <commdlg.h>
#include <iostream>
#include <filesystem>

using namespace std;
using namespace experimental::filesystem;
using namespace olc;

constexpr auto TILE_SIZE = 16;

//  Path variables
string Level_FullPath;
path Level_Path;
path Level_Name;
string Tilemap_FullPath;
path Tilemap_Path;
path Tilemap_Name;
path Tilemap_Extension;

void Set_Paths(string, string);

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

    int offsetx;
    int offsety;

    int amountDrawn;

    tile_info mapInfo[713];

    Pixel *bg_col;

public:
    bool OnUserCreate() override {
        sAppName = "Map Editor";

        res = new editor_resources(Tilemap_FullPath);
        bg_col = new Pixel( { 25, 25, 25, 255 } );

        offsetx = -1;
        offsety = -1;

        amountDrawn = 0;
		
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
                int index = mouseY * 31 + mouseX;                
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
                    int index = mouseY * 31 + mouseX;
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
                int index = mouseY * 31 + mouseX;
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
                int index = mouseY * 31 + mouseX;
                mapInfo[index].solid = false;
            }

            // Turn solid state on
            else if (GetMouse(2).bHeld) {
                int index = mouseY * 31 + mouseX;
                mapInfo[index].solid = true;
            }
        }

        // Draw tiles to screen
        if (amountDrawn > 0)
			for (int i = 0; i < 713; i++) {
				if (mapInfo[i].x != -1)
					DrawPartialSprite(mapInfo[i].x * TILE_SIZE, mapInfo[i].y * TILE_SIZE, res->tMap->spr, mapInfo[i].xo, mapInfo[i].yo, TILE_SIZE, TILE_SIZE, 1);

				if (mapInfo[i].solid)
					DrawRect(mapInfo[i].x * TILE_SIZE, mapInfo[i].y * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1, RED);
			}

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
		DrawString(5, 375, "F4 = Save As  F5 = Save  F8 = Load Level  F9 = Load Tilemap", WHITE, 1);
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
			OPENFILENAME ofn;
			::memset(&ofn, 0, sizeof(ofn));
			char f1[MAX_PATH];
			f1[0] = 0;
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = f1;
			ofn.lpstrFile[0] = '\0';
			ofn.lpstrFilter = "Level Files\0*.lvl\0\0";
			ofn.lpstrTitle = "Save As";
			ofn.nFilterIndex = 1;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_NONETWORKBUTTON |
						OFN_FILEMUSTEXIST |
						OFN_HIDEREADONLY;

			if (::GetSaveFileName(&ofn) != FALSE) {
				Set_Paths("Level", f1);
			}

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
			if (Level_FullPath == "") {
				OPENFILENAME ofn;
				::memset(&ofn, 0, sizeof(ofn));
				char f1[MAX_PATH];
				f1[0] = 0;
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;
				ofn.lpstrFile = f1;
				ofn.lpstrFile[0] = '\0';
				ofn.lpstrFilter = "Level Files\0*.lvl\0\0";
				ofn.lpstrTitle = "Save As";
				ofn.nFilterIndex = 1;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_NONETWORKBUTTON |
							OFN_FILEMUSTEXIST |
							OFN_HIDEREADONLY;

				if (::GetSaveFileName(&ofn) != FALSE) {
					Set_Paths("Level", f1);
				}
			}

			ofstream out(Level_FullPath, ios::out | ios::trunc | ios::binary);
			if (out.is_open()) {
				out << Tilemap_FullPath;
				for (int i = 0; i < 713; i++) {
					out << " " << to_string(mapInfo[i].x) << " " << to_string(mapInfo[i].y) << " " << to_string(mapInfo[i].xo) << " " << to_string(mapInfo[i].yo) << " " << (mapInfo[i].solid ? "1" : "0");
				}
			}
			out.close();
		}

        // Open Level
		if (GetKey(F8).bPressed) {
			OPENFILENAME ofn;
			::memset(&ofn, 0, sizeof(ofn));
			char f1[MAX_PATH];
			f1[0] = 0;
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = f1;
			ofn.lpstrFile[0] = '\0';
			ofn.lpstrFilter = "Level Files\0*.lvl\0\0";
			ofn.lpstrTitle = "Select A File";
			ofn.nFilterIndex = 1;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_NONETWORKBUTTON |
						OFN_FILEMUSTEXIST |
						OFN_HIDEREADONLY;

			if (::GetOpenFileName(&ofn) != FALSE) {
				Set_Paths("Level", f1);
				ifstream in(Level_FullPath, ios::in | ios::binary);
				if (in.is_open()) {
					string filename;
					in >> filename;
					if (filename == "-1") {
						filename = "";
					}

					if (filename != "") {
						Set_Paths("Tilemap", filename);
						if (Tilemap_Extension == ".png") {
							res->tMap->set_sprite(new Sprite(Tilemap_FullPath));
						}

						if (Tilemap_Extension == ".spr") {
							res->tMap->set_sprite(SprConverter::convertDumbBlendedPixels(Tilemap_FullPath, 1));
						}
					}

					for (int i = 0; i < 713; i++) {
						string boolval;

						in >> mapInfo[i].x >> mapInfo[i].y >> mapInfo[i].xo >> mapInfo[i].yo >> boolval;
						if (boolval == "1") mapInfo[i].solid = true;
						else                mapInfo[i].solid = false;

						if (mapInfo[i].x != -1)
							amountDrawn++;
					}
				}

				in.close();
			}
		}


        // Open tilemap
        if (GetKey(F9).bPressed) {
			OPENFILENAME ofn;
			::memset(&ofn, 0, sizeof(ofn));
			char f1[MAX_PATH];
			f1[0] = 0;
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = f1;
			ofn.lpstrFile[0] = '\0';
			ofn.lpstrFilter = "Sprite Files\0*.spr\0Png Files\0*.png\0\0";
			ofn.lpstrTitle = "Select A File";
			ofn.nFilterIndex = 1;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_NONETWORKBUTTON |
						OFN_FILEMUSTEXIST |
						OFN_HIDEREADONLY;

			if (::GetOpenFileName(&ofn) != FALSE) {
				Set_Paths("Tilemap", f1);
				if (Tilemap_Extension == ".png") {
					res->tMap->set_sprite(new Sprite(Tilemap_FullPath));
				}

				if (Tilemap_Extension == ".spr") {
					res->tMap->set_sprite(SprConverter::convertDumbBlendedPixels(Tilemap_FullPath, 1));
				}
			}
        }

        return true;
    }
};

void Set_Paths(string _type, string _fileName) {
	if (_type == "Tilemap") {
		Tilemap_FullPath = _fileName;
		Tilemap_Path = path(_fileName).remove_filename();
		Tilemap_Name = path(_fileName).filename();
		Tilemap_Extension = path(_fileName).extension();
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
