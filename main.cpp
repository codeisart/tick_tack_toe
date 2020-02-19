#include <stdio.h>
#include <vector>
#include "SDL2/SDL.h"
#include <SDL2/SDL_ttf.h>

#define WIDTH 600
#define HEIGHT 600
#define ABS(X) (x<0 ? -x : x)
#define MAX(A,B) (A>B ? A : B)
#define RGBA(R,G,B,A) (uint32_t)((A&0xff)<<24)|((R&0xff)<<16)|((G&0xff)<<8)|((B&0xff))
#define NORM(RS,RE,VAL) ((float)VAL-RS)/((float)RE-RS)
#define LERP(RS,RE,NVAL) ((float)RS+((float)(RE-RS)*NVAL))
#define CLAMP(X,Y,N) ((N)<(X) ? (X) : (N) > (Y) ? (Y) : (N))

static const uint32_t kFontSize = 16;
TTF_Font* gDebugFont = nullptr;
SDL_Renderer *gRenderer = nullptr;
int gCurrentTune = 0;

uint32_t ddraw(int x,int y, SDL_Color c, const char* fmt,  ... )
{
    char buff[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);

    SDL_Surface *text = TTF_RenderUTF8_Blended(gDebugFont, buff, c);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, text);
    SDL_FreeSurface(text);
    SDL_Rect dest = { x,y };
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
    SDL_RenderCopy(gRenderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    return kFontSize;
}
void rect(uint32_t* buf, int buff_width, int buff_height, int xpos, int ypos, int width, int height, uint32_t col)
{
    uint32_t* pos = buf+(ypos*buff_width);
    for( int y=ypos; y < ypos+height; ++y, pos+=buff_width)
	for( int x=xpos; x < xpos+width; ++x )
	    pos[x] = col;
}
void elipse(uint32_t* buf, int buff_width, int buff_height, int xpos, int ypos, int width, int height, uint32_t col)
{
    for( int i = 0; i < 360; ++i)
    {
	float thetaRads = ((float)i/360.f) * (2.f*M_PI);
	int x = (float)xpos+cosf(thetaRads) * (float)width/2.f;
	int y = (float)ypos+sinf(thetaRads) * (float)height/2.f;
	buf[(int)x+(y*buff_width)] = col;
    }
}

void line(uint32_t* buf, int buff_width, int buff_height, int x1, int y1, int x2, int y2, uint32_t col)
{
    if( y1 > y2 )
    {
	int tmp = y1;
	y1 = y2;
	y2 = tmp;
	tmp = x1;
	x1 = x2;
	x2 = tmp;
    }
    if( y2-y1 == 0)
    {
	for(int x=x1; x<x2; x++)
	    buf[x+(y1*buff_width)]= col;
    }
    else
    {
	float xstep = (float)(x2-x1) / (y2-y1);
	float x = x1;
	for(int y = y1; y < y2; ++y, x+=xstep) {
	    buf[static_cast<int>(x)+(y*buff_width)] = col;
	}
    }
}

bool equals3(char a, char b, char c) { return a == b && b == c; }

struct Pos2d { int x=0; int y=0; };
struct Game
{
    int turn = 0;	    // either 0, or 1 (for player 1 or 2).
    char board[3][3] = {
	{1,2,3},
	{4,5,6},
	{7,8,9}
    };

    char checkWon() const
    {
	for(int y=0; y<3; ++y) if( equals3( board[y][0],board[y][1],board[y][2] ) ) return board[y][0];
	for(int x=0; x<3; ++x) if( equals3( board[0][x],board[1][x],board[2][x] ) ) return board[0][x];
	if( equals3(board[0][0],board[1][1],board[2][2]) ) return board[0][0];
	if( equals3(board[0][2],board[1][1],board[2][0]) ) return board[0][2];
	return 0;
    }

    void draw(uint32_t* p, int w, int h)
    {
	int bx = w / 3;
	int by = h / 3;

	// draw board.
	line(p,w,h, bx,    0,    bx, HEIGHT, RGBA(255,255,255,255));
	line(p,w,h, bx*2 , 0,  bx*2, HEIGHT, RGBA(255,255,255,255));
	line(p,w,h, 0,    by,  WIDTH,    by, RGBA(255,255,255,255));
	line(p,w,h, 0,   by*2,  WIDTH,  by*2, RGBA(255,255,255,255));

	for(int y = 0; y < 3; ++y) {
	    for(int x=0; x < 3; ++x) {
		if(board[y][x]=='o')
		    elipse(p,w,h,(bx*x)+(bx/2),(by*y)+(by/2), bx,by,RGBA(255,255,255,255));
		else if(board[y][x]=='x')
		{
		    line(p,w,h,	bx*x,	     by*y, bx*(x+1),	by*(y+1),     RGBA(255,255,255,255));
		    line(p,w,h,	bx*(x+1),    by*y, bx*x,	by*(y+1),     RGBA(255,255,255,255));
		}
	    }
	}
    }
}gGame;

int main()
{
    srand (time(NULL));
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
	fprintf(stderr, "Could not init SDL: %s\n", SDL_GetError());
	return 1;
    }
    SDL_Window *screen = SDL_CreateWindow("My application",
	    SDL_WINDOWPOS_CENTERED,
	    SDL_WINDOWPOS_CENTERED,
	    WIDTH, HEIGHT,
	    0//SDL_WINDOW_FULLSCREEN
	    );
    if(!screen) {
	fprintf(stderr, "Could not create window\n");
	return 1;
    }
    gRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
    if(!gRenderer) {
	fprintf(stderr, "Could not create renderer\n");
	return 1;
    }

    if(TTF_Init()==-1) {
	printf("TTF_Init: %s\n", TTF_GetError());
	exit(2);
    }
    // load font.ttf at size 16 into font
    gDebugFont=TTF_OpenFont("Hack-Regular.ttf", kFontSize);
    if(!gDebugFont) {
	printf("TTF_OpenFont: %s\n", TTF_GetError());
	exit(3);
    }

    uint32_t* pixels = new uint32_t[WIDTH*HEIGHT];
    SDL_Texture * texture = SDL_CreateTexture(gRenderer,
	    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);

    static const SDL_Color white = {255, 255, 255, 255};
    bool quit = false;

    int32_t prevTime = SDL_GetTicks();;
    while (!quit)
    {
	int32_t currentTime = SDL_GetTicks();
	int32_t delta = currentTime-prevTime;

	uint32_t *p = pixels;
	memset(p,0,WIDTH*HEIGHT*4);
	gGame.draw(p,WIDTH,HEIGHT);

	SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, texture, NULL, NULL);

	if( gGame.checkWon() )
	{
	    ddraw(WIDTH/2-(5/2),HEIGHT/2,white,"%c WON!", gGame.checkWon());
	}

	// flip.
	SDL_RenderPresent(gRenderer);

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
	    if (e.type == SDL_QUIT)
	    {
		quit = true;
	    }
	    if (e.type == SDL_KEYDOWN)
	    {
		SDL_Keymod m = SDL_GetModState();
	    }
	    if (e.type == SDL_MOUSEBUTTONDOWN)
	    {
		// we want to map click into board location
		int x,y;
		SDL_GetMouseState(&x,&y);
		int blkx = WIDTH / 3;
		int blky = HEIGHT / 3;
		int gridx = x / blkx;
		int gridy = y / blky;
		char c = gGame.turn ? 'x' : 'o';
		gGame.board[gridy][gridx] = c;
		gGame.turn = !gGame.turn;
	    }
	    prevTime = currentTime;
	}
	SDL_Delay( 60 );
    }

    delete[] pixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(screen);
    TTF_CloseFont(gDebugFont);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
