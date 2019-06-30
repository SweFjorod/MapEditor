# MapEditor
MapEditor for One Lone Coders RPG game by SweFjorod.

Thanks to One Lone Coder for the olcPixelGameEngine
Thanks to Gorbit99 for the olcSprConverter.h
Thanks to Lode Vandevenne for the lodepng so I could encode png files.

You need to have edit-button.png and font.pgex in the same spot as the executable, for example
if you are running VS and debugging remember to copy those 2 files to the debug lib.

This started out as a small project where I wanted to learn how to code C++ but has evolved
to a much bigger project over time.

This is basicly just a "paint" program where you instead of painting with colors you paint
with png's (or part of them), as OLC's RPG game is made with olcConsoleGameEngine this 
also supports loading .spr files, you can then (convert and) save them as png's if you like.

This code might not be the best way to code in C++ as I had not coded a single line of C++
code prior to this project, so I learned C++ while coding this.

The Level (map) files are stored as plain text, first line is where and the name of the
.png (or .spr) file to use. Second line is the size of the level, then it's just stored
from top left corner of the level, what sprite to use and if it's solid or not, so each
16x16 pixel is stored in 2 blocks.

Without much effort, you could add if you like a way to save the canvas as a .png file as
well, all the encoding is allready in the code, if you wanted to make new png's to use
as new tiles.
