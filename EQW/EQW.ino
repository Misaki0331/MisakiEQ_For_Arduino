#define version_Major 1
#define version_Minor 1
#define version_Maintenance 3
#include "bin.h"
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include <SD-master.h>
#include <SPI.h>
SDClass mySD;
Sd2Card card;
SdVolume volume;
SdFile root;
bool isSDReady = 0;
//if the IC model is known or the modules is unreadable,you can use this constructed function
LCDWIKI_KBV mylcd(ILI9486, A3, A2, A1, A0, A4); //model,cs,cd,wr,rd,reset
//if the IC model is not known and the modules is readable,you can use this constructed function
//LCDWIKI_KBV mylcd(320,480,A3,A2,A1,A0,A4);//width,height,cs,cd,wr,rd,reset
void displayBMP(int x, int y, uint32_t address, uint16_t size) {
  int sizeX = pgm_read_word(address + 0);
  int sizeY = pgm_read_word(address + 2);
  int shift = 0;
  uint8_t graph = 0;

  for (int i = 2; i < size; i++) {
    uint16_t value = pgm_read_word(address + i * 2);
    if (value == 0xFD00) {
      i++;
      mylcd.Set_Draw_color(pgm_read_word(address + i * 2));
    } else if (value == 0xFE00) {
      i++;
      for (; pgm_read_word(address + i * 2 ) < 0xF000 && i + 2 < size; i += 2) {
        int pos = pgm_read_word(address + i * 2);
        int sy = pgm_read_word(address + i * 2 + 2);
        mylcd.Fill_Rectangle(x + (pos % (sizeX / 8)) * 8, y + (pos / (sizeX / 8)) , x + (pos % (sizeX / 8)) * 8 + 7, y + (pos / (sizeX / 8)) + sy - 1);
      }
      i--;
    } else if (value >= 0xFF00) {
      graph = value % 256;
      i++;
      for (; pgm_read_word(address + i * 2) < 0xF000 && i < size; i++) {
        int POS = pgm_read_word(address + i * 2);
        int ta = 0, tc = 0;
        for (int j = 0; j < 8; j++) {
          if (graph & (1 << 7 - j))tc++;
          if ((graph & (1 << 7 - j)) != ta) {
            ta = graph & (1 << 7 - j);
            if (!ta) {
              mylcd.Draw_Fast_HLine(x + (POS % (sizeX / 8)) * 8 + j - tc, y + POS / (sizeX / 8), tc);
              tc = 0;
            }

          }
        }
        if (tc > 0)mylcd.Draw_Fast_HLine(x + (POS % (sizeX / 8)) * 8 + 8 - tc, y + POS / (sizeX / 8), tc);

      }
      i--;

    }


  }

}
uint16_t convert_color(int r, int g, int b) {
  uint16_t color;
  color = r / 8;
  color *= 32;
  color += g / 4;
  color *= 16;
  color += b / 8;
  return color;
}
struct Pos {
  int x;
  int y;
};

//define some colour values
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x03FC
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

bool con;
String vst;
Pos tMapPos = { -1, -1};
uint32_t i = 0;
char text[70];
void displayEnglish(int x, int y, uint8_t chara, uint8_t siz) {
  if (chara < 16)return;
  for (int p = 0; p < 5; p++) {
    uint8_t bin = pgm_read_word(Eng_fontbin + (chara - 16) * 5 + p);
    bool ba = 0;
    int bc = 0;
    if (siz == 1) {
      for (int q = 0; q < 8; q++)
      {
        uint8_t a = bin;
        if (a & (1 << q))bc++;
        if ((a & (1 << q)) != ba) {
          ba = a & (1 << q);
          if (!ba) {
            mylcd.Draw_Fast_VLine(x + p , y + q - bc, bc);
            bc = 0;
          }


        }
        //if(bc>0)mylcd.Draw_Fast_VLine(x+p , y+ 8-bc,bc);
      }
    } else {
      for (int q = 0; q < 8; q++)
      {
        if (bin & (1 << q))mylcd.Fill_Rectangle(x + p * siz, y + q * siz, x + p * siz + siz - 1, y + q * siz + siz - 1);
      }
    }
  }
}
void printRom(String t, int x, int y, uint16_t color) {
  printRom(t, x, y, color, 1, -1);
}
void printRom(String t, int x, int y, uint16_t color, uint8_t siz) {
  printRom(t, x, y, color, siz, -1);
}
void printRom(String t, int x, int y, uint16_t color, uint8_t siz, long bgc) {
  if (siz == 0)return;
  char *text;
  uint16_t temp = mylcd.Get_Draw_color();
  mylcd.Set_Draw_color(color);
  text = new char[t.length() + 1];
  t.toCharArray(text, t.length() + 1);
  for (int j = 0; j < t.length(); j++) {
    if (bgc >= 0 && bgc < 65536) {
      mylcd.Set_Draw_color(bgc);
      mylcd.Fill_Rectangle(x + 6 * siz * j, y, x + 6 * siz * j + 5 * siz - 1, y + 7 * siz - 1);
      mylcd.Set_Draw_color(color);
    }
    displayEnglish(x + 6 * siz * j, y, text[j], siz);
  }
  delete[] text;
  mylcd.Set_Draw_color(temp);
}
void displayJapanese(int x, int y, uint16_t chara, uint16_t color) {
  displayJapanese(x, y, chara, color, 1, -1);
}
void displayJapanese(int x, int y, uint16_t chara, uint16_t color, uint8_t siz, long bgcol) {
  uint16_t temp = mylcd.Get_Draw_color();
  bool bgc = 0;
  if (bgcol < 0 || bgcol > 65535) {
    mylcd.Set_Draw_color(color);
    if (siz == 1) {
      for (int j = 0; j < 144; j++)if ((pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Draw_Pixel(x + j % 12, y + j / 12);
    } else {
      for (int j = 0; j < 144; j++)if ((pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Fill_Rectangle(x + (j % 12)*siz, y + (j / 12)*siz, x + (j % 12)*siz + siz - 1, y + (j / 12)*siz + siz - 1);
    }
  } else {
    int cnt = 0;
    for (int j = 0; j < 144; j++)if (pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16))cnt++;
    if (cnt >= 62) {
      mylcd.Set_Draw_color(color);
      mylcd.Fill_Rectangle(x + 0, y , x + 12 * siz - 1, y + 12 * siz - 1);
      bgc = 1;
      mylcd.Set_Draw_color(bgcol);
      if (siz == 1) {
        for (int j = 0; j < 144; j++)if (!(pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Draw_Pixel(x + j % 12, y + j / 12);
      } else {
        for (int j = 0; j < 144; j++)if (!(pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Fill_Rectangle(x + (j % 12)*siz, y + (j / 12)*siz, x + (j % 12)*siz + siz - 1, y + (j / 12)*siz + siz - 1);
      }
    } else {
      mylcd.Set_Draw_color(bgcol);
      mylcd.Fill_Rectangle(x + 0, y , x + 12 * siz - 1, y + 12 * siz - 1);
      bgc = 0;
      mylcd.Set_Draw_color(color);
      if (siz == 1) {
        for (int j = 0; j < 144; j++)if ((pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Draw_Pixel(x + j % 12, y + j / 12);
      } else {
        for (int j = 0; j < 144; j++)if ((pgm_read_word(EQ_fontbin + 9 * chara + j / 16) & (1 << 15 - j % 16)))mylcd.Fill_Rectangle(x + (j % 12)*siz, y + (j / 12)*siz, x + (j % 12)*siz + siz - 1, y + (j / 12)*siz + siz - 1);
      }
    }

  }
  //mylcd.Set_Draw_color(bgc?bgcol:color);

  mylcd.Set_Draw_color(temp);
}
void displayJapaneseWord(int x, int y, uint16_t wordType, uint16_t color) {
  displayJapaneseWord(x, y, wordType, color, 1, -1);
}
void displayJapaneseWord(int x, int y, uint16_t wordType, uint16_t color, uint8_t siz, long bgcol) {
  if (siz == 0)return;

  for (int p = 0; p < 24 && pgm_read_word(((pgm_read_word(&mem_WordSet[wordType]))) + p * 2) != 0xFFFF; p++) {
    displayJapanese(x + (14 * p)*siz, y, pgm_read_word(((pgm_read_word(&mem_WordSet[wordType]))) + p * 2), color, siz, bgcol);
  }
}

void shindo(int x, int y, int siz) {
  if (!(siz > 0 && siz < 10))return;
  mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 4));
  mylcd.Fill_Rectangle(x - 7, y - 7, x + 7, y + 7);
  mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz));
  mylcd.Draw_Line(x - 7, y - 7, x + 7, y - 7);
  mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 1));
  mylcd.Draw_Line(x - 7, y - 7, x - 7, y + 7);
  mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 2));
  mylcd.Draw_Line(x + 7, y + 7, x + 7, y - 7);
  mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 3));
  mylcd.Draw_Line(x + 7, y + 7, x - 7, y + 7);
  if (siz == 9) {
    mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz));
    mylcd.Draw_Line(x - 6, y - 6, x + 6, y - 6);
    mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 1));
    mylcd.Draw_Line(x - 6, y - 6, x - 6, y + 6);
    mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 2));
    mylcd.Draw_Line(x + 6, y + 6, x + 6, y - 6);
    mylcd.Set_Draw_color(pgm_read_word(JPN_shindo_color + 6 * siz + 3));
    mylcd.Draw_Line(x + 6, y + 6, x - 6, y + 6);
  }

  if (siz >= 5 && siz <= 8) {
    switch (siz) {
      case 5:
        printRom("5-", x - 5, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
        break;
      case 6:
        printRom("5+", x - 5, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
        break;
      case 7:
        printRom("6-", x - 5, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
        break;
      case 8:
        printRom("6+", x - 5, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
        break;
    }

  } else {
    char sindo[3];
    if (siz < 5) {
      sprintf(sindo, "%d", siz);
      printRom(sindo, x - 2, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
    } else {
      sprintf(sindo, "%d", siz - 2, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
      printRom(sindo, x - 2, y - 3, pgm_read_word(JPN_shindo_color + 6 * siz + 5));
    }
  }

}
bool isDrawMap = 0;
uint16_t drawMap() {

  isDrawMap = 1;
  mylcd.Fill_Screen(CYAN);
  mylcd.Vert_Scroll(0, mylcd.Get_Display_Height(), 0);
  mylcd.Set_Draw_color(GREEN);
  uint32_t start = millis();

  uint8_t a = 0;
  for (int m = 0; m < 1393; m++) {
    uint16_t POS = pgm_read_word(JPN_map_v3_bin + m);
    if (POS >= 0xFF00) {
      a = POS + 256;
      continue;
    }
    bool ta = 0;
    uint8_t tc = 0;
    /*for (int j = 0; j < 8; j++) {
      if (a & (1 << 7 - j)) {
        mylcd.Draw_Pixel(POS % 40 * 8 + j, POS / 40);
      }
      }*/
    for (int j = 0; j < 8; j++) {
      if (a & (1 << 7 - j))tc++;
      if ((a & (1 << 7 - j)) != ta) {
        ta = a & (1 << 7 - j);
        if (!ta) {
          mylcd.Draw_Fast_HLine(POS % 40 * 8 + j - tc, POS / 40, tc);
          tc = 0;
        }

      }
    }
    if (tc > 0)mylcd.Draw_Fast_HLine(POS % 40 * 8 + 8 - tc, POS / 40, tc);

  }
  for (int m = 0; m < 69; m++) {
    uint16_t POS = (pgm_read_word(JPN_map_v2_pnt + (m * 3)) + 256) % 256;
    POS *= 256;
    POS += (pgm_read_word(JPN_map_v2_pnt + (m * 3) + 1) + 256) % 256;
    uint8_t a = pgm_read_word(JPN_map_v2_pnt + (m * 3) + 2);
    mylcd.Fill_Rectangle(POS % 40 * 8, POS / 40 + 1, POS % 40 * 8 + 7, POS / 40 - a);
  }

  mylcd.Set_Draw_color(WHITE);
  mylcd.Draw_Line(0, 208, 98, 208);
  mylcd.Draw_Line(98, 208, 170, 136);
  mylcd.Draw_Line(170, 51, 170, 136);
  mylcd.Set_Draw_color(0x001F);
  mylcd.Fill_Rectangle(0, 472, 320, 480);
  mylcd.Set_Text_Back_colour(0x001F);
  mylcd.Set_Text_colour(WHITE);
  char versionText[50];

  sprintf(versionText, "MisakiEQ For Arduino Ver.%d.%d.%d ", version_Major, version_Minor, version_Maintenance);
  vst = versionText;

  printRom(versionText, 160 - 3 * vst.length(), 473, 0xffff);
  return millis() - start;
}
void drawCenter(long x, long y) {
  long ix, iy;
  ix = (204 * x) - 264400;
  iy = (-250 * y) + 115000;
  mylcd.Set_Draw_color(RED);
  bool wrote = 0;
  ix = ix / 100;
  iy = iy / 100;
  if (tMapPos.x != -1 || tMapPos.y != -1) {
    for (int gy = tMapPos.y - 7; gy <= tMapPos.y + 7; gy++) {
      for (int gx = tMapPos.x - 7; gx <= tMapPos.x + 7; gx++) {
        if (gx < 0 || gy < 0 || gx > 320 || gy > 480)continue;
        if (pgm_read_word(JPN_map_bmp + gx / 8 + gy * 40) & (1 << 7 - gx % 8)) {
          mylcd.Set_Draw_color(GREEN);
          mylcd.Draw_Pixel(gx, gy);
        } else {
          mylcd.Set_Draw_color(CYAN);
          mylcd.Draw_Pixel(gx, gy);
        }
      }

    }
    mylcd.Set_Draw_color(WHITE);
    mylcd.Draw_Line(0, 208, 98, 208);
    mylcd.Draw_Line(98, 208, 170, 136);
    mylcd.Draw_Line(170, 51, 170, 136);
    tMapPos.x = -1;
    tMapPos.y = -1;
  }
  //char t[64];
  //sprintf(t,"X:%5ld Y:%5ld W:%4d N:%4d",ix,iy,x,y);
  //Serial.println(t);
  mylcd.Set_Draw_color(RED);
  if (!((ix < 98 && iy < 209) || (ix >= 98 && ix <= 170 && iy < 209 - (ix - 98))) && ix >= 0 && iy <= 480) {

    mylcd.Draw_Line(ix - 5, iy - 7, ix + 7, iy + 5);
    mylcd.Draw_Line(ix - 6, iy - 7, ix + 7, iy + 6);
    mylcd.Draw_Line(ix - 6, iy - 6, ix + 6, iy + 6);
    mylcd.Draw_Line(ix - 7, iy - 6, ix + 6, iy + 7);
    mylcd.Draw_Line(ix - 7, iy - 5, ix + 5, iy + 7);
    mylcd.Draw_Line(ix - 5, iy + 7, ix + 7, iy - 5);
    mylcd.Draw_Line(ix - 6, iy + 7, ix + 7, iy - 6);
    mylcd.Draw_Line(ix - 6, iy + 6, ix + 6, iy - 6);
    mylcd.Draw_Line(ix - 7, iy + 6, ix + 6, iy - 7);
    mylcd.Draw_Line(ix - 7, iy + 5, ix + 5, iy - 7);
    wrote = 1;
    tMapPos.x = ix;
    tMapPos.y = iy;
  }
  if (!wrote) {
    ix += 145;
    iy -= 351;
    if ((ix < 98 && iy < 209) || (ix >= 98 && ix <= 170 && iy < 209 - (ix - 98)) && iy > 51) {
      mylcd.Draw_Line(ix - 5, iy - 7, ix + 7, iy + 5);
      mylcd.Draw_Line(ix - 6, iy - 7, ix + 7, iy + 6);
      mylcd.Draw_Line(ix - 6, iy - 6, ix + 6, iy + 6);
      mylcd.Draw_Line(ix - 7, iy - 6, ix + 6, iy + 7);
      mylcd.Draw_Line(ix - 7, iy - 5, ix + 5, iy + 7);
      mylcd.Draw_Line(ix - 5, iy + 7, ix + 7, iy - 5);
      mylcd.Draw_Line(ix - 6, iy + 7, ix + 7, iy - 6);
      mylcd.Draw_Line(ix - 6, iy + 6, ix + 6, iy - 6);
      mylcd.Draw_Line(ix - 7, iy + 6, ix + 6, iy - 7);
      mylcd.Draw_Line(ix - 7, iy + 5, ix + 5, iy - 7);
      tMapPos.x = ix;
      tMapPos.y = iy;
    }
  }

}
void clearConsole() {
  i = 0;
  mylcd.Fill_Screen(0x0000);
  mylcd.Vert_Scroll(0, mylcd.Get_Display_Height(), 0);
}
uint32_t serialSpeed = 19200;
uint32_t serialTimeout = 10;
void printConsole(String ctext) {
  char *text;
  text = new char[ctext.length() + 1];
  ctext.toCharArray(text, ctext.length() + 1);
  i += 8;
  if (i > 480) {
    /*for (int p = 7; p >= 0; p--) {
      mylcd.Vert_Scroll(0, mylcd.Get_Display_Height(), (i - p - 1) % 480);
      /*for(int x=0;x<360;x++){
        if(my_lcd.Read_Pixel(x,(i - p - 1) % 480)>0) my_lcd.Draw_Pixel(j*PIXEL_NUMBER+l,i);
      }*/

    //my_lcd.Draw_Pixel(j*PIXEL_NUMBER+l,i);

    /* mylcd.Draw_Line(0, (i - p - 1) % 480, 359, (i - p - 1) % 480);
      }*/
    mylcd.Set_Draw_color(BLACK);
    mylcd.Fill_Rectangle(0, i % 480 == 0 ? 481 : i % 480 + 1, 359, (i - 8) % 480);
    mylcd.Vert_Scroll(0, mylcd.Get_Display_Height(), i % 480);
    //mylcd.Print_String("                                                      |", 0, (i-8)%480);
  }
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_colour(WHITE);
  uint16_t cid = 0xffff;
  char buckup[64];
  int col = 0;
  int skip = 0;
  int x = 0;
  int fx = 0;
  char he[2];
  for (int len = 0; text[len] != NULL || len < ctext.length(); len++) {
    if (text[len] == '|' && text[len + 1] == '*') {

      he[0] = text[len + 2]; he[1] = 0;
      sscanf(he, "%x", &col);
      buckup[skip] = 0;

      printRom(buckup, fx * 6, (i - 8) % 480, cid);
      fx += x;
      x = 0;
      skip = 0;
      //sprintf(buckup,"color is %4x get %x",convert_color(pgm_read_word(char_col + 3 * col), pgm_read_word(char_col + 3 * col + 1), pgm_read_word(char_col + 3 * col + 2)),col)
      //;//mylcd.Set_Text_colour(WHITE);
      //mylcd.Print_String(buckup, fx * 6, (i ) % 480);
      cid = pgm_read_word(char_col +  col);
      //mylcd.Set_Text_colour(pgm_read_word(char_col +  col));
      len += 2;
      if (text[len ] == NULL || len >= ctext.length())break;
    } else {
      buckup[skip] = text[len];
      skip++;
      x++;
    }


  }
  buckup[skip] = 0;

  printRom(buckup, fx * 6, (i - 8) % 480, cid);
  delete[] text;
}
char logText[64];
struct timeStamp {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minutes;
  uint8_t seconds;
};
int isDisplayedUI = 0;
void displayUI(bool isEQW) {
  if (isDisplayedUI != isEQW + 1) {
    isDisplayedUI = isEQW + 1;
    drawMap();
    if (isEQW) {
      mylcd.Set_Draw_color(0xF800);
      mylcd.Fill_Rectangle(0, 0, 14 * 6 * 2 + 4, 16 * 2);
      mylcd.Set_Draw_color(0x0000);
      mylcd.Fill_Rectangle(0, 426, 320, 471);
      mylcd.Fill_Rectangle(215, 383, 319, 426);
      displayJapaneseWord(4, 4, 366, 0xFFFF, 2, 0xF800);
      displayJapaneseWord(160, 459, 378, 0xFFFF);
      displayJapanese(307, 459, 0x115, 0xFFFF);
      displayJapaneseWord(0, 443, 368, 0xFFFF);
      displayJapaneseWord(0, 459, 367, 0xFFFF);
      displayJapaneseWord(0, 427, 369, 0xFFFF);
      mylcd.Set_Draw_color(0x001F);
      mylcd.Fill_Rectangle(0, 32, 14 * 6 * 2 + 4, 46);
      displayJapaneseWord(0, 33, 370, 0xFFFF, 1, 0x001F);

      displayJapaneseWord(216, 385, 371, 0xFFFF);
      displayJapaneseWord(216, 418, 372, 0xFFFF);

      mylcd.Fill_Rectangle(0, 47, 14 * 4 * 2 + 4 + 6, 63);
      displayJapanese(1, 51, 0x122, 0xFFFF, 1, -1);
      displayJapanese(41, 51, 0x121, 0xFFFF, 1, -1);
      printRom(":", 42, 426, 0xFFFF, 2);
      printRom(":M", 70, 442, 0xFFFF, 2);
      printRom(":   km", 70, 458, 0xFFFF, 2);



    } else {
      mylcd.Set_Draw_color(0x001F);
      mylcd.Fill_Rectangle(0, 0, 14 * 4 * 2 + 4, 16 * 2);
      displayJapaneseWord(4, 4, 384, 0xFFFF, 2, 0x001F);
    }
  }

}
long Time = 0;
long tempTime = 0;
void quickEQW(String str) {
  //printConsole(str);
  displayUI(1);
  /*  FFFFF 日付 2年1月2日         3-7
      FFFFF 時刻                  8-12
      FF 上位ビット1 最終報 他nn報  13-14
      FFF 震源地ID                15-17
      FF マグニチュード            18-19
      FFF 震源の深さ              20-22
      FFF 東経                   23-25
      FFF 北緯                   26-28
      F 最大震度                  29
      F 地域震度                  30
      FFFFFFFF カウントダウン(ミリ秒)31-38

   * */
  char txt[64
          ];
  char tmp[20];
  str.toCharArray(txt, 63);
  uint8_t hou;
  uint16_t y, z;
  uint16_t a, b, c;
  uint32_t p;
  sprintf(tmp, "%c%c", txt[13], txt[14]);
  sscanf(tmp, "%x", &hou);
  if (hou % 128 > 99)hou = 99 + (hou / 128 * 128);
  sprintf(tmp, "%2d", hou % 128);
  printRom(tmp, 16, 49, 0xFFFF, 2, 0x001F);             //第XX報
  displayJapaneseWord(55, 51, 376, hou / 128 ? 0xFFFF : 0x001F, 1, 0x001F); //最終報
  sprintf(tmp, "%c%c", txt[3], txt[4]);
  sscanf(tmp, "%x", &a);
  sprintf(tmp, "%c", txt[5]);
  sscanf(tmp, "%x", &b);
  sprintf(tmp, "%c%c", txt[6], txt[7]);
  sscanf(tmp, "%x", &c);
  sprintf(tmp, "%c%c%c%c%c", text[8], text[9], text[10], text[11], text[12]);
  sscanf(tmp, "%lx", &p);
  sprintf(tmp, "%d/%02d/%02d %02ld:%02ld:%02ld", 2000 + a, b, c, p / 3600, p / 60 % 60, p % 60);
  printRom(tmp, 14 * 4 + 1, 33 + 5, 0xFFFF, 1, 0x001F); //発生時刻
  sprintf(tmp, "%c%c", txt[18], txt[19]);
  sscanf(tmp, "%x", &a);
  sprintf(tmp, "%2d.%d", a / 10, a % 10);
  printRom(tmp, 94, 442, 0xFFFF, 2, 0x0000);          //マグニチュード
  sprintf(tmp, "%c%c%c", txt[20], txt[21], txt[22]);
  sscanf(tmp, "%x", &y);
  sprintf(tmp, "%3d", y % 1000);
  printRom(tmp, 82, 458, 0xFFFF, 2, 0x0000);           //震源の深さ
  sprintf(tmp, "%c%c%c", txt[15], txt[16], txt[17]);
  sscanf(tmp, "%x", &y);
  if (y >= 366)y = 373;
  mylcd.Set_Draw_color(0x0000);
  mylcd.Fill_Rectangle(54, 427, 54 + 14 * 10, 438);
  displayJapaneseWord(54, 427, y, 0xFFFF, 1, 0x0000);    //震源地
  mylcd.Fill_Rectangle(288, 383, 320, 397);
  sprintf(tmp, "%c", txt[29]);
  sscanf(tmp, "%x", &a);
  sprintf(tmp, "%c", txt[30]);
  sscanf(tmp, "%x", &b);
  if (a < 5 || a == 9) {
    sprintf(tmp, "%d", a < 5 ? a : 7);
    printRom(tmp, 292, 383, 0xFFFF, 2, 0x0000);            //最大震度
  } else if (a >= 5 && a < 9) {
    sprintf(tmp, "%d", a < 7 ? 5 : 6);
    printRom(tmp, 292, 383, 0xFFFF, 2, 0x0000);            //最大震度
    displayJapanese(304, 385, 0x11A + ((a + 1) % 2), 0xFFFF, 1, 0x0000); //最大震度 漢字
  } else {
    displayJapaneseWord(290, 385, 373, 0xFFFF, 1, 0);
  }
  mylcd.Fill_Rectangle(266, 402, 320, 430);
  if (b < 5 || b == 9) {
    sprintf(tmp, "%d", b < 5 ? b : 7);
    printRom(tmp, 270, 402, 0xFFFF, 4, 0x0000);            //地域震度
  } else if (b >= 5 && b < 9) {
    sprintf(tmp, "%d", b < 7 ? 5 : 6);
    printRom(tmp, 270, 402, 0xFFFF, 4, 0x0000);            //地域震度
    displayJapanese(294, 406, 0x11A + ((b + 1) % 2), 0xFFFF, 2, 0x0000); //地域震度 漢字
  } else {
    displayJapaneseWord(266, 406, 373, 0xFFFF, 2, 0);
  }
  sprintf(tmp, "%c%c%c", text[23], text[24], text[25]);
  sscanf(tmp, "%x", &y);
  // Serial.println(y,DEC);
  sprintf(tmp, "%c%c%c", text[26], text[27], text[28]);
  //Serial.println(" ");
  sscanf(tmp, "%x", &z);
  //Serial.println(z,DEC);
  drawCenter(y, z);
  sprintf(tmp, "%c%c%c%c%c%c%c%c", text[31], text[32], text[33], text[34], text[35], text[36], text[37], text[38]);
  sscanf(tmp, "%lx", &Time);






}
void displayBinary(int address) { //-1=ALL

  int startAddress = address;
  int EndAddress = address + 60;
  if (address == -1) {
    startAddress = 0;
    EndAddress = 16384;
  } else {
    if (address < -1 || address >= 16384)return;
  }
  if (EndAddress > 16384)EndAddress = 16384;
  for (uint32_t stv = startAddress; stv < EndAddress; stv++) {
    char pr[3 + 4 + 5 * 16 + 4 * 16 + 4];
    sprintf(pr, "|*a%04X", stv);
    for (int miniAddress = 0; miniAddress < 16; miniAddress++) {
      uint8_t valueChar = pgm_read_byte_far(stv * 16 + miniAddress);
      sprintf(pr, "%s|*%c%02X", pr, miniAddress % 2 ? '7' : 'f', valueChar);
    }
    sprintf(pr, "%s|*9", pr);
    for (int miniAddress = 0; miniAddress < 16; miniAddress++) {
      uint8_t valueChar = pgm_read_byte_far(stv * 16 + miniAddress);
      if (valueChar < 0x20)valueChar = '.';
      sprintf(pr, "%s|*9%c", pr, valueChar);
    }
    printConsole(pr);
  }
}
bool outputCons = 0;
void serialEvent() {
  while (Serial.available()) {
    String getText;
    getText = Serial.readStringUntil('\n');
    sprintf(text, "");
    getText.toCharArray(text, 70);
    int o = 0;
    uint32_t value;
    int x, y, s;
    char t[2];

    if (text[0] == '/') {
      switch (text[1]) {
        case 'i':
        case 'I':


          sscanf(text, "%s %s %ld", NULL, NULL, &value);
          switch (text[3]) {
            case 's':
            case 'S':

              if (value == 0 || value > 1000000) {
                sprintf(logText, "Now serial speed is |*a%ld", serialSpeed);
                printConsole(logText);
              } else {
                serialSpeed = value;
                Serial.end();
                Serial.begin(serialSpeed);
                sprintf(logText, "Changed Serial speed set to |*c%ld", serialSpeed);
                printConsole(logText);
              }
              break;
            case 't':
            case 'T':
              if (value > 1000) {
                sprintf(logText, "Now serial timeout is |*a%ld |*fms", serialTimeout);
                printConsole(logText);
              } else {
                serialTimeout = value;
                Serial.setTimeout(serialTimeout);
                sprintf(logText, "Changed serial timeout set to |*c%ld |*fms", serialTimeout);
                printConsole(logText);
              }
              break;
          }
          break;
        case 's':
        case 'S':
          i = 480;
          drawMap();
          break;
        case 'd':
        case 'D':

          for (int q = 0; q < 47; q++) {
            t[0] = text[3 + q]; t[1] = 0;
            sscanf(t, "%x", &s);
            shindo(pgm_read_word(JPN_shindo_chiten_pos + q * 2), pgm_read_word(JPN_shindo_chiten_pos + q * 2 + 1), s);
          }
          break;
        case 'c':
        case 'C':
          Time = -3000000;
          tempTime = 0;
          isDisplayedUI = -1;
          clearConsole();
          break;
        case 'Q':
        case 'q':
          quickEQW(text);
          break;
        case 'b':
        case 'B':

          sscanf(text, "%s %d", NULL, &o);
          if (o >= 23 && o <= 108) {
            tone(47, pgm_read_word(toHz + o - 21)) ;
          } else {
            noTone(47);
          }
          break;
        case 'T':
        case 't':
          int address = 0;
          sscanf(text, "%s %x", NULL, &address);
          displayBinary(address);
          /*uint32_t test=Misaki_bmp;
            for(int y=0;y<2044;y+=8){
            char txt[64];
            sprintf(txt, "");
            for(int x=0;x<8;x++){

              sprintf(txt,"%s %04X",txt,pgm_read_word(test+(y+x)*2));
            }
            printConsole(txt);
            }*/
          //clearConsole();

          //displayUI(text[3] == '1');

          break;

        default:
          if (Time < -30000)printConsole(text);
      }
    } else if (text[0] == '!') {
      if (text[1] == 'E' && text[2] == 'Q' && text[3] == 'W' && text[4] == ' ') {
        switch (text[5]) {
          case 'e':
          case 'E':
            for (int p = 7; p < 100; p++)text[p - 7] = text[p];
            long x, y;

            sscanf(text, "%ld %ld", &x, &y);
            drawCenter(x, y);
            break;
        }


      }
    } else  {
      if (Time < -30000)printConsole(text);
    }
  }

}
void setup()
{

  Time = -10000000;
  Serial.begin(serialSpeed);
  mylcd.Init_LCD();
  //Serial.println(mylcd.Read_ID(), HEX);
  mylcd.Fill_Screen(BLACK);
  Serial.setTimeout(serialTimeout);
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_Size(1);
  mylcd.Set_Draw_color(BLACK);
  pinMode(10, OUTPUT);

  char text[70];
  sprintf(text, "MisakiEQ For Arduino Ver.%d.%d.%d", version_Major, version_Minor, version_Maintenance);

  printConsole(text);
  //printConsole(" ");

  uint16_t Year = 0;
  uint8_t Month, Day;
  char Date[13] = __DATE__;
  if (Date[4] == '1')Day = 10;
  Day += Date[5] - '0';
  for (int j = 0; j < 4; j++) {
    Year *= 10;
    Year += Date[7 + j] - '0';
  }
  for (Month = 0; Month < 12; Month++) {
    char buffer[4];
    strcpy_P(buffer, (char*)pgm_read_word(&(month_Word[Month])));
    if (Date[0] == buffer[0] && Date[1] == buffer[1] && Date[2] == buffer[2]) {
      Month++;
      break;
    }
  }


  sprintf(text, "Last Build Time:|*a%4d/%02d/%02d %s", Year, Month, Day, __TIME__);

  printConsole(text);
  printConsole(" ");
  sprintf(text, "|*bTwitter|*f:@|*b0x7FF");
  printConsole(text);
  sprintf(text, "|*cYouTube|*f:bit.ly/|*cKanon_YouTube");
  printConsole(text);
  printConsole(" ");
  sprintf(text, "[|*bMisaki|*fEQ for Arduino]'s |*7GitHub|*f:");
  printConsole(text);
  sprintf(text, "github.com/|*bMisaki0331");
  printConsole(text);
  sprintf(text, "            /|*bMisakiEQ_For_Arduino");

  printConsole(text);
  printConsole(" ");
  sprintf(text, "(C)2021 Misaki All rights reserved");
  printConsole(text);


  displayBMP(207, 0, Misaki_bmp, 2095);
  pinMode(47, OUTPUT);
  printConsole(" ");
  printRom("Initializing SD Card...", 0, 96, 0xFFFF, 1, 0) ;
  
  if (mySD.begin(10,11,12,13)) {
    isSDReady = 1;
    uint32_t volumesize;
  volumesize = mySD.volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= mySD.volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;
  printRom("                        ", 0, 96, 0xFFFF, 1, 0) ;
  uint32_t s=volumesize/1024%1024*1000/1024;
     sprintf(text," SD: |*a%ld.%03ld|*f MB in Volume",volumesize/1024,s);
    printConsole(text);
    
  } else {
    printRom("                        ", 0, 96, 0xFFFF, 1, 0) ;
    printConsole(" SD:|*9Error |*f- Not Available");
  }
  uint32_t free_Flash = 0;
  for (uint32_t Address = 0x3DFFF;; Address--) {
    if (pgm_read_byte_far(Address) == 255) {
      free_Flash++;
    } else {
      break;
    }
  }
  sprintf(text, "ROM: |*a%6ld|*f Bytes (|*a%3ld|*fKB) Free", free_Flash, free_Flash / 1024);
  printConsole(text);
  int aRamStart = 0x0100;                   // RAM先頭アドレス（固定値）
  int aGvalEnd;                             // グローバル変数領域末尾アドレス
  int aHeapEnd;                             // ヒープ領域末尾アドレス(次のヒープ用アドレス）
  int aSp;                                  // スタックポインタアドレス（次のスタック用アドレス）
  char aBuff[6];                            // 表示フォーマット操作バッファ
  int8_t *heapptr, *stackptr;
  stackptr = (uint8_t *)malloc(4);        // とりあえず4バイト確保
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);                         // 確保したメモリを返却
  stackptr =  (uint8_t *)(SP);            // SPの値を保存（SPには次のスタック用の値が入っている）
  aSp = (int)stackptr;                    // スタックポインタの値を記録
  aHeapEnd = (int)heapptr;                // ヒープポインタの値を記録
  aGvalEnd = (int)__malloc_heap_start - 1; // グローバル変数領域の末尾アドレスを記録
  uint32_t usageP = (RAMEND - aRamStart);
  usageP = (usageP - (aSp - aHeapEnd + 1))*10000/ usageP;

  sprintf(text, "RAM: |*a%4d |*fBytes Free (Usage:|*a%3ld.%02ld|*f%%)", aSp - aHeapEnd + 1, usageP/100,usageP%100);
  printConsole(text);
  for (int j = 0; j < 3; j++)printConsole(" ");

}
uint32_t TimeLoop = 0;
uint32_t temM = 0;
void loop()
{
  temM++;

  uint32_t tempT = millis();

  if (Time >= -100000)Time -= tempT - TimeLoop;
  if (Time < -30000 && Time > -100000) {
    clearConsole();
    isDisplayedUI = -1;
    i = 0;
    Time = -300000;
  }
  TimeLoop = tempT;
  //uint32_t timer=millis();
  uint32_t c = Time / 1000 + 1;
  if (Time <= 0)c = 0;
  if (tempTime != c && Time > -1000) {
    tempTime = c;
    if (c < 1000)sprintf(logText, "%3d", c);
    if (c >= 1000)sprintf(logText, "???");
    printRom(logText, 216, 435, 0xF800, 5, 0x0000);          //到達時刻まで

    //sprintf(logText,"%7ldFPS",temM);
    //printRom(logText,254,473,0xFFFF,1,0x001F);
    outputCons = 0;
    temM = 0;
  } else {
    outputCons = 1;
  }
  if (Time == 0)temM = 0;
  mylcd.Set_Text_Mode(0);
  //display 1 times string
  //mylcd.Fill_Screen(0x0000);

  //sprintf(text,"%ld:%02ld.%03ld",timer/60000,timer/1000%60,timer%1000);


  //delay(100);
}
