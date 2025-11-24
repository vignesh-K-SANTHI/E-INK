///////////////////////////////////////////STM32F746ZGT6////////////////////////////////////////////////////////////////////////////////////
#include "main.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include"FreeSerif10pt7b.h"
#include"FreeSerif15pt7b.h"
#include"FreeSerif20pt7b.h"
#include"FreeSerif25pt7b.h"
#include"FreeSerif30pt7b.h"
#include"FreeSerif35pt7b.h"
#include"FreeSerif40pt7b.h"
#include"FreeSerif45pt7b.h"
#include"FreeSerif50pt7b.h"
#include"FreeSerif55pt7b.h"
#include"FreeSerif60pt7b.h"
#include"FreeSerif65pt7b.h"
#include"FreeSerif70pt7b.h"
int counter=0;
const char* text;
const GFXfont* autoFont;
extern const GFXfont FreeSerif10pt7b;
extern const GFXfont FreeSerif15pt7b;
extern const GFXfont FreeSerif20pt7b;
extern const GFXfont FreeSerif25pt7b;
extern const GFXfont FreeSerif30pt7b;
extern const GFXfont FreeSerif35pt7b;
extern const GFXfont FreeSerif40pt7b;
extern const GFXfont FreeSerif45pt7b;
extern const GFXfont FreeSerif50pt7b;
extern const GFXfont FreeSerif55pt7b;
extern const GFXfont FreeSerif60pt7b;
extern const GFXfont FreeSerif65pt7b;
extern const GFXfont FreeSerif70pt7b;

const GFXfont* FONT_LIST[] = {
    &FreeSerif10pt7b,
	&FreeSerif15pt7b,
    &FreeSerif20pt7b,
	&FreeSerif25pt7b,
    &FreeSerif30pt7b,
	&FreeSerif35pt7b,
    &FreeSerif40pt7b,
	&FreeSerif45pt7b,
    &FreeSerif50pt7b,
	&FreeSerif55pt7b,
	&FreeSerif60pt7b,
	&FreeSerif65pt7b,
	&FreeSerif70pt7b
};
const int FONT_COUNT = sizeof(FONT_LIST)/sizeof(FONT_LIST[0]);

//int CHAR_SPACING = 0;    // extra pixels between each character
//int WORD_SPACING = 5;    // extra pixels between words
//int LINE_SPACING = 0;    // extra pixels added below each wrap line

// ================= Orientation control =================
typedef enum {
    ORIENT_0,
    ORIENT_90,
    ORIENT_180,
    ORIENT_270
} EPD_Orientation;

// Global variable for current display orientation
static EPD_Orientation epd_orientation = ORIENT_0;

/* ================= E-INK pins mapping (GPIOE) ================= */
#define EPD_BUSY_PIN   GPIO_PIN_10
#define EPD_RESET_PIN  GPIO_PIN_12
#define EPD_DC_PIN     GPIO_PIN_14
#define EPD_CS_PIN     GPIO_PIN_15
#define EPD_PORT       GPIOE

/* Helper macros */
#define EPD_CS_LOW()    HAL_GPIO_WritePin(EPD_PORT, EPD_CS_PIN, GPIO_PIN_RESET)
#define EPD_CS_HIGH()   HAL_GPIO_WritePin(EPD_PORT, EPD_CS_PIN, GPIO_PIN_SET)
#define EPD_DC_CMD()    HAL_GPIO_WritePin(EPD_PORT, EPD_DC_PIN, GPIO_PIN_RESET)
#define EPD_DC_DATA()   HAL_GPIO_WritePin(EPD_PORT, EPD_DC_PIN, GPIO_PIN_SET)
#define EPD_RST_LOW()   HAL_GPIO_WritePin(EPD_PORT, EPD_RESET_PIN, GPIO_PIN_RESET)
#define EPD_RST_HIGH()  HAL_GPIO_WritePin(EPD_PORT, EPD_RESET_PIN, GPIO_PIN_SET)
#define EPD_BUSY_READ() (HAL_GPIO_ReadPin(EPD_PORT, EPD_BUSY_PIN) == GPIO_PIN_SET) // Busy LOW


#define EPD_WIDTH   800
#define EPD_HEIGHT  272

#define Source_BYTES    400/8
#define Gate_BITS  272
#define ALLSCREEN_BYTES   Source_BYTES*Gate_BITS

/* SPI handle */
SPI_HandleTypeDef hspi1;

/* ------------------ SPI helpers ------------------ */
static void SPI_Write(uint8_t value)
{
    HAL_SPI_Transmit(&hspi1, &value, 1, HAL_MAX_DELAY);
}

static void delay_xms(uint32_t ms)
{
	HAL_Delay(ms);
}

/* ------------------ EPD low-level commands ------------------ */
static void EPD_W21_WriteCMD(uint8_t cmd)
{
	EPD_CS_LOW();
	EPD_DC_CMD();
	SPI_Write(cmd);
	EPD_CS_HIGH();
}
static void EPD_W21_WriteDATA(uint8_t dat)
{
	EPD_CS_LOW();
	EPD_DC_DATA();
	SPI_Write(dat);
	EPD_CS_HIGH();
}
int val = 0;
static void EPD_WaitUntilIdle(void)
{
    while (EPD_BUSY_READ()){
    	val = 1;
    }

    HAL_Delay(10);
}

void EPD_DeepSleep(void)
{
  EPD_W21_WriteCMD(0x10); //enter deep sleep
  EPD_W21_WriteDATA(0x01);
  delay_xms(100);
}

static void Set_ramMP(void)
{
	EPD_W21_WriteCMD(0x11);
	EPD_W21_WriteDATA(0x03);

	EPD_W21_WriteCMD(0x44);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x31);

    EPD_W21_WriteCMD(0x45);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x2B);
    EPD_W21_WriteDATA(0x01);
}

static void Set_ramSP(void)
{

	EPD_W21_WriteCMD(0x91);
	EPD_W21_WriteDATA(0x04);

	EPD_W21_WriteCMD(0xc4);
	EPD_W21_WriteDATA(0x30);
	EPD_W21_WriteDATA(0x00);

	EPD_W21_WriteCMD(0xc5);
	EPD_W21_WriteDATA(0x0f);
	EPD_W21_WriteDATA(0x01);
	EPD_W21_WriteDATA(0x00);
   EPD_W21_WriteDATA(0x00);
}
static void Set_ramMA(void)
{

	EPD_W21_WriteCMD(0x4e);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0x4f);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);
}



static void Set_ramSA(void)
{

	EPD_W21_WriteCMD(0xce);
	EPD_W21_WriteDATA(0x31);
	EPD_W21_WriteCMD(0xcf);
	EPD_W21_WriteDATA(0x0f);
	EPD_W21_WriteDATA(0x01);
}

static void EPD_HW_TNIT()
{
	EPD_RST_LOW();
	delay_xms(10);
	EPD_RST_HIGH();
	delay_xms(10);

	 EPD_WaitUntilIdle();
	 EPD_W21_WriteCMD(0x12);  //SWRESET
	 EPD_WaitUntilIdle();
}
static void EPD_Update()
{
	EPD_W21_WriteCMD(0x22);
	EPD_W21_WriteDATA(0xF7);
	EPD_W21_WriteCMD(0x20);
	EPD_WaitUntilIdle();
}

static void EPD_Update_Fast()
{
	EPD_W21_WriteCMD(0x22);
	EPD_W21_WriteDATA(0xC7);
	EPD_W21_WriteCMD(0x20);
	EPD_WaitUntilIdle();
}

/* Fast update initialization (partial LUT) */
static void EPD_HW_Init_Fast(void)
{
    EPD_RST_LOW();
    delay_xms(10);
    EPD_RST_HIGH();
    delay_xms(10);

    EPD_W21_WriteCMD(0x12); // SWRESET
    EPD_WaitUntilIdle();

    EPD_W21_WriteCMD(0x18); // Temperature Sensor
    EPD_W21_WriteDATA(0x80);

    EPD_W21_WriteCMD(0x22); // Display Update Control
    EPD_W21_WriteDATA(0xB1);

    EPD_W21_WriteCMD(0x20); // Master Activation
    EPD_WaitUntilIdle();

    EPD_W21_WriteCMD(0x1A); // Set dummy line
    EPD_W21_WriteDATA(0x64);
    EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0x22); // Display Update Control 2
    EPD_W21_WriteDATA(0x91);

    EPD_W21_WriteCMD(0x20); // Master Activation
    EPD_WaitUntilIdle();
}

static void EPD_WhiteScreen_ALL_Fast1(const unsigned char *datas)
{
	unsigned int i;
    unsigned char tempOriginal;
    unsigned int tempcol=0;
    unsigned int templine=0;

   EPD_RST_LOW();
   delay_xms(10);
   EPD_RST_HIGH();

   EPD_W21_WriteCMD(0x12); // SWRESET
   EPD_WaitUntilIdle();

   EPD_W21_WriteCMD(0x18); // Temperature Sensor
   EPD_W21_WriteDATA(0x80);

   EPD_W21_WriteCMD(0x22); // Display Update Control
   EPD_W21_WriteDATA(0xB1);

   EPD_W21_WriteCMD(0x20); // Master Activation
   EPD_WaitUntilIdle();

   EPD_W21_WriteCMD(0x1A); // Set dummy line
   EPD_W21_WriteDATA(0x64);
   EPD_W21_WriteDATA(0x00);

   EPD_W21_WriteCMD(0x22); // Display Update Control 2
   EPD_W21_WriteDATA(0x91);

   EPD_W21_WriteCMD(0x20); // Master Activation
   EPD_WaitUntilIdle();

    EPD_W21_WriteCMD(0x11);
	EPD_W21_WriteDATA(0x05);

	EPD_W21_WriteCMD(0x44); //set Ram-X address start/end position
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x31);    //0x12-->(18+1)*8=152

	EPD_W21_WriteCMD(0x45); //set Ram-Y address start/end position
	EPD_W21_WriteDATA(0x0F);   //0x97-->(151+1)=152
	EPD_W21_WriteDATA(0x01);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);



	EPD_W21_WriteCMD(0x4E);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0x4F);
	EPD_W21_WriteDATA(0x0F);
	EPD_W21_WriteDATA(0x01);
	EPD_WaitUntilIdle();
	EPD_W21_WriteCMD(0x24);
	for(i=0;i<Source_BYTES*Gate_BITS;i++)
	   {
	     tempOriginal=*(datas+templine*Source_BYTES*2+tempcol);
	     templine++;
	     if(templine>=Gate_BITS)
	     {
	       tempcol++;
	       templine=0;
	     }
	    EPD_W21_WriteDATA(~tempOriginal);

}

	 EPD_W21_WriteCMD(0x26);   //write RAM for black(0)/white (1)
	  for(i=0;i<Source_BYTES*Gate_BITS;i++)
	   {
	     EPD_W21_WriteDATA(0X00);
	   }

	  EPD_W21_WriteCMD(0x91);
	  EPD_W21_WriteDATA(0x04);

	  EPD_W21_WriteCMD(0xC4); //set Ram-X address start/end position
	  EPD_W21_WriteDATA(0x31);
	  EPD_W21_WriteDATA(0x00);    //0x12-->(18+1)*8=152

	  EPD_W21_WriteCMD(0xC5); //set Ram-Y address start/end position
	  EPD_W21_WriteDATA(0x0F);   //0x97-->(151+1)=152  ÐÞ¸ÄµÄ
	  EPD_W21_WriteDATA(0x01);
	  EPD_W21_WriteDATA(0x00);
	  EPD_W21_WriteDATA(0x00);

	  EPD_W21_WriteCMD(0xCE);
	  EPD_W21_WriteDATA(0x31);
	  EPD_W21_WriteCMD(0xCF);
	  EPD_W21_WriteDATA(0x0F);
	  EPD_W21_WriteDATA(0x01);

	  EPD_WaitUntilIdle();

	  tempcol=tempcol-1; //Byte dislocation processing
	    templine=0;
	      EPD_W21_WriteCMD(0xa4);   //write RAM for black(0)/white (1)
	    for(i=0;i<Source_BYTES*Gate_BITS;i++)
	     {
	       tempOriginal=*(datas+templine*Source_BYTES*2+tempcol);
	       templine++;
	       if(templine>=Gate_BITS)
	       {
	         tempcol++;
	         templine=0;
	       }
	       EPD_W21_WriteDATA(~tempOriginal);
	   //EPD_W21_WriteDATA(tempOriginal);
	     }

	      EPD_W21_WriteCMD(0xa6);   //write RAM for black(0)/white (1)
	    for(i=0;i<Source_BYTES*Gate_BITS;i++)
	     {
	       EPD_W21_WriteDATA(0X00);
	     }

	 //    EPD_Update_Fast();
	    EPD_W21_WriteCMD(0x22);
		EPD_W21_WriteDATA(0xC7);
		EPD_W21_WriteCMD(0x20);
		EPD_WaitUntilIdle();
}

 static void EPD_Full_Update_Mode(const unsigned char *data)
{
  unsigned int i;

	EPD_RST_LOW();
	delay_xms(10);
	EPD_RST_HIGH();
	delay_xms(10);

	EPD_WaitUntilIdle();
    EPD_W21_WriteCMD(0x12);  //SWRESET
    EPD_WaitUntilIdle();

 // Set_ramMP();
    EPD_W21_WriteCMD(0x11);

    EPD_W21_WriteDATA(0x03);

	EPD_W21_WriteCMD(0x44);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x31);

	EPD_W21_WriteCMD(0x45);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x0F);
	EPD_W21_WriteDATA(0x01);

   // Set_ramMA();
	EPD_W21_WriteCMD(0x4e);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0x4f);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0x24);
    for(i=0;i<Source_BYTES*Gate_BITS;i++)
    {

    	 EPD_W21_WriteDATA(data[i]);
    }

    // Set_ramMA();
	EPD_W21_WriteCMD(0x4e);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0x4f);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0x26);
    for(i=0;i<Source_BYTES*Gate_BITS;i++)
    {
        EPD_W21_WriteDATA(0x00);
    }


  //Set_ramSP();
    EPD_W21_WriteCMD(0x91);
	EPD_W21_WriteDATA(0x04);

	EPD_W21_WriteCMD(0xc4);
	EPD_W21_WriteDATA(0x31);
	EPD_W21_WriteDATA(0x00);

	EPD_W21_WriteCMD(0xc5);
	EPD_W21_WriteDATA(0x0f);
	EPD_W21_WriteDATA(0x01);
	EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    //Set_ramSA();
    EPD_W21_WriteCMD(0xce);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0xcf);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);

    EPD_W21_WriteCMD(0xA4);
    for(i=0;i<Source_BYTES*Gate_BITS;i++)
    {

        EPD_W21_WriteDATA(data[i]);

    }

 // Set_ramSA();
    EPD_W21_WriteCMD(0xce);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteCMD(0xcf);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);

  EPD_W21_WriteCMD(0xA6);
  for(i=0;i<Source_BYTES*Gate_BITS;i++)
  {
      EPD_W21_WriteDATA(0x00);
  }

  //EPD_Update();
    EPD_W21_WriteCMD(0x22);
  	EPD_W21_WriteDATA(0xF7);
  	EPD_W21_WriteCMD(0x20);
  	EPD_WaitUntilIdle();
}


#define WIDTH  800
#define HEIGHT 272

static uint8_t canvas[HEIGHT][WIDTH];           // Canvas for rendering characters
static uint8_t epd_bitmap_array[HEIGHT*(WIDTH/8)];  // Packed 1-bit bitmap for display
// ---------- Safe GFXfont drawing helpers ----------

static inline uint32_t glyph_byte_length(const GFXglyph *g) {
    // Number of bits = width * height
    uint32_t bits = (uint32_t)g->width * (uint32_t)g->height;
    return (bits + 7) >> 3;
}

void drawGlyph(const GFXfont *f, int x, int y, unsigned char c)
{
    if (!f) return;
    if (c < f->first || c > f->last) return;

    const GFXglyph *glyph = &f->glyph[c - f->first];
    const uint8_t *bitmap = f->bitmap; // base of font bitmap

    int gw = glyph->width;
    int gh = glyph->height;
    int xo = glyph->xOffset;
    int yo = glyph->yOffset;
    int xadv = glyph->xAdvance;
    uint32_t byteOffset = glyph->bitmapOffset;

    if (gw == 0 || gh == 0) {
        // nothing to draw (e.g. space)
        return;
    }

    uint32_t glyphBytes = glyph_byte_length(glyph);

    // bitIndex_total = (byteOffset * 8) + row*gw + col
    uint32_t baseBit = byteOffset * 8u;

    // Loop rows, cols with 32-bit indices to avoid overflow for large fonts.
    for (uint32_t row = 0; row < (uint32_t)gh; ++row) {
        for (uint32_t col = 0; col < (uint32_t)gw; ++col) {
            uint32_t bitIndex = baseBit + row * (uint32_t)gw + col;
            uint32_t byteIndex = bitIndex >> 3;     // /8
            uint8_t  bitInByte = 7u - (bitIndex & 7u);

            // Safety: ensure we don't read beyond glyph bytes.
            if (byteIndex < byteOffset || byteIndex >= byteOffset + glyphBytes) {
                // outside glyph storage — treat as off
                continue;
            }

            uint8_t b = bitmap[byteIndex];
            bool pixelOn = ((b >> bitInByte) & 0x01) != 0;

            if (pixelOn) {
                int px = x + (int)col + xo;
                int py = y + (int)row + yo;

                // Clip to canvas boundaries
                if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                    canvas[py][px] = 0; // black pixel
                }
            }
        }
    }
}


void drawCharWithFont(const GFXfont *f, int x, int y, char c)
{
    drawGlyph(f, x, y, (unsigned char)c);
}

void drawGlyphScaled(const GFXfont *f, int x, int y, unsigned char c, float scale)
{
    if (!f) return;
    if (c < f->first || c > f->last) return;

    const GFXglyph *glyph = &f->glyph[c - f->first];
    const uint8_t *bitmap = f->bitmap;

    int gw = glyph->width;
    int gh = glyph->height;
    int xo = glyph->xOffset;
    int yo = glyph->yOffset;

    uint32_t baseBit = glyph->bitmapOffset * 8;

    for (int row = 0; row < gh; row++) {
        for (int col = 0; col < gw; col++) {

            uint32_t bitIndex  = baseBit + row * gw + col;
            uint32_t byteIndex = bitIndex >> 3;
            uint8_t bitInByte  = 7 - (bitIndex & 7);

            bool pixelOn = (bitmap[byteIndex] >> bitInByte) & 1;
            if (!pixelOn) continue;

            // scaled position
            int px = x + (int)((col + xo) * scale);
            int py = y + (int)((row + yo) * scale);

            int s = (int)(scale);
            if (s < 1) s = 1;  // ensure minimum 1 pixel

            for (int dy = 0; dy < s; dy++)
                for (int dx = 0; dx < s; dx++)
                    if (px+dx >= 0 && px+dx < WIDTH && py+dy >= 0 && py+dy < HEIGHT)
                        canvas[py+dy][px+dx] = 0;
        }
    }
}

int measureWrappedTextHeight(const char* text, const GFXfont* f, int maxWidth)
{
    int first = f->first;
    int last  = f->last;

    int lineHeight = f->yAdvance;

    // Compute font ascent/descent
    int maxAscent = 0, maxDescent = 0;
    for (int ch = first; ch <= last; ch++) {
        const GFXglyph *g = &f->glyph[ch - first];
        int ascent  = -g->yOffset;
        int descent = g->height + g->yOffset;
        if (ascent  > maxAscent)  maxAscent  = ascent;
        if (descent > maxDescent) maxDescent = descent;
    }

    // --- word wrap (same as your code) ---
    int lineCount = 0;
    int idx = 0;
    int curWidth = 0;
    char tempWord[128];
    int maxLines = 40;

    while (text[idx] != '\0') {
        int wi = 0;
        while (text[idx] != '\0' && text[idx] != ' ' && wi < 127)
            tempWord[wi++] = text[idx++];
        tempWord[wi] = '\0';

        int hasSpace = (text[idx] == ' ');
        if (hasSpace) idx++;

        int wordWidth = 0;
        for (int k = 0; k < wi; k++) {
            unsigned char ch = tempWord[k];
            if (ch >= first && ch <= last)
                wordWidth += f->glyph[ch-first].xAdvance;
        }
        if (hasSpace && ' ' >= first && ' ' <= last)
            wordWidth += f->glyph[' '-first].xAdvance;

        if (curWidth + wordWidth >= maxWidth) {
            lineCount++;
            curWidth = 0;
        }

        curWidth += wordWidth;
        if (lineCount >= maxLines) break;
    }

    if (curWidth > 0) lineCount++;

    return (maxAscent + maxDescent) + (lineHeight * (lineCount - 1));
}

const GFXfont* chooseBestFont(const char* text)
{
    int maxWidth = WIDTH - 10;
    const GFXfont* best = FONT_LIST[0];

    for (int i = 0; i < FONT_COUNT; i++)
    {
        int h = measureWrappedTextHeight(text, FONT_LIST[i], maxWidth);
        if (h <= HEIGHT)
            best = FONT_LIST[i];
        else
            break;     // larger fonts won't fit
    }
    return best;
}

//void renderTextToBitmapCentered(const char* text, const GFXfont *f)
//{
//    if (!text || !f) return;
//
//    // Clear canvas
//    for (int yy = 0; yy < HEIGHT; ++yy)
//        for (int xx = 0; xx < WIDTH; ++xx)
//            canvas[yy][xx] = 1;
//
//    int first = f->first;
//    int last  = f->last;
//    int lineHeight = f->yAdvance + LINE_SPACING;
//
//    int maxAscent = 0;
//    int maxDescent = 0;
//
//    // Measure max ascent & descent
//    for (int ch = first; ch <= last; ch++) {
//        const GFXglyph *g = &f->glyph[ch - first];
//        int ascent  = -g->yOffset;
//        int descent = g->height + g->yOffset;
//        if (ascent > maxAscent) maxAscent = ascent;
//        if (descent > maxDescent) maxDescent = descent;
//    }
//
//    // ------------------------------
//    // WORD WRAPPING with spacing
//    // ------------------------------
//    #define MAX_LINES 40
//    #define MAX_LINE_CHARS 512
//
//    static char lines[MAX_LINES][MAX_LINE_CHARS];
//    int lineCount = 0;
//
//    for (int i = 0; i < MAX_LINES; i++)
//        lines[i][0] = 0;
//
//    char currentLine[MAX_LINE_CHARS] = "";
//    int currentWidth = 0;
//    int idx = 0;
//    int maxWidth = WIDTH - 10;
//
//    while (text[idx] != '\0') {
//
//        char word[128];
//        int wi = 0;
//
//        while (text[idx] != '\0' && text[idx] != ' ' && wi < sizeof(word) - 1)
//            word[wi++] = text[idx++];
//        word[wi] = '\0';
//
//        int hasSpace = (text[idx] == ' ');
//        if (hasSpace) idx++;
//
//        // Compute width of the word including spacing
//        int wordWidth = 0;
//        for (int k = 0; k < wi; k++) {
//            unsigned char ch = (unsigned char)word[k];
//            if (ch < first || ch > last) continue;
//            wordWidth += f->glyph[ch - first].xAdvance + CHAR_SPACING;
//        }
//
//        if (hasSpace)
//            wordWidth += WORD_SPACING;
//
//        if (currentWidth + wordWidth >= maxWidth && currentLine[0] != '\0') {
//            strcpy(lines[lineCount++], currentLine);
//            currentLine[0] = 0;
//            currentWidth = 0;
//        }
//
//        strcat(currentLine, word);
//        if (hasSpace) strcat(currentLine, " ");
//
//        currentWidth += wordWidth;
//    }
//
//    if (currentLine[0] != '\0')
//        strcpy(lines[lineCount++], currentLine);
//
//    if (lineCount == 0) return;
//
//    // ------------------------------
//    // VERTICAL CENTERING
//    // ------------------------------
//    int totalHeight =
//        (maxAscent + maxDescent) +
//        (lineHeight * (lineCount - 1));
//
//    int cursorY = (HEIGHT - totalHeight) / 2 + maxAscent;
//
//    // ------------------------------
//    // DRAW LINES
//    // ------------------------------
//    for (int ln = 0; ln < lineCount; ln++) {
//        // Compute the line width including spacing
//        int lineW = 0;
//
//        for (int j = 0; lines[ln][j] != '\0'; j++) {
//            unsigned char ch = (unsigned char)lines[ln][j];
//            if (ch < first || ch > last) continue;
//
//            if (ch == ' ') lineW += WORD_SPACING;
//            else lineW += f->glyph[ch - first].xAdvance + CHAR_SPACING;
//        }
//
//        int cursorX = (WIDTH - lineW) / 2;
//
//        // Draw characters
//        for (int j = 0; lines[ln][j] != '\0'; j++) {
//
//            unsigned char ch = (unsigned char)lines[ln][j];
//            if (ch < first || ch > last) continue;
//
//            if (ch == ' ') {
//                cursorX += WORD_SPACING;
//                continue;
//            }
//
//            drawGlyph(f, cursorX, cursorY, ch);
//            cursorX += f->glyph[ch - first].xAdvance + CHAR_SPACING;
//        }
//
//        cursorY += lineHeight;
//    }
//
//    // ------------------------------
//    // ORIENTATION (Your exact code)
//    // ------------------------------
//    int idxOut = 0;
//
//    for (int yy = 0; yy < HEIGHT; yy++) {
//        for (int xb = 0; xb < WIDTH/8; xb++) {
//
//            uint8_t b = 0;
//
//            for (int bit = 0; bit < 8; bit++) {
//
//                int x = xb*8 + bit;
//                int sx = 0, sy = 0;
//
//                switch (epd_orientation) {
//
//                case ORIENT_0:
//                    sx = x; sy = yy;
//                    break;
//
//                case ORIENT_90:
//                    sx = yy;
//                    sy = (WIDTH - 1) - x;
//                    break;
//
//                case ORIENT_180:
//                    sx = (WIDTH - 1) - x;
//                    sy = (HEIGHT - 1) - yy;
//                    break;
//
//                case ORIENT_270:
//                    sx = (HEIGHT - 1) - yy;
//                    sy = x;
//                    break;
//                }
//
//                if (canvas[sy][sx] == 0)
//                    b |= 1 << (7 - bit);
//            }
//
//            epd_bitmap_array[idxOut++] = b;
//        }
//    }
//}



void renderTextToBitmapCentered(const char* text, const GFXfont *f)
{
    if (!text || !f) return;

    // Clear canvas to white
    for (int yy = 0; yy < HEIGHT; ++yy)
        for (int xx = 0; xx < WIDTH; ++xx)
            canvas[yy][xx] = 1;

    // Font metrics
    int first = f->first;
    int last  = f->last;
    int lineHeight = f->yAdvance;

    // Measure max ascent and descent from all glyphs
    int maxAscent = 0;
    int maxDescent = 0;
    for (int ch = first; ch <= last; ch++) {
        const GFXglyph *g = &f->glyph[ch - first];
        int ascent  = -g->yOffset;
        int descent = g->height + g->yOffset;
        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;
    }

    // Word wrap
    #define MAX_LINES 40
    #define MAX_LINE_CHARS 512
    static char lines[MAX_LINES][MAX_LINE_CHARS];
    int lineCount = 0;
    for (int i=0;i<MAX_LINES;i++) lines[i][0]=0;

    char currentLine[MAX_LINE_CHARS] = "";
    int currentWidth = 0;
    int idx = 0;
    int maxWidth = WIDTH - 10;

    while (text[idx] != '\0') {
        char word[128]; int wi=0;
        while (text[idx] != '\0' && text[idx] != ' ' && wi < (int)sizeof(word)-1)
            word[wi++] = text[idx++];
        word[wi]='\0';
        int hasSpace = (text[idx] == ' ');
        if (hasSpace) idx++;

        int wordWidth = 0;
        for (int k=0;k<wi;k++) {
            unsigned char ch = (unsigned char)word[k];
            if (ch < first || ch > last) continue;
            wordWidth += f->glyph[ch - first].xAdvance;
        }
        if (hasSpace && ' ' >= first && ' ' <= last)
            wordWidth += f->glyph[' ' - first].xAdvance;

        if (currentWidth + wordWidth >= maxWidth && currentLine[0] != '\0') {
            strncpy(lines[lineCount], currentLine, MAX_LINE_CHARS-1);
            lines[lineCount][MAX_LINE_CHARS-1]=0;
            lineCount++;
            currentLine[0]=0;
            currentWidth=0;
        }

        strncat(currentLine, word, MAX_LINE_CHARS - strlen(currentLine) - 1);
        if (hasSpace) strncat(currentLine, " ", MAX_LINE_CHARS - strlen(currentLine) - 1);
        currentWidth += wordWidth;
        if (lineCount >= MAX_LINES-1) break;
    }
    if (currentLine[0] != '\0' && lineCount < MAX_LINES)
        strncpy(lines[lineCount++], currentLine, MAX_LINE_CHARS-1);

    if (lineCount == 0) return;

    // Compute total text height
    int totalHeight = (maxAscent + maxDescent) + (lineHeight * (lineCount - 1));
    int cursorY = (HEIGHT - totalHeight) / 2 + maxAscent;

    // Draw each line centered horizontally
    for (int ln = 0; ln < lineCount; ln++) {
        int lineW = 0;
        for (int j=0; lines[ln][j]!='\0'; j++) {
            unsigned char ch = (unsigned char)lines[ln][j];
            if (ch < first || ch > last) continue;
            lineW += f->glyph[ch - first].xAdvance;
        }
        int cursorX = (WIDTH - lineW) / 2;

        for (int j=0; lines[ln][j]!='\0'; j++) {
            unsigned char ch = (unsigned char)lines[ln][j];
            if (ch < first || ch > last) continue;
            drawGlyph(f, cursorX, cursorY, ch);
            cursorX += f->glyph[ch - first].xAdvance;
        }
        cursorY += lineHeight;
    }

    int idxOut = 0;
    for (int yy = 0; yy < HEIGHT; yy++) {
        for (int xb = 0; xb < WIDTH/8; xb++) {

            uint8_t b = 0;

            for (int bit = 0; bit < 8; bit++) {

                int x = xb*8 + bit;
                int sx = 0, sy = 0;

                switch (epd_orientation) {

                case ORIENT_0:
                    sx = x;
                    sy = yy;
                    break;

                case ORIENT_90:
                    sx = yy;
                    sy = (WIDTH - 1) - x;
                    break;

                case ORIENT_180:
                    sx = (WIDTH - 1) - x;
                    sy = (HEIGHT - 1) - yy;
                    break;

                case ORIENT_270:
                    sx = (HEIGHT - 1) - yy;
                    sy = x;
                    break;
                }

                if (canvas[sy][sx] == 0)
                    b |= 1 << (7 - bit);
            }

            epd_bitmap_array[idxOut++] = b;
        }
    }

}


/* ------------------ Peripheral Initialization ------------------ */
static void MX_SPI1_Init(void){
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // safe speed
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if(HAL_SPI_Init(&hspi1)!=HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    // Output pins: RESET, DC, CS
    GPIO_InitStruct.Pin = EPD_RESET_PIN | EPD_DC_PIN | EPD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EPD_PORT, &GPIO_InitStruct);

    // Input pin: BUSY
    GPIO_InitStruct.Pin = EPD_BUSY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(EPD_PORT, &GPIO_InitStruct);

    // SPI pins: PA5=SCK, PA7=MOSI
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Optional: PA6=MISO
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Initial pin states
    EPD_CS_HIGH();
    EPD_RST_HIGH();
}
// Add at the top or before main() if not already present
void SystemClock_Config(void) {
    // Minimal clock config using HSI
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) while(1);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) while(1);
}

// Minimal Error_Handler
void Error_Handler(void) {
    __disable_irq();
    while(1) {
        // You could toggle an LED here for debugging
    }
}
/* ------------------ Main ------------------ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
	MX_GPIO_Init();
	MX_SPI1_Init();

	 // epd_orientation = ORIENT_0;   // Normal orientation
	 // epd_orientation = ORIENT_90;  // 90° clockwise
	    epd_orientation = ORIENT_180; // Upside down
	 // epd_orientation = ORIENT_270; // 270° counterclockwise



	   // const char* text = "SOLVE SOLUTION SIMPLIFIED solve solution simplifies CORPORATE CONNECTION corporate connection SOLVE SOLUTION SIMPLIFIED solve solution simplifies CORPORATE CONNECTION corporate connection";
//	    const char* text="A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.";

        text="S";
	    autoFont = chooseBestFont(text);
		renderTextToBitmapCentered(text, autoFont);
	    EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

	     text="SOLVE";
		 autoFont = chooseBestFont(text);
		renderTextToBitmapCentered(text, autoFont);
		EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

	     text="SOLVE SOLUTION";
		 autoFont = chooseBestFont(text);
		renderTextToBitmapCentered(text, autoFont);
		EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES";
		  autoFont = chooseBestFont(text);
		 renderTextToBitmapCentered(text, autoFont);
		 EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES CORPORATE CONNECTION";
		  autoFont = chooseBestFont(text);
		 renderTextToBitmapCentered(text, autoFont);
		 EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES CORPORATE CONNECTION solve solution simplifies corporate connection";
		  autoFont = chooseBestFont(text);
		 renderTextToBitmapCentered(text, autoFont);
		 EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.";
		  autoFont = chooseBestFont(text);
		 renderTextToBitmapCentered(text, autoFont);
		 EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);





     while(1)
     {

    	  text="S";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES CORPORATE CONNECTION";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

		  text="SOLVE SOLUTION SIMPLIFIES CORPORATE CONNECTION solve solution simplifies corporate connection";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);

//		  text="~!@#$% ^&*()_ +-={}| \][;'' :,.<>/?";
//		  autoFont = chooseBestFont(text);
//		  renderTextToBitmapCentered(text, autoFont);
//		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);


		  text="A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.A gentle breeze drifted across the quiet park, carrying soft birdsong over dew-touched grass. Early sunlight painted warm colors on benches and winding paths. People began arriving slowly, greeting one another with calm smiles. The peaceful scene invited everyone to pause, breathe, and appreciate a moment of stillness today here.";
		  autoFont = chooseBestFont(text);
		  renderTextToBitmapCentered(text, autoFont);
		  EPD_WhiteScreen_ALL_Fast1(epd_bitmap_array);


    }
}

