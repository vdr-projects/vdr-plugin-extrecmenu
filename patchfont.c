/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/font.h>
#include "patchfont.h"
#include "mymenusetup.h"

void PatchFont(eDvbFont DvbFont)
{
 const cFont *font = cFont::GetFont(DvbFont);
 int num_rows_global = font->Height();
 int num_rows = num_rows_global+2;
 int i,j,num_char;
 
 cFont::tPixelData* font_data = new cFont::tPixelData[225*(num_rows)];
 
 for (i = 0; i < 225; i++)
  for (j = 0; j < num_rows; j++)
   font_data[(i*num_rows)+j]=0x0000000000000000;
   
 font_data[0+0]=6;//font->CharData(0)->width;
 font_data[0+1]=num_rows_global;
 
 for(num_char=33, i = 1; num_char<256; i++, num_char++)
 {
  const cFont::tCharData* char_data = font->CharData(num_char);
  font_data[i*num_rows+0] = (num_char>=127 && num_char<=129)?6:char_data->width;
  font_data[i*num_rows+1] = char_data->height;

  // new marker
  if(num_char==128&&mysetup.PatchNew)
  {
   if(DvbFont==fontOsd)
   {
    font_data[i*num_rows+ 0]=22;
    font_data[i*num_rows+ 1]=27;
    font_data[i*num_rows+ 5]=0x000000; // .. .... .... .... .... ....
    font_data[i*num_rows+ 6]=0x000f00; // .. .... .... #### .... ....
    font_data[i*num_rows+ 7]=0x007fe0; // .. .... .### #### ###. ....
    font_data[i*num_rows+ 8]=0x00fff0; // .. .... #### #### #### ....
    font_data[i*num_rows+ 9]=0x01f0f8; // .. ...# #### .... #### #...
    font_data[i*num_rows+10]=0x01c03c; // .. ...# ##.. .... ..## ##..
    font_data[i*num_rows+11]=0x03c01c; // .. ..## ##.. .... ...# ##..
    font_data[i*num_rows+12]=0x03801c; // .. ..## #... .... ...# ##..
    font_data[i*num_rows+13]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+14]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+15]=0x1ff00e; // .#.#### #### .... .... ###.
    font_data[i*num_rows+16]=0x0fe00e; // .. #### ###. .... .... ###.
    font_data[i*num_rows+17]=0x07c00e; // .. .### ##.. .... .... ###.
    font_data[i*num_rows+18]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+19]=0x01001c; // .. ...# .... .... ...# ##..
    font_data[i*num_rows+20]=0x00001c; // .. .... .... .... ...# ##..
    font_data[i*num_rows+21]=0x00003c; // .. .... .... .... ..## ##..
    font_data[i*num_rows+22]=0x0000f8; // .. .... .... .... #### #...
    font_data[i*num_rows+23]=0x000ff0; // .. .... .... #### #### ....
    font_data[i*num_rows+24]=0x001fe0; // .. .... ...# #### ###. ....
    font_data[i*num_rows+25]=0x000f00; // .. .... .... #### .... ....
    font_data[i*num_rows+26]=0x000000; // .. .... .... .... .... ....
   }
   else
   {
    font_data[i*num_rows+ 0]=22;
    font_data[i*num_rows+ 1]=22;
    font_data[i*num_rows+ 2]=0x000000; // .. .... .... .... .... ....
    font_data[i*num_rows+ 3]=0x000f00; // .. .... .... #### .... ....
    font_data[i*num_rows+ 4]=0x007fe0; // .. .... .### #### ###. ....
    font_data[i*num_rows+ 5]=0x00fff0; // .. .... #### #### #### ....
    font_data[i*num_rows+ 6]=0x01f0f8; // .. ...# #### .... #### #...
    font_data[i*num_rows+ 7]=0x01c03c; // .. ...# ##.. .... ..## ##..
    font_data[i*num_rows+ 8]=0x03c01c; // .. ..## ##.. .... ...# ##..
    font_data[i*num_rows+ 9]=0x03801c; // .. ..## #... .... ...# ##..
    font_data[i*num_rows+10]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+11]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+12]=0x1ff00e; // .#.#### #### .... .... ###.
    font_data[i*num_rows+13]=0x0fe00e; // .. #### ###. .... .... ###.
    font_data[i*num_rows+14]=0x07c00e; // .. .### ##.. .... .... ###.
    font_data[i*num_rows+15]=0x03800e; // .. ..## #... .... .... ###.
    font_data[i*num_rows+16]=0x01001c; // .. ...# .... .... ...# ##..
    font_data[i*num_rows+17]=0x00001c; // .. .... .... .... ...# ##..
    font_data[i*num_rows+18]=0x00003c; // .. .... .... .... ..## ##..
    font_data[i*num_rows+19]=0x0000f8; // .. .... .... .... #### #...
    font_data[i*num_rows+20]=0x000ff0; // .. .... .... #### #### ....
    font_data[i*num_rows+21]=0x001fe0; // .. .... ...# #### ###. ....
    font_data[i*num_rows+22]=0x000f00; // .. .... .... #### .... ....
    font_data[i*num_rows+23]=0x000000; // .. .... .... .... .... ....
   }
  }
  // dvd marker
  else if(num_char==129&&mysetup.PatchDvd)
  {
   if(DvbFont==fontOsd)
   {
    font_data[i*num_rows+ 0]=22;
    font_data[i*num_rows+ 1]=27;
    font_data[i*num_rows+ 5]=0x0;      // .. .... .... .... .... ....
    font_data[i*num_rows+ 6]=0x003f00; // .. .... ..## #### .... ....
    font_data[i*num_rows+ 7]=0x01ffe0; // .. ...# #### #### ###. ....
    font_data[i*num_rows+ 8]=0x03ff90; // .. ..## #### #### #..# ....
    font_data[i*num_rows+ 9]=0x07ff08; // .. .### #### #### .... #...
    font_data[i*num_rows+10]=0x0fff04; // .. #### #### #### .... .#..
    font_data[i*num_rows+11]=0x0ffe04; // .. #### #### ###. .... .#..
    font_data[i*num_rows+12]=0x0ffe0c; // .. #### #### ###. .... ##..
    font_data[i*num_rows+13]=0x1ffc3e; // .# #### #### ##.. ..## ###.
    font_data[i*num_rows+14]=0x1ff2fe; // .# #### #### ..#. #### ###.
    font_data[i*num_rows+15]=0x1fe1fe; // .# #### ###. ...# #### ###.
    font_data[i*num_rows+16]=0x1fe1fe; // .# #### ###. ...# #### ###.
    font_data[i*num_rows+17]=0x1fd3fe; // .# #### ##.# ..## #### ###.
    font_data[i*num_rows+18]=0x1f0ffe; // .# #### .... #### #### ###.
    font_data[i*num_rows+19]=0x0c1ffc; // .. ##.. ...# #### #### ##..
    font_data[i*num_rows+20]=0x081ffc; // .. #... ...# #### #### ##..
    font_data[i*num_rows+21]=0x083ffc; // .. #... ..## #### #### ##..
    font_data[i*num_rows+22]=0x043ff8; // .. .#.. ..## #### #### #...
    font_data[i*num_rows+23]=0x027ff0; // .. ..#. .### #### #### ....
    font_data[i*num_rows+24]=0x01ffe0; // .. ...# #### #### ###. ....
    font_data[i*num_rows+25]=0x003f00; // .. .... ..## #### .... ....
    font_data[i*num_rows+26]=0x000000; // .. .... .... .... .... ....
   }
   else
   {
    font_data[i*num_rows+ 0]=22;
    font_data[i*num_rows+ 1]=22;
    font_data[i*num_rows+ 2]=0x0;      // .. .... .... .... .... ....
    font_data[i*num_rows+ 3]=0x003f00; // .. .... ..## #### .... ....
    font_data[i*num_rows+ 4]=0x01ffe0; // .. ...# #### #### ###. ....
    font_data[i*num_rows+ 5]=0x03ff90; // .. ..## #### #### #..# ....
    font_data[i*num_rows+ 6]=0x07ff08; // .. .### #### #### .... #...
    font_data[i*num_rows+ 7]=0x0fff04; // .. #### #### #### .... .#..
    font_data[i*num_rows+ 8]=0x0ffe04; // .. #### #### ###. .... .#..
    font_data[i*num_rows+ 9]=0x0ffe0c; // .. #### #### ###. .... ##..
    font_data[i*num_rows+10]=0x1ffc3e; // .# #### #### ##.. ..## ###.
    font_data[i*num_rows+11]=0x1ff2fe; // .# #### #### ..#. #### ###.
    font_data[i*num_rows+12]=0x1fe1fe; // .# #### ###. ...# #### ###.
    font_data[i*num_rows+13]=0x1fe1fe; // .# #### ###. ...# #### ###.
    font_data[i*num_rows+14]=0x1fd3fe; // .# #### ##.# ..## #### ###.
    font_data[i*num_rows+15]=0x1f0ffe; // .# #### .... #### #### ###.
    font_data[i*num_rows+16]=0x0c1ffc; // .. ##.. ...# #### #### ##..
    font_data[i*num_rows+17]=0x081ffc; // .. #... ...# #### #### ##..
    font_data[i*num_rows+18]=0x083ffc; // .. #... ..## #### #### ##..
    font_data[i*num_rows+19]=0x043ff8; // .. .#.. ..## #### #### #...
    font_data[i*num_rows+20]=0x027ff0; // .. ..#. .### #### #### ....
    font_data[i*num_rows+21]=0x01ffe0; // .. ...# #### #### ###. ....
    font_data[i*num_rows+22]=0x003f00; // .. .... ..## #### .... ....
    font_data[i*num_rows+23]=0x000000; // .. .... .... .... .... ....
   }
  }
  else
  {
   for(j = 0; j<(int)char_data->height; j++)
   {
    cFont::tPixelData Data = (cFont::tPixelData)char_data->lines[j];
 
    font_data[(i*num_rows)+2+j] = Data;
   }
  }
 }
 cFont::SetFont(DvbFont, font_data);
}