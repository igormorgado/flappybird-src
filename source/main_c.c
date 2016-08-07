
/* This program is a derivative of a test program also shipped within SDL.
   It uses a happy face sprite and draws multiple instances of them, bouncing them around the screen.
   I (Eric Wing) have modified it to help test bringing up SDL on other platforms and to do both performance benchmarking.
   
   Please see the SDL documentation for details on using the SDL API:
   /Developer/Documentation/SDL/docs.html
*/
   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "ALmixer.h"
#include "chipmunk.h"

#include "BlurrrCore.h"

#include "CircularQueue.h"
#include "TimeTicker.h"


#ifdef __ANDROID__
#include <jni.h>
#include "SDL_system.h"
#include "ALmixer_PlatformExtensions.h"
#endif

#include <stdlib.h>

#define NUMBER_OF_BIRD_FRAMES 3

struct GameTextures
{
	SDL_Texture* background;
	SDL_Texture* bush;
	SDL_Texture* clouds;
//	SDL_Texture* fly1;
//	SDL_Texture* fly2;
	SDL_Texture* ground;
	SDL_Texture* pipe_bottom;
	SDL_Texture* pipe_top;
	SDL_Texture* bird[NUMBER_OF_BIRD_FRAMES];
	SDL_Texture* medalBackground;
	SDL_Texture* placeholderMedal;
	SDL_Texture* bronzeMedal;
	SDL_Texture* silverMedal;
	SDL_Texture* goldMedal;
	SDL_Texture* platinumMedal;

	SDL_Texture* medalSceneRenderToTexture;

	SDL_Texture* playButton;
	SDL_Texture* quitButton;

};
struct GameTextures g_gameTextures;

struct GameSounds
{
	ALmixer_Data* coin;
	ALmixer_Data* flap;
	ALmixer_Data* crash;
	ALmixer_Data* fall;
	ALmixer_Data* swoosh;

};
struct GameSounds g_gameSounds;

typedef double MyFloat;
typedef struct Flappy_FloatPoint
{
	MyFloat x;
	MyFloat y;
} Flappy_FloatPoint;

struct SceneryModelData
{
	SDL_Point position;
	SDL_Point size;
	Flappy_FloatPoint velocity;
};
struct SceneryModelData g_cloudModelData;
struct SceneryModelData g_groundModelData;
struct SceneryModelData g_bushModelData;

struct SceneryModelData g_pipeTopModelData;
struct SceneryModelData g_pipeBottomModelData;

struct SceneryModelData g_playerModelData;


enum GameState
{
	GAMESTATE_TITLE_SCREEN = 0,
	GAMESTATE_FADE_OUT_TITLE_SCREEN,
	GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN,
	GAMESTATE_BIRD_PRELAUNCH,
	GAMESTATE_BIRD_LAUNCHED_BUT_BEFORE_PIPES,
	GAMESTATE_MAIN_GAME_ACTIVE,
//	GAMESTATE_DIED_FROM_HITTING_GROUND,
	GAMESTATE_DIED_FROM_HITTING_PIPE_AND_FALLING,
//	GAMESTATE_DIED_FROM_HITTING_PIPE_AND_ON_GROUND,
	GAMESTATE_BIRD_DEAD_ON_GROUND,
	GAMESTATE_DEAD_TO_GAME_OVER_TRANSITION,
	GAMESTATE_SWOOPING_IN_GAME_OVER,
	GAMESTATE_SWOOPED_IN_GAME_OVER,
	GAMESTATE_SWOOPING_IN_MEDAL_DISPLAY,
//	GAMESTATE_SWOOPED_IN_MEDAL_DISPLAY,
	GAMESTATE_TALLYING_SCORE,
//	GAMESTATE_SWOOPING_IN_BUTTONS,
	GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION,
	GAMESTATE_FADE_OUT_GAME_OVER
};

int g_gameState = GAMESTATE_TITLE_SCREEN;
Uint32 g_gameStateCurrentPhaseStartTime = 0;


struct HighScoreData
{
	/* Flappy Bird doesn't immediately write over the high score.
		Instead, it saves the previous high score until the tally at the end so you can see the record change.
	*/
	Uint32 savedHighScore;
	Uint32 previousHighScore;
	Uint32 currentHighScore;
};
struct HighScoreData g_highScoreData;



struct PipeModelData
{
	Sint32 intervalScalar;
	Sint32 viewPositionX;
	Sint32 realPositionX;

	Sint32 upperViewPositionY;
	Sint32 upperRealPositionY;
	Sint32 lowerViewPositionY;
	Sint32 lowerRealPositionY;
	
	Sint32 pipeHeight;

	SDL_Point size;
	cpBody* upperPhysicsBody;
	cpShape* upperPhysicsShape;

	cpBody* lowerPhysicsBody;
	cpShape* lowerPhysicsShape;
	
	cpBody* scoreSensorPhysicsBody;
	cpShape* scoreSensorPhysicsShape;
	
};

struct PipeModelData* g_arrayOfPipes = NULL;
CircularQueueVoid* g_circularQueueOfPipes = NULL;
CircularQueueVoid* g_queueOfAvailablePipes = NULL;

struct GameInstanceData
{
	Uint32 currentScore;
	Uint32 distanceTraveled;
	Uint32 diedAtTime;
	Uint32 needsWhiteOut;
	SDL_bool isPaused;
	
//	Uint32 tallyStartTime;
	
};

struct GameInstanceData g_gameInstanceData;


cpSpace* g_mainSpace = NULL;

struct BirdModelData
{
	cpBody* birdBody;
	cpShape* birdShape;
//	SDL_Point viewPosition;
//	SDL_Point realPosition;
	SDL_Point size;
	
	SDL_Point position;
	Flappy_FloatPoint velocity;
	/* For when the bird smacks the pipe and needs to fall */
	SDL_bool isFalling;
	SDL_bool isReadyToPlayFallingSound;
	SDL_bool isDead; /* smacks the pipe or ground */
	SDL_bool isGameOver; /* bird is completely dead on ground after falling from pipe or smacks ground */

	// TODO: Move to game state
	SDL_bool isPrelaunch;
	Uint32 birdLaunchStartTime;
	SDL_bool isReadyForPipe;

	
	SDL_Texture* currentTexture;
	Uint32 animationStartTime;
	Uint32 animationDuration;
	SDL_bool isAnimatingSprite;
	
};

/*
struct SpriteAnimationData
{

};
*/
struct BirdModelData g_birdModelData;

struct GroundPhysicsData
{
	cpBody* body;
	cpShape* shape;
	SDL_Point position;

};
struct GroundPhysicsData g_groundPhysicsData;


struct MedalBackgroundData
{
	SDL_Rect dstRect;
	Uint32 sweepInStartTime;
	Uint32 tallyStartTime;
	
};
struct MedalBackgroundData g_medalBackgroundData;


struct GameOverDisplayData
{
	SDL_Rect dstRect;
	int textureWidth;
	int textureHeight;
	Uint32 sweepInStartTime;
};
struct GameOverDisplayData g_gameOverDisplayData;

/* On consoles like SteamOS, we must have an explicit Quit option or the user will not be able
 to quit the game. (Tricks like Alt-F4 don't work unlessed explictly handled by the game.)
 But on iOS, we can't have a Quit button or we'll be rejeted.
 Android by convention doesn't display a Quit button either since the Exit button is available.
 Console-ized Androids have the Exit/Back button built into the controllers.
 So we will conditionalize the code depending on the platform.
 Desktops show both buttons.
 Mobiles will not.
 */
#if defined(__IPHONEOS__) || defined(__ANDROID__)
	#define FLAPPY_PROVIDE_QUIT_BUTTON 0
#else
	#define FLAPPY_PROVIDE_QUIT_BUTTON 1
#endif

struct GuiButton
{
    SDL_Rect dstRect;
    SDL_Texture* currentTexture;
    SDL_bool isPressed;
    SDL_bool isSelected;
};
struct GuiButton g_guiPlayButton;

#if FLAPPY_PROVIDE_QUIT_BUTTON
struct GuiButton g_guiQuitButton;
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */


int Flappy_lroundf(float x)
{
	return (int)(x+0.5f);
}

/*
 Produces a random int x, min <= x <= max
 following a uniform distribution
 */
int
randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

/*
 Produces a random float x, min <= x <= max
 following a uniform distribution
 */
float
randomFloat(float min, float max)
{
    return rand() / (float) RAND_MAX *(max - min) + min;
}

void
fatalError(const char *string)
{
    SDL_Log("%s: %s\n", string, SDL_GetError());
  //  exit(1);
}



static int NUM_HAPPY_FACES = 200;    /* number of faces to draw */
#define MILLESECONDS_PER_FRAME 16 * 1       /* about 60 frames per second */
//#define MILLESECONDS_PER_FRAME 16 * 2       /* about 30 frames per second */
#define HAPPY_FACE_SIZE 32      /* width and height of happyface (pixels) */

//#define SCREEN_WIDTH 480*2
//#define SCREEN_HEIGHT 320*2
//#define SCREEN_WIDTH 768
//#define SCREEN_HEIGHT 1024
//#define SCREEN_WIDTH 1366
//#define SCREEN_HEIGHT 768
//#define SCREEN_WIDTH 1136
//#define SCREEN_HEIGHT 640
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
//#define SCREEN_WIDTH 1024
//#define SCREEN_HEIGHT 768




/* Measured from iPad mini with ruler */
//#define GROUND_HEIGHT ((1024/16 * 1.9) * 768 / 1024) = 91.2
#define GROUND_HEIGHT 30


#define BIRD_WIDTH 62
#define BIRD_HEIGHT 50

const int PIPE_WIDTH = (int)(1.9*(double)BIRD_WIDTH);

/* Distance between pipes:
	iPad mini: 768 pixels (12cm) across screen,
	distance between leading pipe edge to next pipe edge: 6.5cm => 416 pixels
 */
//#define PIPE_DISTANCE 310
//#define PIPE_DISTANCE 380
//const int PIPE_DISTANCE = (int)(3.2*(double)BIRD_WIDTH) + PIPE_WIDTH;
//#define PIPE_DISTANCE (3*BIRD_WIDTH + PIPE_WIDTH + 12);
//#define PIPE_DISTANCE 400
#define PIPE_DISTANCE 360
//#define PIPE_DISTANCE 418
//#define PIPE_DISTANCE 743

/* About 3.5 Flappy Birds */
//#define PIPE_HEIGHT_SEPARATION 256
//#define PIPE_HEIGHT_SEPARATION 192
const int PIPE_HEIGHT_SEPARATION = (int)(3.5*(double)BIRD_HEIGHT);

/* y from bottom increasing upwards
   roughly Flappy height?
 */
//#define MIN_PIPE_HEIGHT 46
#define MIN_PIPE_HEIGHT BIRD_HEIGHT
#define NUM_PIPE_HEIGHT_QUANTA 20

#define BIRD_COLLISION_TYPE 1
#define GROUND_COLLISION_TYPE 2
#define PIPE_COLLISION_TYPE 3
#define SCORE_SENSOR_TYPE 4

#define BIRD_FLAP_ANIMATION_DURATION 150
#define BIRD_FLAP_ANIMATION_DURATION_FOR_PRELAUNCH 1000
#define PIPE_START_TIME_DELAY 1200

#define TALLY_DURATION 750

#define FADE_OUT_TIME 500
#define FADE_IN_TIME 500

struct TimeTicker* g_gameClock = NULL;

// See
// http://sdl.beuc.net/sdl.wiki/SDL_Average_FPS_Measurement

// How many frames time values to keep
// The higher the value the smoother the result is...
// Don't make it 0 or less :)
#define FRAME_VALUES 10

// An array to store frame times:
Uint32 g_arrayOfFrameTimes[FRAME_VALUES];

// Last calculated SDL_GetTicks
Uint32 g_frameTimeLast;

// total frames rendered
Uint32 g_totalFramesRenderedCounter;

// the value you want
float g_framesPerSecond;

// This function gets called once on startup.
void TemplateHelper_InitFps()
{
	
	// Set all frame times to 0ms.
	memset(g_arrayOfFrameTimes, 0, sizeof(g_arrayOfFrameTimes));
	g_totalFramesRenderedCounter = 0;
	g_framesPerSecond = 0;
	g_frameTimeLast = SDL_GetTicks();
	
}

void TemplateHelper_UpdateFps()
{
	
	Uint32 frametimesindex;
	Uint32 getticks;
	Uint32 count;
	Uint32 i;
	
	// frametimesindex is the position in the array. It ranges from 0 to FRAME_VALUES.
	// This value rotates back to 0 after it hits FRAME_VALUES.
	frametimesindex = g_totalFramesRenderedCounter % FRAME_VALUES;
	
	// store the current time
	getticks = SDL_GetTicks();
	
	// save the frame time value
	g_arrayOfFrameTimes[frametimesindex] = getticks - g_frameTimeLast;
	
	// save the last frame time for the next fpsthink
	g_frameTimeLast = getticks;
	
	// increment the frame count
	g_totalFramesRenderedCounter++;
	
	// Work out the current framerate
	
	// The code below could be moved into another function if you don't need the value every frame.
	
	// I've included a test to see if the whole array has been written to or not. This will stop
	// strange values on the first few (FRAME_VALUES) frames.
	if(g_totalFramesRenderedCounter < FRAME_VALUES)
	{
		
		count = g_totalFramesRenderedCounter;
		
	}
	else
	{
		
		count = FRAME_VALUES;
		
	}
	
	// add up all the values and divide to get the average frame time.
	g_framesPerSecond = 0;
	for(i = 0; i < count; i++)
	{
		
		g_framesPerSecond += g_arrayOfFrameTimes[i];
		
	}
	
	g_framesPerSecond /= count;
	
	// now to make it an actual frames per second value...
	g_framesPerSecond = 1000.f / g_framesPerSecond;
	
}



static SDL_Texture *texture = 0;    /* reference to texture holding happyface */

struct HappyFaceData
{
    float x, y;                 /* position of happyface */
    float xvel, yvel;           /* velocity of happyface */
};

static struct HappyFaceData* faces = NULL;

SDL_Window* g_mainWindow = NULL;
SDL_Renderer* g_mainRenderer = NULL;

Uint32 g_baseTime;
Uint32 g_myFPSPrintTimer;
SDL_bool g_appDone;

static TTF_Font* s_veraMonoFont = NULL;
static TTF_Font* s_retroFontForMainScore = NULL;
static float s_lastRecordedFPS = 0.0f;
static float s_lastDrawnFPS = 0.0f;
static SDL_Surface* s_surfaceFPS = NULL;
static SDL_Texture* s_textureFPS = NULL;


SDL_Texture* g_currentScoreTextTexture;

struct TextTextureData
{
	SDL_Texture* scoreLabelTextTexture;
	SDL_Texture* bestLabelTextTexture;
	SDL_Texture* newHighScoreLabelTextTexture;

	SDL_Texture* scoreNumberTextTexture;
//	SDL_Texture* scoreNumberTextShadowTexture;
	
	SDL_Texture* bestNumberTextTexture;
//	SDL_Texture* bestNumberTextShadowTexture;

	TTF_Font* fontForMedalLabels;
	TTF_Font* fontForScoreNumbers;
	
	SDL_Texture* getReadyTextTexture;
	SDL_Texture* gameOverTextTexture;
	
	SDL_Texture* flappyBlurrrTexture;
	SDL_Texture* taglineTexture;


};
struct TextTextureData g_textTextureData;


// Used to get the height of the game screen, minus the bird's Y
Sint32 Flappy_InvertY(Sint32 y)
{
	return (SCREEN_HEIGHT-y);
};


void Flappy_PauseGame()
{
	if(g_gameInstanceData.isPaused)
	{
	}
	else
	{
		TimeTicker_Stop(g_gameClock);
		g_gameInstanceData.isPaused = SDL_TRUE;
	}
	
}
void Flappy_UnpauseGame()
{
	if(g_gameInstanceData.isPaused)
	{
		TimeTicker_Start(g_gameClock);
		g_gameInstanceData.isPaused = SDL_FALSE;
	}
	else
	{
	}
	
}

void Flappy_TogglePause()
{
	if(g_gameInstanceData.isPaused)
	{
		TimeTicker_Start(g_gameClock);
		g_gameInstanceData.isPaused = SDL_FALSE;
	}
	else
	{
		TimeTicker_Stop(g_gameClock);
		g_gameInstanceData.isPaused = SDL_TRUE;
	}
	
}

SDL_bool TemplateHelper_ToggleFullScreen(SDL_Window* the_window, SDL_Renderer* the_renderer)
{
	int width = SCREEN_WIDTH;
	int height = SCREEN_HEIGHT;
	
    Uint32 flags = (SDL_GetWindowFlags(the_window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
    if(SDL_SetWindowFullscreen(the_window, flags) < 0) // NOTE: this takes FLAGS as the second param, NOT true/false!
    {
		SDL_Log("SDL_SetWindowFullscreen() failed: %s", SDL_GetError());
        return SDL_FALSE;
    }
    if((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
    {
        SDL_RenderSetLogicalSize(the_renderer, SCREEN_WIDTH, SCREEN_HEIGHT); // TODO: pass renderer as param maybe?
        return SDL_TRUE;
    }
    SDL_SetWindowSize(the_window, width, height);
    return SDL_TRUE;
}

struct FlappyGameControllerIdData
{
	SDL_GameController* gameController;
	int instanceID;
//	int deviceID;
};

#define MAX_NUM_CONTROLLERS 16
static struct FlappyGameControllerIdData s_gameControllerIdToControllerMapArray[MAX_NUM_CONTROLLERS];

void Flappy_InitGameControllerDataMap()
{
	int i;
	for(i=0; i<MAX_NUM_CONTROLLERS; i++)
	{
		s_gameControllerIdToControllerMapArray[i].gameController = NULL;
		s_gameControllerIdToControllerMapArray[i].instanceID = -1;
	}
}

void Flappy_AddGameControllerDataToMap(int instance_id, SDL_GameController* game_controller)
{
	int i;
	SDL_bool already_exists = SDL_FALSE;
	SDL_bool insertion_succeeded = SDL_FALSE;
	for(i=0; i<MAX_NUM_CONTROLLERS; i++)
	{
		if(s_gameControllerIdToControllerMapArray[i].gameController == game_controller)
		{
			already_exists = SDL_TRUE;
			SDL_assert(s_gameControllerIdToControllerMapArray[i].instanceID == instance_id);
			return;
			
		}
	}
	
	if(SDL_FALSE == already_exists)
	{
		for(i=0; i<MAX_NUM_CONTROLLERS; i++)
		{
			if(NULL == s_gameControllerIdToControllerMapArray[i].gameController)
			{
				s_gameControllerIdToControllerMapArray[i].gameController = game_controller;
				s_gameControllerIdToControllerMapArray[i].instanceID = instance_id;
				insertion_succeeded = SDL_TRUE;
				return;
			}
		}
	}

	/* Should have enough space to have added in array. */
	
	SDL_assert(insertion_succeeded != SDL_FALSE);
}

SDL_GameController* Flappy_GetGameControllerForInstanceId(int instance_id)
{
	int i;
	for(i=0; i<MAX_NUM_CONTROLLERS; i++)
	{
		if(s_gameControllerIdToControllerMapArray[i].instanceID == instance_id)
		{
			return s_gameControllerIdToControllerMapArray[i].gameController;
		}
	}
	return NULL;
}


void Flappy_RemoveGameControllerDataFromMap(SDL_GameController* game_controller)
{
	int i;
	for(i=0; i<MAX_NUM_CONTROLLERS; i++)
	{
		if(s_gameControllerIdToControllerMapArray[i].gameController == game_controller)
		{
			s_gameControllerIdToControllerMapArray[i].gameController = NULL;
			s_gameControllerIdToControllerMapArray[i].instanceID = -1;
			return;
		}
	}
}



void Flappy_AddController(int device_id)
{
    if(SDL_IsGameController(device_id))
	{
        SDL_GameController* game_controller = SDL_GameControllerOpen(device_id);
		
        if(game_controller)
		{
			SDL_Joystick* joy_stick = SDL_GameControllerGetJoystick(game_controller);
			int instance_id = SDL_JoystickInstanceID(joy_stick);
			Flappy_AddGameControllerDataToMap(instance_id, game_controller);
        }
    }
}

void Flappy_RemoveController(int instance_id)
{
    SDL_GameController* game_controller = Flappy_GetGameControllerForInstanceId(instance_id);
    SDL_GameControllerClose(game_controller);
	Flappy_RemoveGameControllerDataFromMap(game_controller);
}



Uint32 Flappy_LoadSavedHighScoreFromStorage()
{
	Uint32 high_score = 0;
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
#if 0
	int i = 0;
	FILE* resource_handle;
	
#ifdef __ANDROID__
	const char* pref_path = SDL_AndroidGetInternalStoragePath();
#else
	char* pref_path = SDL_GetPrefPath("net.playcontrol", "FlappyBlurrr");
#endif
	if(NULL == pref_path)
	{
		/* This might be okay because it is a first time run and the file doesn't exist. */
		return 0;
	}
	
	SDL_strlcpy(resource_file_path, pref_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "highscore.txt", MAX_FILE_STRING_LENGTH);
	
#ifdef __ANDROID__
#else
	SDL_free(pref_path);
#endif
	pref_path = NULL;
	
	resource_handle = fopen(resource_file_path, "r");
	if(NULL != resource_handle)
	{
		/* Please be nice. */
		fscanf(resource_handle, "%d", &i);
		high_score = i;
	}
	fclose(resource_handle);
#else
	SDL_RWops* rw_ops;

	
#ifdef __ANDROID__
	const char* pref_path = SDL_AndroidGetInternalStoragePath();
#else
	char* pref_path = SDL_GetPrefPath("net.playcontrol", "FlappyBlurrr");
#endif
	if(NULL == pref_path)
	{
		/* This might be okay because it is a first time run and the file doesn't exist. */
		return 0;
	}
	
	SDL_strlcpy(resource_file_path, pref_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "highscore.txt", MAX_FILE_STRING_LENGTH);
	
#ifdef __ANDROID__
#else
	SDL_free(pref_path);
#endif
	pref_path = NULL;
	
	/* Please be nice. Assumes the file contains a single ASCII number representing the number of sprites. */
	/* RWops is more resiliant because platforms don't always give you access to fopen/fread, like Android's .apk system.  */
	/* However in this case, all our target platforms allow fopen for preference files. */
	/* This is mainly done for demonstration. */
	/* No error checking is done for malicious input. */
	rw_ops = SDL_RWFromFile(resource_file_path, "r");
	if(NULL != rw_ops)
	{
		Uint32 result = 0;
		Uint8 character;
	   	do
		{
			character = SDL_ReadU8(rw_ops);
			/* Note: This won't handle negative numbers or exponential */
			if(character >=48 && character <= 57)
			{
				/* convert ascii value to real number */
				int value = character - 48;
//				SDL_Log("value: %d", value);
				/* update value */
				result = 10 * result + value;
			}
		}
		while(character >=48 && character <= 57);

//		SDL_Log("highscore.txt specified %d", result);
		SDL_RWclose(rw_ops);
		high_score = result;
	}
#endif
	
	return high_score;
}



SDL_bool Flappy_SaveHighScoreToStorage(Uint32 high_score)
{
#define MAX_NUMBER_OF_DIGITS 32
	char number_string[MAX_NUMBER_OF_DIGITS];
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	
#if 0
	FILE* resource_handle;
	
#ifdef __ANDROID__
	const char* pref_path = SDL_AndroidGetInternalStoragePath();
#else
	char* pref_path = SDL_GetPrefPath("net.playcontrol", "FlappyBlurrr");
#endif
	if(NULL == pref_path)
	{
		return SDL_FALSE;
	}

	SDL_strlcpy(resource_file_path, pref_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "highscore.txt", MAX_FILE_STRING_LENGTH);

#ifdef __ANDROID__
#else
	SDL_free(pref_path);
#endif
	pref_path = NULL;

	resource_handle = fopen(resource_file_path, "w");
	
	
	if(NULL != resource_handle)
	{
		fprintf(resource_handle, "%d", high_score);
		fclose(resource_handle);
		return SDL_TRUE;

	}
	else
	{
		SDL_Log("Failed to write high score to storage: %s", strerror(errno));
		return SDL_FALSE;
	}

#else
	SDL_RWops* rw_ops;
	
#ifdef __ANDROID__
	const char* pref_path = SDL_AndroidGetInternalStoragePath();
#else
	char* pref_path = SDL_GetPrefPath("net.playcontrol", "FlappyBlurrr");
#endif
	if(NULL == pref_path)
	{
		/* This might be okay because it is a first time run and the file doesn't exist. */
		return SDL_FALSE;
	}
	
	SDL_strlcpy(resource_file_path, pref_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "highscore.txt", MAX_FILE_STRING_LENGTH);
	
#ifdef __ANDROID__
#else
	SDL_free(pref_path);
#endif
	pref_path = NULL;
	
	/* Please be nice. Assumes the file contains a single ASCII number representing the number of sprites. */
	/* RWops is more resiliant because platforms don't always give you access to fopen/fread, like Android's .apk system.  */
	/* However in this case, all our target platforms allow fopen for preference files. */
	/* This is mainly done for demonstration. */
	/* No error checking is done for malicious input. */
	rw_ops = SDL_RWFromFile(resource_file_path, "w");
	if(NULL != rw_ops)
	{
		int i = 0;
		SDL_uitoa(high_score, number_string, 10);
		while('\0' != number_string[i])
		{
			SDL_WriteU8(rw_ops, number_string[i]);
			i = i + 1;
		}

		SDL_RWclose(rw_ops);
		return SDL_TRUE;
	}
	else
	{
		SDL_Log("Failed to write high score to storage: %s", SDL_GetError());
		return SDL_FALSE;
	}
#endif
	
	
}

SDL_bool Flappy_SaveHighScoreToStorageIfNeeded()
{
	/* We write to disk, but don't change the in memory value of previousHighScore
	 	because we need that value for the tally if the game is interrupted/resumed.
	 */
	if(g_gameInstanceData.currentScore > g_highScoreData.savedHighScore)
	{
		g_highScoreData.savedHighScore = g_gameInstanceData.currentScore;
		return Flappy_SaveHighScoreToStorage(g_gameInstanceData.currentScore);
	}
	return SDL_TRUE;
}



MyFloat Flappy_ComputeLinearInterpolation(MyFloat current_value, MyFloat start_value, MyFloat end_value)
{
	/* Use the linear interpolation formula:
	 * X = (1-t)x0 + tx1
	 * where x0 would be the start value
	 * and x1 is the final value
	 * and t is delta_time*inv_time (adjusts 0 <= time <= 1)
	 * delta_time = current_time-start_time
	 * inv_time = 1/ (end_time-start_time)
	 * so t = current_time-start_time / (end_time-start_time)
	 *
	 */
	if(current_value > end_value)
	{
		return 1.0;
	}
	else if(current_value < start_value)
	{
		return 0.0;
	}
	return (current_value-start_value) / (end_value-start_value);
}

void Flappy_StepPhysics(Uint32 delta_time)
{
	// Recommended between 60 and 240; higher = more accuracy (but higher CPU load)
	const MyFloat TICKS_PER_SECOND = 100.0;
	static MyFloat _accumulator = 0.0;

	MyFloat dt = (MyFloat)delta_time * 1.0/1000.0;
	MyFloat fixed_dt = 1.0/TICKS_PER_SECOND;
	
	// add the current dynamic timestep to the accumulator
	_accumulator += dt;
	
	while(_accumulator > fixed_dt)
	{
		cpSpaceStep(g_mainSpace, fixed_dt);
		_accumulator -= fixed_dt;
	}
}


void Flappy_RenderBackground(SDL_Renderer* the_renderer)
{
	/* Render the background.
		We stretch the sprite to fit the screen for convenience.
	 */

	SDL_Rect dst_rect = {0, 0, SCREEN_WIDTH, SCREEN_WIDTH};
	SDL_RenderCopy(the_renderer, g_gameTextures.background, NULL, &dst_rect);
}

/* Render the bush/hills */
void Flappy_RenderHills(SDL_Renderer* the_renderer)
{
	
	SDL_Rect src_rect = {0, 0, g_bushModelData.size.x, g_bushModelData.size.y};
	/* The ground height in the image is actually really tall and we don't want all of it.
	 So we can truncate the bottom part that we don't want.
	 We'll take 1/5 of the image.
	 */
	SDL_Rect dst_rect = {g_bushModelData.position.x, Flappy_InvertY(g_groundModelData.size.y + g_bushModelData.size.y), g_bushModelData.size.x, g_bushModelData.size.y};
	
	
	/* The texture does not fill the whole width of the screen.
	 So draw it multiple times, shifted over
	 until we fill the entire length of the screen.
	 */
	Sint32 number_of_times_to_repeat_draw;
	Sint32 remainder;
	Sint32 i;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_QueryTexture(g_gameTextures.bush, &the_format, &the_access, &the_width, &the_height);
	
	number_of_times_to_repeat_draw = (SCREEN_WIDTH / the_width) + 1;
	remainder = SCREEN_WIDTH % the_width;
	
	if(remainder > 0)
	{
		number_of_times_to_repeat_draw = number_of_times_to_repeat_draw + 1;
	}
	
	for(i=0; i<number_of_times_to_repeat_draw; i++)
	{
		dst_rect.x = g_bushModelData.position.x + g_bushModelData.size.x * i;
		if(dst_rect.x < SCREEN_WIDTH)
		{
//			SDL_RenderCopy(renderer, g_gameTextures.bush, &src_rect, &dst_rect);
			SDL_RenderCopy(the_renderer, g_gameTextures.bush, NULL, &dst_rect);
		}
	}
}


void Flappy_RenderClouds(SDL_Renderer *the_renderer)
{
	/* Render the clouds.
	 * Our sprite actually contains multiple clouds, so we'll just render the whole thing.
	 * To make it look continuous, we'll render it twice, one following the other while moving to the left.
	 */
	
	{
		
		
		SDL_Rect src_rect = {0, 0, g_cloudModelData.size.x, g_cloudModelData.size.y};
		
		SDL_Rect dst_rect = {g_cloudModelData.position.x, 0, g_cloudModelData.size.x, g_cloudModelData.size.y};
		
		
		/* The texture does not fill the whole width of the screen. 
			So draw it multiple times, shifted over
			until we fill the entire length of the screen.
		 */
		Sint32 number_of_times_to_repeat_draw;
		Sint32 remainder;
		Sint32 i;
		
		Uint32 the_format;
		int the_access;
		int the_width;
		int the_height;
		
		SDL_QueryTexture(g_gameTextures.clouds, &the_format, &the_access, &the_width, &the_height);
		
		number_of_times_to_repeat_draw = (SCREEN_WIDTH / the_width) + 1;
		remainder = SCREEN_WIDTH % the_width;
		if(remainder > 0)
		{
			number_of_times_to_repeat_draw = number_of_times_to_repeat_draw + 1;
		}
		
		for(i=0; i<number_of_times_to_repeat_draw; i++)
		{
			dst_rect.x = g_cloudModelData.position.x + g_cloudModelData.size.x * i;
			if(dst_rect.x < SCREEN_WIDTH)
			{
				//			SDL_RenderCopy(renderer, g_gameTextures.clouds, &src_rect, &dst_rect);
				SDL_RenderCopy(the_renderer, g_gameTextures.clouds, NULL, &dst_rect);
			}
		}
		
	}
}


void Flappy_RenderPipes(SDL_Renderer* the_renderer)
{
	/*
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;
	
	current_position = g_playerModelData.position.x;
	
	shift_factor = SDL_abs(current_position) / g_groundModelData.size.x;
	g_groundModelData.position.x = (g_groundModelData.size.x * shift_factor) + current_position;
	
	
	PIPE_DISTANCE

	g_playerModelData.position.x
	*/
	
	
	
	
//		SDL_Rect src_rect = {0, 0, 106, 672};
	
	
	
	/* The texture does not fill the whole width of the screen.
	 So draw it multiple times, shifted over
	 until we fill the entire length of the screen.
	 */
	Sint32 number_of_times_to_repeat_draw;
	Sint32 remainder;
	Sint32 i;
	
	number_of_times_to_repeat_draw = (SCREEN_WIDTH / PIPE_DISTANCE) + 1;
	remainder = SCREEN_WIDTH % PIPE_DISTANCE;
	if(remainder > 0)
	{
		number_of_times_to_repeat_draw = number_of_times_to_repeat_draw + 1;
	}
	
	for(i=0; i<CircularQueueVoid_Size(g_circularQueueOfPipes); i++)
	{

//			dst_rect.x = g_pipeTopModelData.position.x + PIPE_DISTANCE * i;
		struct PipeModelData* current_pipe = (struct PipeModelData*)CircularQueueVoid_ValueAtIndex(g_circularQueueOfPipes, i);
		SDL_Rect src_rect = {0, 0, g_pipeTopModelData.size.x, current_pipe->pipeHeight};
		SDL_Rect dst_rect = {g_pipeTopModelData.position.x, current_pipe->lowerViewPositionY, g_pipeTopModelData.size.x, current_pipe->pipeHeight};

		dst_rect.x = current_pipe->viewPositionX -  g_pipeTopModelData.size.x/2;
//			dst_rect.y = Flappy_InvertY(current_pipe->pipeHeight+GROUND_HEIGHT);

		SDL_RenderCopy(the_renderer, g_gameTextures.pipe_bottom, &src_rect, &dst_rect);
//			SDL_RenderCopy(renderer, g_gameTextures.pipe_bottom, NULL, &dst_rect);
		
//			src_rect.y = 512 - current_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION;
		src_rect.y = 0;
//			src_rect.h = SCREEN_HEIGHT - current_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT;
		src_rect.h = 512;
//			dst_rect.y = current_pipe->upperViewPositionY - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT - current_pipe->pipeHeight;
//			dst_rect.y = current_pipe->upperViewPositionY;
		dst_rect.y = SCREEN_HEIGHT - current_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT - 512;
		dst_rect.h = 512;
//			dst_rect.h = SCREEN_HEIGHT - current_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT;

		SDL_RenderCopy(the_renderer, g_gameTextures.pipe_top, &src_rect, &dst_rect);
//			SDL_RenderCopy(renderer, g_gameTextures.pipe_top, NULL, &dst_rect);


	}
}

void Flappy_RenderBird(SDL_Renderer* the_renderer)
{
	/* render bird */

	SDL_Rect src_rect = {0, 0, g_birdModelData.size.x, g_birdModelData.size.y};
	/* The ground height in the image is actually really tall and we don't want all of it.
	 So we can truncate the bottom part that we don't want.
	 We'll take 1/5 of the image.
	 */
	SDL_Rect dst_rect = {g_birdModelData.position.x - g_birdModelData.size.x/2, g_birdModelData.position.y, g_birdModelData.size.x, g_birdModelData.size.y};
	
	
	
	//		SDL_RenderCopy(renderer, g_gameTextures.fly1, NULL, &dst_rect);
	SDL_RenderCopyEx(the_renderer, g_birdModelData.currentTexture, NULL, &dst_rect, -cpBodyGetAngle(g_birdModelData.birdBody)/(2.0*M_PI/360.0), NULL, SDL_FLIP_NONE);
}

/* render ground */
void Flappy_RenderGround(SDL_Renderer* the_renderer)
{
	
	
	SDL_Rect src_rect = {0, 0, g_groundModelData.size.x, g_groundModelData.size.y};
	/* The ground height in the image is actually really tall and we don't want all of it.
	 So we can truncate the bottom part that we don't want.
	 We'll take 1/5 of the image.
	 */
	SDL_Rect dst_rect = {g_groundModelData.position.x, Flappy_InvertY(g_groundModelData.size.y), g_groundModelData.size.x, g_groundModelData.size.y};
	
	
	/* The texture does not fill the whole width of the screen.
	 So draw it multiple times, shifted over
	 until we fill the entire length of the screen.
	 */
	Sint32 number_of_times_to_repeat_draw;
	Sint32 remainder;
	Sint32 i;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_QueryTexture(g_gameTextures.ground, &the_format, &the_access, &the_width, &the_height);
	
	number_of_times_to_repeat_draw = (SCREEN_WIDTH / the_width) + 1;
	remainder = SCREEN_WIDTH % the_width;
	if(remainder > 0)
	{
		number_of_times_to_repeat_draw = number_of_times_to_repeat_draw + 1;
	}
	
	for(i=0; i<number_of_times_to_repeat_draw; i++)
	{
		dst_rect.x = g_groundModelData.position.x + g_groundModelData.size.x * i;
		if(dst_rect.x < SCREEN_WIDTH)
		{
			SDL_RenderCopy(the_renderer, g_gameTextures.ground, &src_rect, &dst_rect);
			//			SDL_RenderCopy(renderer, g_gameTextures.ground, NULL, &dst_rect);
		}
	}
	
}

void Flappy_RenderCurrentScore(SDL_Renderer *the_renderer)
{
	if((g_gameState >= GAMESTATE_BIRD_PRELAUNCH) && (g_gameState < GAMESTATE_SWOOPING_IN_GAME_OVER))
	{
		/* render current score */
		
		Uint32 the_format;
		int the_access;
		int the_width;
		int the_height;
		SDL_Rect dst_rect;
		
		SDL_QueryTexture(g_currentScoreTextTexture, &the_format, &the_access, &the_width, &the_height);
		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2 + 4;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2 + 4;
		dst_rect.w = the_width;
		dst_rect.h = the_height;
		SDL_SetTextureColorMod(g_currentScoreTextTexture, 0, 0, 0);
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		//		SDL_Log("text texture w,h: %d, %d", the_width, the_height);
		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2 - 2;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2 - 2;
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2 + 2;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2 + 2;
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2 + 2;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2 - 2;
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2 - 2;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2 + 2;
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		
		SDL_SetTextureColorMod(g_currentScoreTextTexture, 255, 255, 255);

		
		dst_rect.x = SCREEN_WIDTH / 2 - the_width/2;
		dst_rect.y = SCREEN_HEIGHT/12 - the_height/2;
		dst_rect.w = the_width;
		dst_rect.h = the_height;
		
		SDL_RenderCopy(the_renderer, g_currentScoreTextTexture, NULL, &dst_rect);
		
		
		
		
	}
}

void Flappy_RenderGameOverText(SDL_Renderer *the_renderer)
{
	if(g_gameState >= GAMESTATE_SWOOPING_IN_GAME_OVER)
		//		if(SDL_TRUE == g_birdModelData.isGameOver)
	{


		SDL_RenderCopy(the_renderer, g_textTextureData.gameOverTextTexture, NULL, &g_gameOverDisplayData.dstRect);
		
	}
}

void Flappy_RenderMedalScene(SDL_Renderer *the_renderer)
{
	if(g_gameState >= GAMESTATE_SWOOPING_IN_MEDAL_DISPLAY)
		//		if(SDL_TRUE == g_birdModelData.isGameOver)
	{
		
		
		//		SDL_RenderCopy(renderer, g_gameTextures.medalBackground, NULL, &g_medalBackgroundData.dstRect);
		SDL_RenderCopy(the_renderer, g_gameTextures.medalSceneRenderToTexture, NULL, &g_medalBackgroundData.dstRect);
		//			SDL_RenderCopy(renderer, g_textTextureData.getReadyTextTexture, NULL, &dst_rect);
		
	}
}

void Flappy_RenderGetReadyText(SDL_Renderer *the_renderer)
{
	//        if(g_birdModelData.isPrelaunch)
	if((g_gameState >= GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN) && (g_gameState <= GAMESTATE_BIRD_LAUNCHED_BUT_BEFORE_PIPES))
	{
		SDL_Rect dst_rect = {0, 0, 512, 256 };
		Uint32 the_format;
		int the_access;
		int the_width;
		int the_height;
		SDL_QueryTexture(g_textTextureData.getReadyTextTexture, &the_format, &the_access, &the_width, &the_height);
		
		dst_rect.w = the_width;
		dst_rect.h = the_height;
		
		dst_rect.x = (SCREEN_WIDTH / 2) - (the_width / 2);
		dst_rect.y = SCREEN_HEIGHT / 5;
		
		
		if(g_gameState == GAMESTATE_BIRD_LAUNCHED_BUT_BEFORE_PIPES)
		{
			MyFloat linear_interp;
			Uint32 current_time = TimeTicker_GetTime(g_gameClock);
#define GET_READY_TEXT_FADE_OUT_TIME 1000
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameStateCurrentPhaseStartTime, g_gameStateCurrentPhaseStartTime + GET_READY_TEXT_FADE_OUT_TIME);
			Uint8 alpha = 255 - linear_interp*255;
			
			SDL_SetTextureAlphaMod(g_textTextureData.getReadyTextTexture, alpha);
			SDL_RenderCopy(the_renderer, g_textTextureData.getReadyTextTexture, NULL, &dst_rect);
		}
		else
		{
			SDL_SetTextureAlphaMod(g_textTextureData.getReadyTextTexture, 255);
			SDL_RenderCopy(the_renderer, g_textTextureData.getReadyTextTexture, NULL, &dst_rect);
		}
		//		SDL_RenderCopy(renderer, g_gameTextures.medalBackground, NULL, &g_medalBackgroundData.dstRect);
		
	}
}

/* Note: This draws both "FlappyBlurrr" and the tagline text below it. */
void Flappy_RenderTitleScreenText(SDL_Renderer* the_renderer)
{
	if( (g_gameState >= GAMESTATE_TITLE_SCREEN) && (g_gameState <= GAMESTATE_FADE_OUT_TITLE_SCREEN) )
	{
		SDL_Rect dst_rect = {0, 0, 512, 256 };
		Uint32 the_format;
		int the_access;
		int the_width;
		int the_height;
		int running_height;
		SDL_QueryTexture(g_textTextureData.flappyBlurrrTexture, &the_format, &the_access, &the_width, &the_height);
		
		dst_rect.w = the_width;
		dst_rect.h = the_height;
		
		dst_rect.x = (SCREEN_WIDTH / 2) - (the_width / 2);
		dst_rect.y = SCREEN_HEIGHT / 7;
		
		SDL_RenderCopy(the_renderer, g_textTextureData.flappyBlurrrTexture, NULL, &dst_rect);
		
		running_height = dst_rect.y;
		
		
		
		SDL_QueryTexture(g_textTextureData.taglineTexture, &the_format, &the_access, &the_width, &the_height);
		
		dst_rect.w = the_width;
		dst_rect.h = the_height;
		
		dst_rect.x = (SCREEN_WIDTH / 2) - (the_width / 2);
		dst_rect.y = running_height + the_height;
		
		SDL_RenderCopy(the_renderer, g_textTextureData.taglineTexture, NULL, &dst_rect);
		
		
		
	}
	
}

/* Renders the play button and quit button as necessary */
void Flappy_RenderGuiButtons(SDL_Renderer* the_renderer)
{
	if( ((g_gameState >= GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION) && (g_gameState <= GAMESTATE_FADE_OUT_GAME_OVER) )
	   || ((g_gameState >= GAMESTATE_TITLE_SCREEN) && (g_gameState <= GAMESTATE_FADE_OUT_TITLE_SCREEN) )
	   )
	{
		SDL_Rect dst_rect;
		dst_rect = g_guiPlayButton.dstRect;
		
		if(SDL_TRUE == g_guiPlayButton.isSelected)
		{
			SDL_SetTextureColorMod(g_guiPlayButton.currentTexture, 255, 255, 255);
		}
		else
		{
			SDL_SetTextureColorMod(g_guiPlayButton.currentTexture, 180, 180, 180);
		}
		if(SDL_TRUE == g_guiPlayButton.isPressed)
		{
			dst_rect.x = dst_rect.x + 6;
			dst_rect.y = dst_rect.y + 6;
		}
		else
		{
		}
		SDL_RenderCopy(the_renderer, g_guiPlayButton.currentTexture, NULL, &dst_rect);
		
#if FLAPPY_PROVIDE_QUIT_BUTTON
		if(SDL_TRUE == g_guiQuitButton.isSelected)
		{
			SDL_SetTextureColorMod(g_guiQuitButton.currentTexture, 255, 255, 255);
		}
		else
		{
			SDL_SetTextureColorMod(g_guiQuitButton.currentTexture, 180, 180, 180);
		}
		
		
		dst_rect = g_guiQuitButton.dstRect;
		if(SDL_TRUE == g_guiQuitButton.isPressed)
		{
			dst_rect.x = dst_rect.x + 6;
			dst_rect.y = dst_rect.y + 6;
		}
		else
		{
		}
		
		SDL_RenderCopy(the_renderer, g_guiQuitButton.currentTexture, NULL, &dst_rect);
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
		
	}
}


void Flappy_RenderWhiteImpactFlash(SDL_Renderer* the_renderer)

{
	if(g_gameInstanceData.needsWhiteOut == SDL_TRUE)
	{
		Uint32 current_time = TimeTicker_GetTime(g_gameClock);
		float linear_interp;
		SDL_Rect dst_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
		
#define WHITE_OUT_TIME 200
		if(current_time > (g_gameInstanceData.diedAtTime + WHITE_OUT_TIME))
		{
			// FIXME: this needs to be moved/deleted. For testing only.
			g_gameInstanceData.needsWhiteOut = SDL_FALSE;
			
			
		}
		else
		{
			/* Use the linear interpolation formula:
			 * X = (1-t)x0 + tx1
			 * where x0 would be the start value
			 * and x1 is the final value
			 * and t is delta_time*inv_time (adjusts 0 <= time <= 1)
			 * delta_time = current_time-start_time
			 * inv_time = 1/ (end_time-start_time)
			 * so t = current_time-start_time / (end_time-start_time)
			 *
			 */
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameInstanceData.diedAtTime, g_gameInstanceData.diedAtTime+WHITE_OUT_TIME);
			Uint8 alpha = 255 - linear_interp*255;
			SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(the_renderer, 255, 255, 255, alpha);
			//				SDL_RenderFillRect(renderer, &dst_rect);
			SDL_RenderFillRect(the_renderer, NULL);
		}
		
	}
}


void Flappy_RenderFadeOut(SDL_Renderer* the_renderer)
/* Render black fade-out */
{
	if((GAMESTATE_FADE_OUT_GAME_OVER == g_gameState)
				|| (GAMESTATE_FADE_OUT_TITLE_SCREEN == g_gameState)
	   )
	{
		Uint32 current_time = TimeTicker_GetTime(g_gameClock);
		float linear_interp;
		SDL_Rect dst_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
		
		if(current_time > (g_gameStateCurrentPhaseStartTime + FADE_OUT_TIME))
		{
			
			SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, 255);
			//				SDL_RenderFillRect(renderer, &dst_rect);
			SDL_RenderFillRect(the_renderer, NULL);
		}
		else
		{
			/* Use the linear interpolation formula:
			 * X = (1-t)x0 + tx1
			 * where x0 would be the start value
			 * and x1 is the final value
			 * and t is delta_time*inv_time (adjusts 0 <= time <= 1)
			 * delta_time = current_time-start_time
			 * inv_time = 1/ (end_time-start_time)
			 * so t = current_time-start_time / (end_time-start_time)
			 *
			 */
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameStateCurrentPhaseStartTime, g_gameStateCurrentPhaseStartTime+FADE_OUT_TIME);
			Uint8 alpha = linear_interp*255;
			SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, alpha);
			//				SDL_RenderFillRect(renderer, &dst_rect);
			SDL_RenderFillRect(the_renderer, NULL);
		}
		
	}
}

void Flappy_RenderFadeIn(SDL_Renderer* the_renderer)

/* Render black fade-in */
{
	if(GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN == g_gameState)
	{
		Uint32 current_time = TimeTicker_GetTime(g_gameClock);
		float linear_interp;
		SDL_Rect dst_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
		
		if(current_time > (g_gameStateCurrentPhaseStartTime + FADE_IN_TIME))
		{
			
			SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, 0);
			//				SDL_RenderFillRect(renderer, &dst_rect);
			SDL_RenderFillRect(the_renderer, NULL);
		}
		else
		{
			/* Use the linear interpolation formula:
			 * X = (1-t)x0 + tx1
			 * where x0 would be the start value
			 * and x1 is the final value
			 * and t is delta_time*inv_time (adjusts 0 <= time <= 1)
			 * delta_time = current_time-start_time
			 * inv_time = 1/ (end_time-start_time)
			 * so t = current_time-start_time / (end_time-start_time)
			 *
			 */
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameStateCurrentPhaseStartTime, g_gameStateCurrentPhaseStartTime+FADE_IN_TIME);
			Uint8 alpha = 255 - linear_interp*255;
			SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, alpha);
			//				SDL_RenderFillRect(renderer, &dst_rect);
			SDL_RenderFillRect(the_renderer, NULL);
		}
		
	}
}

void TemplateHelper_RenderFPS(SDL_Renderer* the_renderer)
{
	SDL_Rect dst_rect;
	/* FPS counter */
	if(s_lastRecordedFPS != s_lastDrawnFPS)
	{
		char fps_string[32];
		SDL_Color text_color = { 255, 255, 255, 0 };
		s_lastDrawnFPS = s_lastRecordedFPS;
		SDL_snprintf(fps_string, 32, "%.2f fps", s_lastRecordedFPS);
		
		SDL_FreeSurface(s_surfaceFPS);
		s_surfaceFPS = TTF_RenderText_Blended(s_veraMonoFont, fps_string, text_color);
		
		SDL_DestroyTexture(s_textureFPS);
		s_textureFPS = SDL_CreateTextureFromSurface(the_renderer, s_surfaceFPS);
	}
	
	if(NULL != s_textureFPS)
	{
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.w = s_surfaceFPS->w;
		dst_rect.h = s_surfaceFPS->h;
		
		SDL_RenderCopy(the_renderer, s_textureFPS, NULL, &dst_rect);
	}
}

void render(SDL_Renderer* the_renderer)
{
	
 
	/* fill background in with black */
    SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, 255);
    SDL_RenderClear(the_renderer);


	Flappy_RenderBackground(the_renderer);
	Flappy_RenderHills(the_renderer);
	Flappy_RenderClouds(the_renderer);
	Flappy_RenderPipes(the_renderer);

	Flappy_RenderBird(the_renderer);
	
	/* Note that when Flappy does a face-plant, its beak is obscured by the ground.
		This means the ground gets rendered after the bird.
	*/
	Flappy_RenderGround(the_renderer);
	

	
	// for debugging sizes
#if 0
	{
		
		SDL_Rect src_rect = {0, 0, g_birdModelData.size.x, g_birdModelData.size.y};
		/* The ground height in the image is actually really tall and we don't want all of it.
		 So we can truncate the bottom part that we don't want.
		 We'll take 1/5 of the image.
		 */
		SDL_Rect dst_rect = {0, g_birdModelData.position.y, g_birdModelData.size.x, g_birdModelData.size.y};
		
		for(i=0; i<=SCREEN_HEIGHT; i+=g_birdModelData.size.y)
		{
			dst_rect.y = Flappy_InvertY(0 + g_birdModelData.size.y + i);
		
		
			SDL_RenderCopy(renderer, g_gameTextures.bird[1], NULL, &dst_rect);
		}
		for(i=0; i<=SCREEN_WIDTH; i+=g_birdModelData.size.x)
		{
			dst_rect.x = g_birdModelData.size.x + i;
			dst_rect.y = Flappy_InvertY(0) - g_birdModelData.size.y;

			
			SDL_RenderCopy(renderer, g_gameTextures.bird[1], NULL, &dst_rect);
		}
	}
#endif

	Flappy_RenderCurrentScore(the_renderer);

	Flappy_RenderGameOverText(the_renderer);
	
	// TODO: This draws the final composited texture, not the render to texture part.
	Flappy_RenderMedalScene(the_renderer);
	

	Flappy_RenderGetReadyText(the_renderer);

	/* Note: This draws both "FlappyBlurrr" and the tagline text below it. */
	Flappy_RenderTitleScreenText(the_renderer);

	/* Renders the play button and quit button as necessary */
	Flappy_RenderGuiButtons(the_renderer);
	
	Flappy_RenderWhiteImpactFlash(the_renderer);
	
	Flappy_RenderFadeOut(the_renderer);
	Flappy_RenderFadeIn(the_renderer);


	

	// Disable this to stop drawing fps counter
	TemplateHelper_RenderFPS(the_renderer);

    /* update screen */
    SDL_RenderPresent(the_renderer);
	
}

//#define GROUND_VELOCITY_X -SCREEN_WIDTH / 5000.0f
// Original flappy seems to move across at about 3 seconds on 768 pixels (iPad)
//#define GROUND_VELOCITY_X -768 / 3000.0f
#define GROUND_VELOCITY_X -768.0 / 2800.0
#define BIRD_VELOCITY_X -1*GROUND_VELOCITY_X

//#define BUSH_VELOCITY_X -SCREEN_WIDTH / 200000.0f
#define BUSH_VELOCITY_X -SCREEN_WIDTH / 80000.0f
#define CLOUD_VELOCITY_X -SCREEN_WIDTH / 20000.0f

#define MAX_NUM_PIPES_REMAINDER SCREEN_WIDTH % PIPE_DISTANCE
#if MAX_NUM_PIPES_REMAINDER
	#define MAX_NUM_PIPES SCREEN_WIDTH / PIPE_DISTANCE + 2
#else
	#define MAX_NUM_PIPES SCREEN_WIDTH / PIPE_DISTANCE + 1
#endif
static void Flappy_InitializeCloudModelData(SDL_Texture* the_texture)
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_zero(g_cloudModelData);
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// 960 / 5000 msec = .192
	g_cloudModelData.velocity.x = CLOUD_VELOCITY_X;
	g_cloudModelData.size.x = the_width;
	g_cloudModelData.size.y = the_height;
}

static void Flappy_UpdateCloudPositions(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;

//	g_cloudModelData.position.x = Flappy_lroundf((MyFloat)g_cloudModelData.position.x + g_cloudModelData.velocity.x * (MyFloat)delta_time);
	current_position = Flappy_lroundf(g_cloudModelData.velocity.x * (MyFloat)diff_time);

	shift_factor = SDL_abs(current_position) / g_cloudModelData.size.x;
	g_cloudModelData.position.x = (g_cloudModelData.size.x * shift_factor) + current_position;
	/*
	if(g_cloudModelData.position.x < (0 - g_cloudModelData.size.x))
	{
		g_cloudModelData.position.x = g_cloudModelData.position.x + g_cloudModelData.size.x;
	}
	 */
}




static void Flappy_InitializeGroundModelData(SDL_Texture* the_texture)
{
	/*
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	*/
	
	SDL_zero(g_playerModelData);
	SDL_zero(g_groundModelData);
//	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// 960 / 5000 msec = .192
	g_groundModelData.velocity.x = GROUND_VELOCITY_X;
//	g_groundModelData.velocity.x = -0.1f;
	g_groundModelData.size.x = SCREEN_WIDTH;
	/* The ground height in the image is actually really tall and we don't want all of it.
	 So we can truncate the bottom part that we don't want.
	 We'll take 1/5 of the image.
	 */
	g_groundModelData.size.y = GROUND_HEIGHT;
	

}

static void Flappy_UpdateGroundPosition(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	/*
	g_groundModelData.position.x = Flappy_lroundf((MyFloat)g_groundModelData.position.x + g_groundModelData.velocity.x * (MyFloat)delta_time);
	if(g_groundModelData.position.x < (0 - g_groundModelData.size.x))
	{
		g_groundModelData.position.x = g_groundModelData.position.x + g_groundModelData.size.x;
	}
	*/
	
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;
	
	current_position = Flappy_lroundf(g_groundModelData.velocity.x * (MyFloat)diff_time);
	
	shift_factor = SDL_abs(current_position) / g_groundModelData.size.x;
	g_groundModelData.position.x = (g_groundModelData.size.x * shift_factor) + current_position;

	
	g_playerModelData.position.x = current_position;
	
	
	
	
	// physics

	{
		
		//cpFloat ground_height = (1024/16 * 1.9) * 768 / 1024;
		
	current_position = Flappy_lroundf(g_birdModelData.velocity.x * (MyFloat)diff_time);

	


		cpBodySetPosition(g_groundPhysicsData.body, cpv(current_position + SCREEN_WIDTH/2, GROUND_HEIGHT/2.0));
//		cpBodySetPosition(g_groundPhysicsData.body, cpv(current_position + SCREEN_WIDTH/2, GROUND_HEIGHT));
//	cpBodySetPosition(g_groundPhysicsData.body, cpv(current_position + SCREEN_WIDTH/2, 0));
		cpSpaceReindexShape(g_mainSpace, g_groundPhysicsData.shape);
//		SDL_Log("current_bird pos:%f, %d\n", g_birdModelData.birdBody->p.x, current_position);
	}
}


static void Flappy_InitializeBushModelData(SDL_Texture* the_texture)
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_zero(g_bushModelData);
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// 960 / 5000 msec = .192
	g_bushModelData.velocity.x = BUSH_VELOCITY_X;
	g_bushModelData.size.x = the_width;
	g_bushModelData.size.y = the_height;
}

static void Flappy_UpdateBushPositions(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;
	
	//	g_cloudModelData.position.x = Flappy_lroundf((MyFloat)g_cloudModelData.position.x + g_cloudModelData.velocity.x * (MyFloat)delta_time);
	current_position = Flappy_lroundf(g_bushModelData.velocity.x * (MyFloat)diff_time);
	
	shift_factor = SDL_abs(current_position) / g_bushModelData.size.x;
	g_bushModelData.position.x = (g_bushModelData.size.x * shift_factor) + current_position;
	/*
	 if(g_cloudModelData.position.x < (0 - g_cloudModelData.size.x))
	 {
	 g_cloudModelData.position.x = g_cloudModelData.position.x + g_cloudModelData.size.x;
	 }
	 */
}


// Intializes both the top and bottom. Assumes the pipe textures are the sime width/height so either may be passed in
static void Flappy_InitializePipeModelData(SDL_Texture* the_texture)
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	Sint32 i;

	SDL_zero(g_pipeTopModelData);
	SDL_zero(g_pipeBottomModelData);
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// 960 / 5000 msec = .192
	g_pipeTopModelData.velocity.x = GROUND_VELOCITY_X;
	g_pipeBottomModelData.velocity.x = GROUND_VELOCITY_X;
	//	g_groundModelData.velocity.x = -0.1f;
	
	g_pipeTopModelData.size.x = the_width;
	g_pipeBottomModelData.size.x = the_width;
//#define PIPE_WIDTH 170
//#define PIPE_WIDTH 106
//#define PIPE_WIDTH 140
//#define PIPE_WIDTH 249
	g_pipeTopModelData.size.x = PIPE_WIDTH;
	g_pipeBottomModelData.size.x = PIPE_WIDTH;

	/* The ground height in the image is actually really tall and we don't want all of it.
	 So we can truncate the bottom part that we don't want.
	 We'll take 1/5 of the image.
	 */
	g_pipeTopModelData.size.y = the_height;
	g_pipeBottomModelData.size.y = the_height;
	
	
	
	g_arrayOfPipes = (struct PipeModelData*)SDL_calloc(MAX_NUM_PIPES, sizeof(struct PipeModelData));
	g_circularQueueOfPipes = CircularQueueVoid_CreateQueue(MAX_NUM_PIPES);
	g_queueOfAvailablePipes = CircularQueueVoid_CreateQueue(MAX_NUM_PIPES);
	for(i=0; i<MAX_NUM_PIPES; i++)
	{
		struct PipeModelData* current_pipe = &g_arrayOfPipes[i];
		
		current_pipe->upperPhysicsBody = cpBodyNewStatic();
		current_pipe->upperPhysicsShape = cpBoxShapeNew(current_pipe->upperPhysicsBody, the_width, SCREEN_HEIGHT, 0.0);
		current_pipe->lowerPhysicsBody = cpBodyNewStatic();
		current_pipe->lowerPhysicsShape = cpBoxShapeNew(current_pipe->lowerPhysicsBody, the_width, SCREEN_HEIGHT, 0.0);

		cpShapeSetCollisionType(current_pipe->upperPhysicsShape, PIPE_COLLISION_TYPE);
		cpShapeSetCollisionType(current_pipe->lowerPhysicsShape, PIPE_COLLISION_TYPE);

		
		current_pipe->scoreSensorPhysicsBody = cpBodyNewStatic();
//		current_pipe->scoreSensorPhysicsShape = cpBoxShapeNew(current_pipe->scoreSensorPhysicsBody, the_width/2, PIPE_HEIGHT_SEPARATION);
		current_pipe->scoreSensorPhysicsShape = cpBoxShapeNew(current_pipe->scoreSensorPhysicsBody, the_width/2, SCREEN_HEIGHT, 0.0);


		
		cpShapeSetSensor(current_pipe->scoreSensorPhysicsShape, cpTrue);
		cpShapeSetCollisionType(current_pipe->scoreSensorPhysicsShape, SCORE_SENSOR_TYPE);

		
		// don't add to space to make it a "rogue" body (intended to be controlled manually)
		//	cpSpaceAddShape(g_mainSpace, g_groundPhysicsData.shape);
		//cpSpaceAddShape(g_mainSpace, g_groundPhysicsData.shape);
		

		
				
		
		CircularQueueVoid_PushBack(g_queueOfAvailablePipes, &g_arrayOfPipes[i]);
	}
}

static void Flappy_UpdatePipePositions(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	/*
	 g_groundModelData.position.x = Flappy_lroundf((MyFloat)g_groundModelData.position.x + g_groundModelData.velocity.x * (MyFloat)delta_time);
	 if(g_groundModelData.position.x < (0 - g_groundModelData.size.x))
	 {
	 g_groundModelData.position.x = g_groundModelData.position.x + g_groundModelData.size.x;
	 }
	 */
	
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;
	Sint32 i;
	
	Sint32 new_offscreen_pipe_position_x;
	Sint32 left_most_pipe_position_x;
	Sint32 right_most_pipe_position_x;

	struct PipeModelData* the_pipe = NULL;

	
	if(g_birdModelData.isPrelaunch)
	{
		return;
	}
//	else if(current_time < g_birdModelData.birdLaunchStartTime + PIPE_START_TIME_DELAY)
    else if(g_gameState < GAMESTATE_MAIN_GAME_ACTIVE)
	{
		return;
	}
	else if(g_birdModelData.isDead)
	{
		return;
	}
	
	// camera position
//	current_position = Flappy_lroundf(g_pipeTopModelData.velocity.x * (MyFloat)diff_time);
	current_position = -1 * Flappy_lroundf(g_pipeTopModelData.velocity.x * (MyFloat)diff_time);
	
//	shift_factor = SDL_abs(current_position) / g_pipeTopModelData.size.x;
	shift_factor = SDL_abs(current_position) / PIPE_DISTANCE;
	g_pipeTopModelData.position.x = (PIPE_DISTANCE * shift_factor) + current_position;
	g_pipeBottomModelData.position.x = (PIPE_DISTANCE * shift_factor) + current_position;
	
	left_most_pipe_position_x = (PIPE_DISTANCE * shift_factor) + current_position;
	
	g_playerModelData.position.x = current_position;
	
	



	/* update all pipe positions in queue */
	/* To avoid drift/rounding-errors due to imperfect frame rate intervals,
		I am avoiding arithmetic based on this previous position.
		Instead, I am trying to use a formula, which will guarantee the perfect placement based on the current time.
		But the trick we need to contend with is how to determine which pipe in the queue corresponds with the correct pipe multiple.
		One way is to do the (bad) arthimetic with delta_time to figure out which 'slot' it is in.
		Then recompute the real position based on that slot.
	 
	*/
	for(i=0; i<CircularQueueVoid_Size(g_circularQueueOfPipes); i++)
	{
		struct PipeModelData* current_pipe = (struct PipeModelData*)CircularQueueVoid_ValueAtIndex(g_circularQueueOfPipes, i);

		current_pipe->viewPositionX = current_pipe->realPositionX - current_position;

		/*
		
		//			dst_rect.x = g_pipeTopModelData.position.x + PIPE_DISTANCE * i;
		
		Sint32 approx_position_x = Flappy_lroundf((MyFloat)current_pipe->position.x + g_groundModelData.velocity.x * (MyFloat)delta_time);

		Sint32 slot_multiple = ((approx_position_x - current_position) / PIPE_DISTANCE ) - shift_factor;
		
//		current_pipe->position.x = (PIPE_DISTANCE * (shift_factor+i)) + current_position;
		current_pipe->position.x = (PIPE_DISTANCE * (shift_factor+slot_multiple)) + current_position;
		
		SDL_Log("\ni:%d, approx:%d, current:%d, slot:%d, x:%d\n", i, approx_position_x, current_position, slot_multiple, current_pipe->position.x);
		 */
//		SDL_Log("\ni:%d, real:%d, view:%d, slot:%d\n", i, current_pipe->realPosition.x, current_pipe->viewPosition, current_pipe->intervalScalar);

	}
	
	
	/* If an old pipe has fallen off the screen, we need to remove it. */
	the_pipe = CircularQueueVoid_Front(g_circularQueueOfPipes);
	
	if((NULL != the_pipe) && ((the_pipe->realPositionX + the_pipe->size.x) < current_position))
	{
//		SDL_Log("removed pipe, x:%d\n", the_pipe->realPositionX);

		cpSpaceRemoveShape(g_mainSpace, the_pipe->scoreSensorPhysicsShape);
		cpSpaceRemoveShape(g_mainSpace, the_pipe->lowerPhysicsShape);
		cpSpaceRemoveShape(g_mainSpace, the_pipe->upperPhysicsShape);

		for(i=0; i<CircularQueueVoid_Size(g_circularQueueOfPipes); i++)
		{
			struct PipeModelData* current_pipe = (struct PipeModelData*)CircularQueueVoid_ValueAtIndex(g_circularQueueOfPipes, i);
//			SDL_Log("\n\tcheckremove1 i:%d, real:%d, view:%d, slot:%d\n", i, current_pipe->realPosition.x, current_pipe->viewPosition, current_pipe->intervalScalar);
		}
		CircularQueueVoid_PopFront(g_circularQueueOfPipes);
		for(i=0; i<CircularQueueVoid_Size(g_circularQueueOfPipes); i++)
		{
			struct PipeModelData* current_pipe = (struct PipeModelData*)CircularQueueVoid_ValueAtIndex(g_circularQueueOfPipes, i);
//			SDL_Log("\n\tcheckremove2 i:%d, real:%d, view:%d, slot:%d\n", i, current_pipe->realPosition.x, current_pipe->viewPosition, current_pipe->intervalScalar);
		}
		
		CircularQueueVoid_PushBack(g_queueOfAvailablePipes, the_pipe);
		
	}
	
	/* If a new pipe will be coming onto the screen, we need to create it. */
	/* Check the queue to see if the pipe has already been created.
	 It should be at the end of the queue and will have the designated x position.
	 */
	//shift_factor = SDL_abs(current_position) / g_bushModelData.size.x;
	//g_bushModelData.position.x = (g_bushModelData.size.x * shift_factor) + current_position;
	
//	right_most_pipe_position_x = left_most_pipe_position_x + PIPE_DISTANCE * MAX_NUM_PIPES;
//	right_most_pipe_position_x = (PIPE_DISTANCE * (MAX_NUM_PIPES)) + current_position;
	right_most_pipe_position_x = (PIPE_DISTANCE * (shift_factor+MAX_NUM_PIPES-1)) + current_position;
	//g_bushModelData.position.x = (g_bushModelData.size.x * shift_factor) + current_position;
	
	right_most_pipe_position_x = current_position + SCREEN_WIDTH + PIPE_WIDTH;
	Sint32 new_pipe_interval_scalar = right_most_pipe_position_x / PIPE_DISTANCE;
	
	
	the_pipe = (struct PipeModelData*)CircularQueueVoid_Back(g_circularQueueOfPipes);
	if((NULL == the_pipe) || (the_pipe->intervalScalar != new_pipe_interval_scalar))
	{
		Sint32 real_x_pos = PIPE_DISTANCE * new_pipe_interval_scalar;
		
		
		if((real_x_pos) >= (current_position + SCREEN_WIDTH))
		{
			
			the_pipe = CircularQueueVoid_Front(g_queueOfAvailablePipes);
			SDL_assert(NULL != the_pipe);
			if(NULL != the_pipe)
			{
				/* 
					http://c-faq.com/lib/randrange.html
					for range [M, N]:
					M + rand() / (RAND_MAX / (N - M + 1) + 1)
				 */
				int M = MIN_PIPE_HEIGHT;
				int N = SCREEN_HEIGHT - GROUND_HEIGHT - PIPE_HEIGHT_SEPARATION - MIN_PIPE_HEIGHT;
				int random_pipe_height = M + rand() / (RAND_MAX / (N - M + 1) + 1);
	//			int random_pipe_height = MIN_PIPE_HEIGHT;

				
				CircularQueueVoid_PopFront(g_queueOfAvailablePipes);
				
				the_pipe->realPositionX = PIPE_DISTANCE * new_pipe_interval_scalar;
				the_pipe->intervalScalar = new_pipe_interval_scalar;
				the_pipe->viewPositionX = the_pipe->realPositionX - current_position;
				
				the_pipe->pipeHeight = random_pipe_height;
				/* Remember that SDL inverts the Y. */

				the_pipe->lowerRealPositionY = GROUND_HEIGHT;
				the_pipe->upperRealPositionY = GROUND_HEIGHT + random_pipe_height + PIPE_HEIGHT_SEPARATION;


				the_pipe->lowerViewPositionY = Flappy_InvertY(
															  the_pipe->lowerRealPositionY
															  
															  )
				- the_pipe->pipeHeight
				
				;
	//														  the_pipe->size.y;
				the_pipe->size.x = g_pipeBottomModelData.size.x;


				cpBodySetPosition(the_pipe->lowerPhysicsBody, cpv(the_pipe->realPositionX, the_pipe->lowerRealPositionY + the_pipe->pipeHeight - SCREEN_HEIGHT/2));
	//			cpBodySetPosition(the_pipe->lowerPhysicsBody, cpv(the_pipe->realPositionX - the_pipe->size.x / 2, the_pipe->lowerRealPositionY + the_pipe->pipeHeight/2));
		//		cpBodySetPosition(the_pipe->upperPhysicsBody, cpv(the_pipe->realPositionX - the_pipe->size.x / 2, the_pipe->upperRealPositionY + (SCREEN_HEIGHT - the_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT)/2));
				cpBodySetPosition(the_pipe->upperPhysicsBody, cpv(the_pipe->realPositionX, the_pipe->upperRealPositionY + (SCREEN_HEIGHT/2)));

	//			cpBodySetPosition(the_pipe->scoreSensorPhysicsBody, cpv(the_pipe->realPositionX + the_pipe->size.x / 4, GROUND_HEIGHT+ random_pipe_height + PIPE_HEIGHT_SEPARATION - PIPE_HEIGHT_SEPARATION/2));
//				cpBodySetPosition(the_pipe->scoreSensorPhysicsBody, cpv(the_pipe->realPositionX, GROUND_HEIGHT+ random_pipe_height + PIPE_HEIGHT_SEPARATION - PIPE_HEIGHT_SEPARATION/2));
				cpBodySetPosition(the_pipe->scoreSensorPhysicsBody, cpv(the_pipe->realPositionX + PIPE_WIDTH/4, SCREEN_HEIGHT/2));

			//	src_rect.h = SCREEN_HEIGHT - current_pipe->pipeHeight - PIPE_HEIGHT_SEPARATION - GROUND_HEIGHT;


				cpSpaceAddShape(g_mainSpace, the_pipe->upperPhysicsShape);
				cpSpaceAddShape(g_mainSpace, the_pipe->lowerPhysicsShape);
				cpSpaceAddShape(g_mainSpace, the_pipe->scoreSensorPhysicsShape);

//				SDL_Log("added pipe, x:%d\n", the_pipe->realPositionX);

				CircularQueueVoid_PushBack(g_circularQueueOfPipes, the_pipe);
			}
		}
	}

	
	
	
	
#if 0
	
	SDL_Rect src_rect = {0, 0, 106, 672};
	
	SDL_Rect dst_rect = {g_pipeTopModelData.position.x, 0, 106, 672};
	
	
	/* The texture does not fill the whole width of the screen.
	 So draw it multiple times, shifted over
	 until we fill the entire length of the screen.
	 */
	Sint32 number_of_times_to_repeat_draw;
	Sint32 remainder;
	Sint32 i;
	
	number_of_times_to_repeat_draw = (SCREEN_WIDTH / PIPE_DISTANCE) + 1;
	remainder = SCREEN_WIDTH % PIPE_DISTANCE;
	if(remainder > 0)
	{
		number_of_times_to_repeat_draw = number_of_times_to_repeat_draw + 1;
	}
	
	for(i=0; i<number_of_times_to_repeat_draw; i++)
	{
		dst_rect.x = g_pipeTopModelData.position.x + PIPE_DISTANCE * i;
		if(dst_rect.x < SCREEN_WIDTH)
		{
			SDL_RenderCopy(renderer, g_gameTextures.pipe_bottom, &src_rect, &dst_rect);
		}
	}
#endif
	
}



// Assumes all the bird textures are the sime width/height so any of the 3 may be passed in
static void Flappy_InitializeBirdModelData(SDL_Texture* the_texture)
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_zero(g_birdModelData);
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// 960 / 5000 msec = .192
	g_birdModelData.velocity.x = 0;
//	g_birdModelData.size.x = the_width;
//	g_birdModelData.size.y = the_height;
//	g_birdModelData.size.x = 92;
//	g_birdModelData.size.y = 74;
//		g_birdModelData.size.x = 62;
//		g_birdModelData.size.y = 50;
	g_birdModelData.size.x = BIRD_WIDTH;
	g_birdModelData.size.y = BIRD_HEIGHT;
	g_birdModelData.position.x = SCREEN_WIDTH / 3;
	g_birdModelData.position.y = SCREEN_HEIGHT / 2;
	
	
	g_birdModelData.currentTexture = g_gameTextures.bird[1];
	g_birdModelData.animationStartTime = 0;
	g_birdModelData.animationDuration = BIRD_FLAP_ANIMATION_DURATION;
	g_birdModelData.isAnimatingSprite = SDL_TRUE;
	
	g_birdModelData.isPrelaunch = SDL_TRUE;
	g_birdModelData.isReadyForPipe = SDL_FALSE;
	g_birdModelData.birdLaunchStartTime = 0;

}

// y is in cartesian
void Flappy_UpdateBirdPositionForPrelaunch(Uint32 delta_time, Uint32 base_time, Uint32 current_time, Sint32 base_bird_pos_x, Sint32 base_bird_pos_y)
{
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;

	cpFloat bird_angle;
	Sint32 pos_y;

	g_birdModelData.velocity.x = BIRD_VELOCITY_X;

	//	g_cloudModelData.position.x = Flappy_lroundf((MyFloat)g_cloudModelData.position.x + g_cloudModelData.velocity.x * (MyFloat)delta_time);
	current_position = Flappy_lroundf(g_birdModelData.velocity.x * (MyFloat)diff_time);
	
	//	shift_factor = SDL_abs(current_position) / g_cloudModelData.size.x;
	//	g_birdModelData.position.x = SCREEN_WIDTH / 3;
	g_birdModelData.position.x = base_bird_pos_x;
	
	pos_y = Flappy_lroundf( g_birdModelData.size.y/4 * SDL_sinf(current_time /1000.0 * 4)) + base_bird_pos_y;
//	pos_y = Flappy_lroundf( g_birdModelData.size.y/4 * SDL_sinf(current_time /1000.0 * 4)) + g_birdModelData.position.y - g_birdModelData.size.y;
		
	g_birdModelData.position.y = Flappy_InvertY(pos_y + (g_birdModelData.size.y / 2));
//	g_birdModelData.birdBody->p.y = pos_y;
	
	cpBodySetPosition(g_birdModelData.birdBody, cpv( g_birdModelData.position.x, pos_y + (g_birdModelData.size.y / 2)));

	
	{
		g_birdModelData.animationDuration = BIRD_FLAP_ANIMATION_DURATION_FOR_PRELAUNCH;
		MyFloat linear_interp;
		Uint32 mod_time = (current_time - g_birdModelData.animationStartTime) % g_birdModelData.animationDuration
;
		Uint32 texture_index;
		
		linear_interp = Flappy_ComputeLinearInterpolation(mod_time, g_birdModelData.animationStartTime, g_birdModelData.animationStartTime+g_birdModelData.animationDuration);
		texture_index = Flappy_lroundf(linear_interp * (MyFloat)NUMBER_OF_BIRD_FRAMES);
		if(texture_index >= NUMBER_OF_BIRD_FRAMES)
		{
			texture_index = 0;
		}
		g_birdModelData.currentTexture = g_gameTextures.bird[texture_index];
		
	}
	
}

void Flappy_UpdateBirdPositionForGame(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	Uint32 diff_time = current_time - base_time;
	Sint32 current_position;
	Sint32 shift_factor;
	cpFloat bird_angle;
	cpVect vec_bird_position;
	cpVect vec_bird_velocity;

	g_birdModelData.velocity.x = BIRD_VELOCITY_X;
	vec_bird_position = cpBodyGetPosition(g_birdModelData.birdBody);

	
	if(g_birdModelData.isDead)
	{
		current_position = g_gameInstanceData.distanceTraveled;
	}
	else
	{
		//	g_cloudModelData.position.x = Flappy_lroundf((MyFloat)g_cloudModelData.position.x + g_cloudModelData.velocity.x * (MyFloat)delta_time);
		current_position = Flappy_lroundf(g_birdModelData.velocity.x * (MyFloat)diff_time);
		g_gameInstanceData.distanceTraveled = current_position;
		
	}
	
	
//	shift_factor = SDL_abs(current_position) / g_cloudModelData.size.x;
//	g_birdModelData.position.x = SCREEN_WIDTH / 3;
	
	
	
	
	
	
	
	g_birdModelData.position.y = Flappy_InvertY(
		Flappy_lroundf(
			vec_bird_position.y
		)
												+ 		(g_birdModelData.size.y / 2)

	);

//	SDL_Log("g_birdModelData.birdBody->p.y=%f, g_birdModelData.position.y=%d", g_birdModelData.birdBody->p.y, g_birdModelData.position.y);
	// Trick to prevent bird from being drawn too far into the ground when physics collision penetrates too far.
	{
		
		int radius = cpCircleShapeGetRadius(g_birdModelData.birdShape);
		// we need 2*radius because SDL draws from the edge of the sprite, not the center.
		int ground_threshold = SCREEN_HEIGHT - GROUND_HEIGHT - (2*radius); // g_birdModelData.size.y
		// should be 640
			
		if(g_birdModelData.position.y > ground_threshold)
		{
			g_birdModelData.position.y = ground_threshold;
		}
	}
//	cpBodySetPosition(g_birdModelData.birdBody, cpv( current_position + (SCREEN_WIDTH / 3) - (g_birdModelData.size.x / 2), g_birdModelData.birdBody->p.y));
	cpBodySetPosition(g_birdModelData.birdBody, cpv( current_position + (SCREEN_WIDTH / 3), vec_bird_position.y));
	vec_bird_velocity = cpBodyGetVelocity(g_birdModelData.birdBody);
	cpBodySetVelocity(g_birdModelData.birdBody, cpv(g_birdModelData.velocity.x, vec_bird_velocity.y));

	bird_angle = cpBodyGetAngle(g_birdModelData.birdBody);
	if((bird_angle < -M_PI_2/4) && (bird_angle > -M_PI_2))
	{
		cpBodySetAngularVelocity(g_birdModelData.birdBody, -5);
		/* Chipmunk API says to reindex after changing the angle */
		cpSpaceReindexShapesForBody(g_mainSpace, g_birdModelData.birdBody);
		g_birdModelData.isAnimatingSprite = SDL_FALSE;
	}
	else if(bird_angle <= -M_PI_2)
	{
		cpBodySetAngularVelocity(g_birdModelData.birdBody, 0);
		cpBodySetAngle(g_birdModelData.birdBody, -M_PI_2);

		/* Chipmunk API says to reindex after changing the angle */
		cpSpaceReindexShapesForBody(g_mainSpace, g_birdModelData.birdBody);

		/* When in a nose dive, texture animation is off. */
		g_birdModelData.isAnimatingSprite = SDL_FALSE;
	}
	else
	{
		g_birdModelData.isAnimatingSprite = SDL_TRUE;
	}
	
	
	if(g_birdModelData.isAnimatingSprite)
	{
		g_birdModelData.animationDuration = BIRD_FLAP_ANIMATION_DURATION;

		MyFloat linear_interp;
		Uint32 mod_time = (current_time - g_birdModelData.animationStartTime) % g_birdModelData.animationDuration;
		Uint32 texture_index;
		
		linear_interp = Flappy_ComputeLinearInterpolation(mod_time, g_birdModelData.animationStartTime, g_birdModelData.animationStartTime+g_birdModelData.animationDuration);
		texture_index = Flappy_lroundf(linear_interp * (MyFloat)NUMBER_OF_BIRD_FRAMES);
		if(texture_index >= NUMBER_OF_BIRD_FRAMES)
		{
			texture_index = 0;
		}
		g_birdModelData.currentTexture = g_gameTextures.bird[texture_index];

	}
	else
	{
		// The middle frame is the one with wings in the neutral position
		g_birdModelData.currentTexture = g_gameTextures.bird[1];
	}
	
	
	if(SDL_TRUE == g_birdModelData.isReadyToPlayFallingSound)
	{
		ALmixer_PlayChannel(-1, g_gameSounds.fall, 0);
		g_birdModelData.isReadyToPlayFallingSound = SDL_FALSE;
		g_birdModelData.isFalling = SDL_FALSE;
	}

	
	
//	SDL_Log("current_bird pos:%f\n", g_birdModelData.birdBody->p.x);
}

void Flappy_UpdateBirdPosition(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
//	if(g_birdModelData.isPrelaunch)
	if((g_gameState >= GAMESTATE_TITLE_SCREEN) && (g_gameState <= GAMESTATE_FADE_OUT_TITLE_SCREEN))
	{
		Flappy_UpdateBirdPositionForPrelaunch(delta_time, base_time, current_time, SCREEN_WIDTH/2, (SCREEN_HEIGHT/3) + 32);
	}
	else if((g_gameState >= GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN) && (g_gameState <= GAMESTATE_BIRD_PRELAUNCH))
	{
		Flappy_UpdateBirdPositionForPrelaunch(delta_time, base_time, current_time, SCREEN_WIDTH/3, SCREEN_HEIGHT/2);
	}
	else
	{
		Flappy_UpdateBirdPositionForGame(delta_time, base_time, current_time);

	}
}

Sint32 Flappy_DetermineCurrentBirdTimePosition(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	if(g_birdModelData.isDead)
	{
//		return g_gameInstanceData.distanceTraveled;
		return g_gameInstanceData.diedAtTime;
	}
	else
	{
		/*
		Uint32 diff_time = current_time - base_time;
		Sint32 current_position;
		
		//	g_cloudModelData.position.x = Flappy_lroundf((MyFloat)g_cloudModelData.position.x + g_cloudModelData.velocity.x * (MyFloat)delta_time);
		current_position = Flappy_lroundf(BIRD_VELOCITY_X * (MyFloat)diff_time);
		return current_position;
		 */
		return current_time;
	}
}



SDL_Texture* Flappy_CreateGameOverTexture(void);

static void Flappy_InitializeGameOverDisplayData()
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	SDL_Texture* the_texture;
	
	SDL_zero(g_gameOverDisplayData);
	the_texture = Flappy_CreateGameOverTexture();
	
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	g_gameOverDisplayData.textureWidth = the_width;
	g_gameOverDisplayData.textureHeight = the_height;
	
	// Midpoint of the screen, then subtract the midpoint of the texture
	g_gameOverDisplayData.dstRect.x = (SCREEN_WIDTH/2) - (the_width/2);
	// This isn't exactly dead center because Game Over and buttons need to be on screen
//	g_gameOverDisplayData.dstRect.y = (SCREEN_HEIGHT/3) - (the_height/2);
//	g_gameOverDisplayData.dstRect.y = (SCREEN_HEIGHT/3) - (the_height/2);
	g_gameOverDisplayData.dstRect.w = the_width;
	g_gameOverDisplayData.dstRect.h = the_height;
	
	
}

static void Flappy_UpdateGameOverDisplayData(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	
	if( ! g_birdModelData.isGameOver)
	{
		return;
	}
	
	
	if(GAMESTATE_SWOOPING_IN_GAME_OVER == g_gameState)
	{
#define GAME_OVER_BOUNCE_HEIGHT 40
		MyFloat linear_interp;
		int displayed_score;
		int lower_target_y = (SCREEN_HEIGHT / 4) - (g_gameOverDisplayData.textureHeight/2) - 40;
		int upper_target_y = lower_target_y - GAME_OVER_BOUNCE_HEIGHT;
		
#define SWEEP_IN_GAME_OVER_DURATION 200
#define SWEEP_IN_GAME_OVER_DELAY_BEFORE_NEXT_STATE 1000

		if(current_time < (g_gameOverDisplayData.sweepInStartTime + (SWEEP_IN_GAME_OVER_DURATION/2)))
		{
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameOverDisplayData.sweepInStartTime, g_gameOverDisplayData.sweepInStartTime + (SWEEP_IN_GAME_OVER_DURATION/2));
			// X = (1-t)x0 + tx1
			g_gameOverDisplayData.dstRect.y = ((1.0-linear_interp) * lower_target_y) + (linear_interp * (MyFloat)(upper_target_y));

		}
		else if(current_time < (g_gameOverDisplayData.sweepInStartTime + (SWEEP_IN_GAME_OVER_DURATION)))
		{
			linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameOverDisplayData.sweepInStartTime + (SWEEP_IN_GAME_OVER_DURATION/2), g_gameOverDisplayData.sweepInStartTime + (SWEEP_IN_GAME_OVER_DURATION));
			// X = (1-t)x0 + tx1
			g_gameOverDisplayData.dstRect.y = ((1.0-linear_interp) * upper_target_y) + (linear_interp * (MyFloat)(lower_target_y));
					
		}
		else
		{
			g_gameOverDisplayData.dstRect.y = lower_target_y;
			if(current_time > (g_gameOverDisplayData.sweepInStartTime + SWEEP_IN_GAME_OVER_DURATION + SWEEP_IN_GAME_OVER_DELAY_BEFORE_NEXT_STATE))
			{
				g_gameState = GAMESTATE_SWOOPING_IN_MEDAL_DISPLAY;
				ALmixer_PlayChannel(-1, g_gameSounds.swoosh, 0);
				g_medalBackgroundData.sweepInStartTime = TimeTicker_GetTime(g_gameClock);
			}

		}
		
		
	}
	
}


static void Flappy_InitializeMedalBackgroundData(SDL_Texture* the_texture)
{
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	SDL_zero(g_medalBackgroundData);
	SDL_QueryTexture(the_texture, &the_format, &the_access, &the_width, &the_height);
	
	// Midpoint of the screen, then subtract the midpoint of the texture
	g_medalBackgroundData.dstRect.x = (SCREEN_WIDTH/2) - (the_width/2);
	// This isn't exactly dead center because Game Over and buttons need to be on screen
	g_medalBackgroundData.dstRect.y = (SCREEN_HEIGHT/2) - (the_height/2);
	g_medalBackgroundData.dstRect.w = the_width;
	
	g_medalBackgroundData.dstRect.h = the_height;
	
	
	/* Create texture for render-to-texture */
    g_gameTextures.medalSceneRenderToTexture = SDL_CreateTexture(g_mainRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, the_width, the_height);
	SDL_assert(NULL != g_gameTextures.medalSceneRenderToTexture);
	SDL_SetTextureBlendMode(g_gameTextures.medalSceneRenderToTexture, SDL_BLENDMODE_BLEND);
	
	g_textTextureData.scoreLabelTextTexture = NULL;
	g_textTextureData.bestLabelTextTexture = NULL;
	g_textTextureData.newHighScoreLabelTextTexture = NULL;
	g_textTextureData.bestNumberTextTexture = NULL;

	
	{
#define MAX_MEDAL_STRING_LENGTH 16
		char the_string[MAX_MEDAL_STRING_LENGTH];
		SDL_Color white_text_color = { 255, 255, 255, 255 };
		//	SDL_Color black_text_color = { 0, 0, 0, 255 };
		SDL_Color orange_text_color = { 220, 120, 0, 255 };
		SDL_Surface* text_surface = NULL;
		
		
		SDL_strlcpy(the_string, "SCORE", MAX_MEDAL_STRING_LENGTH);
		text_surface = TTF_RenderText_Blended(g_textTextureData.fontForMedalLabels, the_string, orange_text_color);
		g_textTextureData.scoreLabelTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
		SDL_FreeSurface(text_surface);

		
		SDL_strlcpy(the_string, "BEST", MAX_MEDAL_STRING_LENGTH);
		text_surface = TTF_RenderText_Blended(g_textTextureData.fontForMedalLabels, the_string, orange_text_color);
		g_textTextureData.bestLabelTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
		SDL_FreeSurface(text_surface);

		
		
		SDL_strlcpy(the_string, "NEW", MAX_MEDAL_STRING_LENGTH);
		text_surface = TTF_RenderText_Blended(g_textTextureData.fontForMedalLabels, the_string, white_text_color);
		g_textTextureData.newHighScoreLabelTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
		SDL_FreeSurface(text_surface);

	}
}



static void Flappy_UpdateMedalBackgroundData(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	
	if( ! g_birdModelData.isGameOver)
	{
		return;
	}

	
	if((g_gameState >= GAMESTATE_SWOOPING_IN_MEDAL_DISPLAY) && (g_gameState <= GAMESTATE_TALLYING_SCORE))
	{
		MyFloat linear_interp;
		int displayed_score;
		
#define SWEEP_IN_MEDAL_PANEL_DURATION 300
		linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_medalBackgroundData.sweepInStartTime, g_medalBackgroundData.sweepInStartTime + SWEEP_IN_MEDAL_PANEL_DURATION);

		
		int the_height = 250;
		// X = (1-t)x0 + tx1
		g_medalBackgroundData.dstRect.y = ((1.0-linear_interp) * (MyFloat)SCREEN_HEIGHT) + (linear_interp * (MyFloat)((SCREEN_HEIGHT/2) - (the_height/2)));
		
		/*
		if(linear_interp >= 1.0)
		{
			g_gameState = GAMESTATE_SWOOPED_IN_MEDAL_DISPLAY;
		}
		 */
		if((GAMESTATE_SWOOPING_IN_MEDAL_DISPLAY == g_gameState) && (current_time >= g_medalBackgroundData.sweepInStartTime + SWEEP_IN_MEDAL_PANEL_DURATION))
		{
			g_gameState = GAMESTATE_TALLYING_SCORE;
			g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
		}
		
		Flappy_SaveHighScoreToStorageIfNeeded();

	}
	
	
	{
		
		SDL_SetRenderTarget(g_mainRenderer, g_gameTextures.medalSceneRenderToTexture);
		
		// Clear screen
		SDL_SetRenderDrawColor(g_mainRenderer, 0, 0, 0, 0 );
		SDL_RenderClear(g_mainRenderer );
		
		{
//			SDL_Rect dst_rect = {0, 0, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
			SDL_RenderCopy(g_mainRenderer, g_gameTextures.medalBackground, NULL, NULL);
		}
		
		
		{
#if 0
		/* Max score in Flappy Bird is 9999 */
#define MAX_MEDAL_STRING_LENGTH 1024
		char the_string[MAX_MEDAL_STRING_LENGTH];
		SDL_Color white_text_color = { 255, 255, 255, 255 };
		SDL_Color black_text_color = { 0, 0, 0, 255 };
		SDL_Color orange_text_color = { 220, 120, 0, 255 };
		SDL_Surface* text_surface = NULL;
		
		SDL_DestroyTexture(g_textTextureData.scoreLabelTextTexture);
		
		SDL_strlcpy(the_string, "SCORE", MAX_MEDAL_STRING_LENGTH);
		text_surface = TTF_RenderText_Blended(g_textTextureData.fontForMedalLabels, the_string, orange_text_color);
		g_textTextureData.scoreLabelTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
		
		//	SDL_Log("text surface w,h: %d, %d", text_surface->w, text_surface->h);
		
		
		SDL_FreeSurface(text_surface);
		
#endif
			Uint32 the_format;
			int the_access;
			int the_width;
			int the_height;
			SDL_QueryTexture(g_textTextureData.scoreLabelTextTexture, &the_format, &the_access, &the_width, &the_height);
#define TOP_MARGIN 16
#define RIGHT_MARGIN 60
			/* Draw "SCORE" label */

			{
//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, TOP_MARGIN, the_width, the_height};
			SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreLabelTextTexture, NULL, &dst_rect);
			}

			
			{
				int running_width;
				/* Draw "BEST" label */
				{
				SDL_QueryTexture(g_textTextureData.bestLabelTextTexture, &the_format, &the_access, &the_width, &the_height);

				//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
				SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, TOP_MARGIN + 116, the_width, the_height};
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestLabelTextTexture, NULL, &dst_rect);
					running_width = dst_rect.w;

				}
				
				/* Draw "NEW" label */
				if( ((g_gameState >= GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION) && (g_gameState <= GAMESTATE_FADE_OUT_GAME_OVER) )
				   	&& (g_gameInstanceData.currentScore > g_highScoreData.previousHighScore)
				)
				{
					SDL_QueryTexture(g_textTextureData.newHighScoreLabelTextTexture, &the_format, &the_access, &the_width, &the_height);
					{
					//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
					SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN - running_width - 20, TOP_MARGIN + 116, the_width, the_height};
					SDL_Rect dst_rect_red_box = dst_rect;
					dst_rect_red_box.w = dst_rect_red_box.w + 4;
					dst_rect_red_box.h = dst_rect_red_box.h - 17;
					dst_rect_red_box.x = dst_rect_red_box.x - 4;
					dst_rect_red_box.y = dst_rect_red_box.y + 14;

//					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
					SDL_SetRenderDrawColor(g_mainRenderer, 255, 0, 0, 255);
					//				SDL_RenderFillRect(renderer, &dst_rect);
					SDL_RenderFillRect(g_mainRenderer, &dst_rect_red_box);
					
					
					SDL_RenderCopy(g_mainRenderer, g_textTextureData.newHighScoreLabelTextTexture, NULL, &dst_rect);
					}
				}
			}
			
			{
#define PLATINUM_MEDAL_MINUIMUM 40
#define GOLD_MEDAL_MINUIMUM 30
#define SILVER_MEDAL_MINUIMUM 20
#define BRONZE_MEDAL_MINUIMUM 10
				
				SDL_Texture* which_medal = NULL;
				
				if(g_gameInstanceData.currentScore >= PLATINUM_MEDAL_MINUIMUM)
				{
					which_medal = g_gameTextures.platinumMedal;
				}
				else if(g_gameInstanceData.currentScore >= GOLD_MEDAL_MINUIMUM)
				{
					which_medal = g_gameTextures.goldMedal;
				}
				else if(g_gameInstanceData.currentScore >= SILVER_MEDAL_MINUIMUM)
				{
					which_medal = g_gameTextures.silverMedal;
				}
				else if(g_gameInstanceData.currentScore >= BRONZE_MEDAL_MINUIMUM)
				{
					which_medal = g_gameTextures.bronzeMedal;
				}
				else
				{
					which_medal = g_gameTextures.placeholderMedal;
				}
				SDL_QueryTexture(which_medal, &the_format, &the_access, &the_width, &the_height);
				
				//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
				SDL_Rect dst_rect = {0 + 36, TOP_MARGIN + 10, the_width, the_height};
				dst_rect.w = 150;
				dst_rect.h = 208;
				
				SDL_RenderCopy(g_mainRenderer, which_medal, NULL, &dst_rect);
			}
			
			{
				/* Max score in Flappy Bird is 9999 */
#define MAX_SCORE_STRING_LENGTH 5
				char score_string[MAX_SCORE_STRING_LENGTH];
				SDL_Color white_text_color = { 255, 255, 255, 0 };
				SDL_Color black_text_color = { 0, 0, 0, 0 };
				SDL_Surface* text_surface = NULL;
				MyFloat linear_interp;
				int displayed_score;
				
				if(g_gameState < GAMESTATE_TALLYING_SCORE)
				{
					displayed_score = 0;
				}
				else if(g_gameState == GAMESTATE_TALLYING_SCORE)
				{
					linear_interp = Flappy_ComputeLinearInterpolation(current_time, g_gameStateCurrentPhaseStartTime, g_gameStateCurrentPhaseStartTime + TALLY_DURATION);
					displayed_score = Flappy_lroundf(linear_interp * (MyFloat)g_gameInstanceData.currentScore);
				}
				else
				{
					displayed_score = g_gameInstanceData.currentScore;
				}
				
			// Note: Could optimize to not recreate texture when there is no change.
				
				
				if(NULL != g_textTextureData.scoreNumberTextTexture)
				{
//					SDL_DestroyTexture(g_textTextureData.scoreNumberTextShadowTexture);
					SDL_DestroyTexture(g_textTextureData.scoreNumberTextTexture);
				}
				


//				SDL_Log("linear_interp %f, current_time=%d, diedAtTime=%d", linear_interp, current_time, g_gameInstanceData.diedAtTime);

				
				
				SDL_snprintf(score_string, MAX_SCORE_STRING_LENGTH, "%d", displayed_score);
				text_surface = TTF_RenderText_Blended(g_textTextureData.fontForScoreNumbers, score_string, white_text_color);
				g_textTextureData.scoreNumberTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
				//	SDL_Log("text surface w,h: %d, %d", text_surface->w, text_surface->h);

//				SDL_Log("score_string %s\n", score_string);
				
				
				SDL_FreeSurface(text_surface);
			
				
				
				
				SDL_QueryTexture(g_textTextureData.scoreNumberTextTexture, &the_format, &the_access, &the_width, &the_height);
				
				//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
				SDL_SetTextureColorMod(g_textTextureData.scoreNumberTextTexture, 0, 0, 0);
				SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, TOP_MARGIN + 40, the_width, the_height};
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreNumberTextTexture, NULL, &dst_rect);
				dst_rect.x = dst_rect.x - 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreNumberTextTexture, NULL, &dst_rect);

				dst_rect.y = dst_rect.y - 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreNumberTextTexture, NULL, &dst_rect);

				dst_rect.x = dst_rect.x + 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreNumberTextTexture, NULL, &dst_rect);

				dst_rect.x = dst_rect.x - 2;
				dst_rect.y = dst_rect.y + 2;
				SDL_SetTextureColorMod(g_textTextureData.scoreNumberTextTexture, 255, 255, 255);
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.scoreNumberTextTexture, NULL, &dst_rect);

			}

		
			
			
			{
				/* Max score in Flappy Bird is 9999 */
#define MAX_SCORE_STRING_LENGTH 5
				char score_string[MAX_SCORE_STRING_LENGTH];
				SDL_Color white_text_color = { 255, 255, 255, 0 };
				SDL_Color black_text_color = { 0, 0, 0, 0 };
				SDL_Surface* text_surface = NULL;
				MyFloat linear_interp;
				int displayed_score;
				
				if(NULL != g_textTextureData.bestNumberTextTexture)
				{
					//					SDL_DestroyTexture(g_textTextureData.scoreNumberTextShadowTexture);
					SDL_DestroyTexture(g_textTextureData.bestNumberTextTexture);
				}
				
				if( ((g_gameState >= GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION) && (g_gameState <= GAMESTATE_FADE_OUT_GAME_OVER) )
				   && (g_gameInstanceData.currentScore > g_highScoreData.previousHighScore)
				   )
				{
					displayed_score = g_gameInstanceData.currentScore;
				}
				else
				{
					displayed_score = g_highScoreData.previousHighScore;
				}

				
				SDL_snprintf(score_string, MAX_SCORE_STRING_LENGTH, "%d", displayed_score);
				text_surface = TTF_RenderText_Blended(g_textTextureData.fontForScoreNumbers, score_string, white_text_color);
				g_textTextureData.bestNumberTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
				//	SDL_Log("text surface w,h: %d, %d", text_surface->w, text_surface->h);
				
				//				SDL_Log("score_string %s\n", score_string);
				
				
				SDL_FreeSurface(text_surface);
				
				
				
				
				SDL_QueryTexture(g_textTextureData.bestNumberTextTexture, &the_format, &the_access, &the_width, &the_height);
				
				//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
				SDL_SetTextureColorMod(g_textTextureData.bestNumberTextTexture, 0, 0, 0);
				SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, TOP_MARGIN + 40 + 116, the_width, the_height};
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestNumberTextTexture, NULL, &dst_rect);
				dst_rect.x = dst_rect.x - 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestNumberTextTexture, NULL, &dst_rect);
				
				dst_rect.y = dst_rect.y - 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestNumberTextTexture, NULL, &dst_rect);
				
				dst_rect.x = dst_rect.x + 4;
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestNumberTextTexture, NULL, &dst_rect);
				
				dst_rect.x = dst_rect.x - 2;
				dst_rect.y = dst_rect.y + 2;
				SDL_SetTextureColorMod(g_textTextureData.bestNumberTextTexture, 255, 255, 255);
				SDL_RenderCopy(g_mainRenderer, g_textTextureData.bestNumberTextTexture, NULL, &dst_rect);
				
			}

		
		}
		
		/* We'll do the actual render in the render function */
		
/*
		//Render red filled quad
		SDL_Rect fillRect = { SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
		SDL_SetRenderDrawColor(g_mainRenderer, 0xFF, 0x00, 0x00, 0xFF );
		SDL_RenderFillRect(g_mainRenderer, &fillRect );
		
		//Render green outlined quad
		SDL_Rect outlineRect = { SCREEN_WIDTH / 6, SCREEN_HEIGHT / 6, SCREEN_WIDTH  / 3, SCREEN_HEIGHT  / 3 };
		SDL_SetRenderDrawColor(g_mainRenderer, 0x00, 0xFF, 0x00, 0xFF );
		SDL_RenderDrawRect(g_mainRenderer, &outlineRect );
		
		//Draw blue horizontal line
		SDL_SetRenderDrawColor(g_mainRenderer, 0x00, 0x00, 0xFF, 0xFF );
		SDL_RenderDrawLine(g_mainRenderer, 0, SCREEN_HEIGHT / 8, SCREEN_WIDTH, SCREEN_HEIGHT / 8 );
		
		//Draw vertical line of yellow dots
		SDL_SetRenderDrawColor(g_mainRenderer, 0xFF, 0xFF, 0x00, 0xFF );
		for( int i = 0; i < SCREEN_HEIGHT/8; i += 1 )
		{
			SDL_RenderDrawPoint(g_mainRenderer, SCREEN_WIDTH / 8, i );
		}
 */
		
		//Reset render target
		SDL_SetRenderTarget(g_mainRenderer, NULL );

	}
}

void Flappy_InitGuiButtons()
{
    Uint32 the_format;
    int the_access;
    int the_width;
    int the_height;
    SDL_Rect dst_rect;

    SDL_QueryTexture(g_gameTextures.playButton, &the_format, &the_access, &the_width, &the_height);

    
    dst_rect.w = the_width;
    dst_rect.h = the_height;
    
    /* On consoles like SteamOS, we must have an explicit Quit option or the user will not be able
     	to quit the game. (Tricks like Alt-F4 don't work unlessed explictly handled by the game.)
     	But on iOS, we can't have a Quit button or we'll be rejeted.
     	Android by convention doesn't display a Quit button either since the Exit button is available.
     	Console-ized Androids have the Exit/Back button built into the controllers.
     	So we will conditionalize the code depending on the platform.
     	Desktops show both buttons.
     	Mobiles will not.
     */
#if FLAPPY_PROVIDE_QUIT_BUTTON
    dst_rect.x = 2*(SCREEN_WIDTH / 5) - (the_width / 2);
#else
    dst_rect.x = (SCREEN_WIDTH / 2) - (the_width / 2);
#endif
    dst_rect.y = SCREEN_HEIGHT - 2 * (SCREEN_HEIGHT / 7);
    
    SDL_assert(g_gameTextures.playButton != NULL);
    g_guiPlayButton.currentTexture = g_gameTextures.playButton;
    g_guiPlayButton.isPressed = SDL_FALSE;
    g_guiPlayButton.isSelected = SDL_TRUE;
    g_guiPlayButton.dstRect = dst_rect;
    
    
    
#if FLAPPY_PROVIDE_QUIT_BUTTON
    
    SDL_QueryTexture(g_gameTextures.quitButton, &the_format, &the_access, &the_width, &the_height);
    
    dst_rect.w = the_width;
    dst_rect.h = the_height;
    
    dst_rect.x = 3*(SCREEN_WIDTH / 5) - (the_width / 2);
    dst_rect.y = SCREEN_HEIGHT - 2 * (SCREEN_HEIGHT / 7);
    
    
    SDL_assert(g_gameTextures.quitButton != NULL);
    g_guiQuitButton.currentTexture = g_gameTextures.quitButton;
    g_guiQuitButton.isPressed = SDL_FALSE;
    g_guiQuitButton.isSelected = SDL_FALSE;
    g_guiQuitButton.dstRect = dst_rect;
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
    
}

void Flappy_UpdateScoreTexture(Uint32 new_score)
{
	/* Max score in Flappy Bird is 9999 */
#define MAX_SCORE_STRING_LENGTH 5
	char score_string[MAX_SCORE_STRING_LENGTH];
	SDL_Color white_text_color = { 255, 255, 255, 0 };
	SDL_Color black_text_color = { 0, 0, 0, 0 };
	SDL_Surface* text_surface = NULL;
	
	SDL_DestroyTexture(g_currentScoreTextTexture);
	SDL_snprintf(score_string, MAX_SCORE_STRING_LENGTH, "%d", new_score);
	text_surface = TTF_RenderText_Blended(s_retroFontForMainScore, score_string, white_text_color);
	g_currentScoreTextTexture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
	
	//	SDL_Log("text surface w,h: %d, %d", text_surface->w, text_surface->h);
	
	
	SDL_FreeSurface(text_surface);
	

	
	/* We'll do the actual render in the render function */
	
}

void InitNewGame()
{
	Sint32 i;
	
	g_gameInstanceData.currentScore = 0;
	Flappy_UpdateScoreTexture(g_gameInstanceData.currentScore);
	
	g_highScoreData.previousHighScore = g_highScoreData.savedHighScore;
	g_highScoreData.currentHighScore = 0;
	
	
	g_gameInstanceData.distanceTraveled = 0;
	
	//	g_gameState = GAMESTATE_BIRD_PRELAUNCH;
	g_gameState = GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN;
	
	g_birdModelData.isPrelaunch = SDL_TRUE;
	g_birdModelData.isReadyForPipe = SDL_FALSE;
	g_birdModelData.birdLaunchStartTime = 0;
	
	g_birdModelData.isDead = SDL_FALSE;
	g_birdModelData.isGameOver = SDL_FALSE;
	g_birdModelData.isFalling = SDL_FALSE;
	g_birdModelData.isReadyToPlayFallingSound = SDL_FALSE;
	
	// because we change this for the title screen
	g_birdModelData.position.x = SCREEN_WIDTH / 3;
	g_birdModelData.position.y = SCREEN_HEIGHT / 2;
	
	if(cpSpaceContainsBody(g_mainSpace, g_birdModelData.birdBody))
	{
		cpSpaceRemoveBody(g_mainSpace, g_birdModelData.birdBody);
		cpSpaceRemoveShape(g_mainSpace, g_birdModelData.birdShape);
		
	}
	
	cpBodySetVelocity(g_birdModelData.birdBody, cpv(0, 0));
	cpBodySetPosition(g_groundPhysicsData.body, cpv(0 + SCREEN_WIDTH/2, GROUND_HEIGHT/2.0));
	cpBodySetAngularVelocity(g_birdModelData.birdBody, 0);
	cpBodySetAngle(g_birdModelData.birdBody, 0);
	
	
	
	for(i=0; i<CircularQueueVoid_Size(g_circularQueueOfPipes); i++)
	{
		struct PipeModelData* current_pipe = (struct PipeModelData*)CircularQueueVoid_ValueAtIndex(g_circularQueueOfPipes, i);
		cpSpaceRemoveShape(g_mainSpace, current_pipe->scoreSensorPhysicsShape);
		cpSpaceRemoveShape(g_mainSpace, current_pipe->lowerPhysicsShape);
		cpSpaceRemoveShape(g_mainSpace, current_pipe->upperPhysicsShape);
		
		CircularQueueVoid_PushBack(g_queueOfAvailablePipes, current_pipe);
	}
	
	while(CircularQueueVoid_Size(g_circularQueueOfPipes)>0)
	{
		CircularQueueVoid_PopFront(g_circularQueueOfPipes);
	}
	
	
	
	
}

void InitTitleScreen()
{
	InitNewGame();
	g_gameState = GAMESTATE_TITLE_SCREEN;
	g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
	g_birdModelData.position.x = SCREEN_WIDTH / 2;
	g_birdModelData.position.y = SCREEN_HEIGHT / 2 + 20;

	
}

void Flappy_UpdateGameState(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	switch(g_gameState)
	{
		case GAMESTATE_FADE_OUT_TITLE_SCREEN:
		{
			if(current_time > (g_gameStateCurrentPhaseStartTime + FADE_OUT_TIME))
			{
				InitNewGame();
				g_gameState = GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN;
				g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
			}
			break;
		}
		case GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN:
		{
			if(current_time > (g_gameStateCurrentPhaseStartTime + FADE_IN_TIME))
			{

				g_gameState = GAMESTATE_BIRD_PRELAUNCH;
				g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
			}
			break;
		}
		
        case GAMESTATE_BIRD_LAUNCHED_BUT_BEFORE_PIPES:
        {
            if(current_time >= g_gameStateCurrentPhaseStartTime + PIPE_START_TIME_DELAY)
            {
                g_gameState = GAMESTATE_MAIN_GAME_ACTIVE;
                g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
            }
            break;
        }


            
		case GAMESTATE_DEAD_TO_GAME_OVER_TRANSITION:
		{
#define DELAY_UNTIL_GAME_OVER_SWOOP_START 500
			if(current_time >= g_gameStateCurrentPhaseStartTime + DELAY_UNTIL_GAME_OVER_SWOOP_START)
			{
				g_gameState = GAMESTATE_SWOOPING_IN_GAME_OVER;
				g_gameOverDisplayData.sweepInStartTime = TimeTicker_GetTime(g_gameClock);
				g_gameStateCurrentPhaseStartTime = g_gameOverDisplayData.sweepInStartTime;

				ALmixer_PlayChannel(-1, g_gameSounds.swoosh, 0);
			}
			break;
		}
			
		case GAMESTATE_TALLYING_SCORE:
		{
			if(current_time > (g_gameStateCurrentPhaseStartTime + TALLY_DURATION))
			{
				g_gameState = GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION;
				g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
//				Flappy_InitGuiButtons();
			}
			break;
		}
			
		case GAMESTATE_FADE_OUT_GAME_OVER:
		{
			if(current_time > (g_gameStateCurrentPhaseStartTime + FADE_OUT_TIME))
			{
				InitNewGame();

				g_gameState = GAMESTATE_LOADING_NEW_GAME_AND_FADE_IN;
				g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
			}
			break;
		}
			
		default:
		{
			
		}
	}
}


void Flappy_Update(Uint32 delta_time, Uint32 base_time, Uint32 current_time)
{
	Flappy_StepPhysics(delta_time);
	
	/* This is either based on time, or the place it crashed */
	Sint32 current_bird_time_position = Flappy_DetermineCurrentBirdTimePosition(delta_time, base_time, current_time);
//	SDL_Log("current_bird_time_position:%d, current_time:%d\n", current_bird_time_position, current_time);
	
	Flappy_UpdateGameState(delta_time, base_time, current_time);

	
	Flappy_UpdateCloudPositions(delta_time, base_time, current_bird_time_position);
	Flappy_UpdateGroundPosition(delta_time, base_time, current_bird_time_position);
	Flappy_UpdateBushPositions(delta_time, base_time, current_bird_time_position);
	Flappy_UpdateBirdPosition(delta_time, base_time, current_time);
	
	Flappy_UpdatePipePositions(delta_time, base_time, current_bird_time_position);
	
	
	Flappy_UpdateGameOverDisplayData(delta_time, base_time, current_time);

	Flappy_UpdateMedalBackgroundData(delta_time, base_time, current_time);
	

}

void Flappy_LoadSounds()
{
	ALmixer_Data* audio_data;
	
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	char base_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(base_path, MAX_FILE_STRING_LENGTH);
	
	/* load the sound */
	/*
		Eric Wing
		Creative Commons Attribution 3.0
	 */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "coin_ding.wav", MAX_FILE_STRING_LENGTH);
//	SDL_Log("loading %s\n", resource_file_path);

	audio_data = ALmixer_LoadAll(resource_file_path, AL_FALSE);
	if(NULL == audio_data)
	{
		SDL_Log("could not load %s: %s\n", resource_file_path, SDL_GetError());

		fatalError("could not load sound");
	}
	g_gameSounds.coin = audio_data;
	
	/* load the sound */
	/*
	 Paul Rodenburg
	 I grant anyone the right to use this sound effect for any purpose commercial or non-commercial world wide for perpetuity, so long as it's not used in a stand alone manner.
	 https://www.youtube.com/watch?v=IsJnuvbjL0s
	 */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "SlideWhistle.wav", MAX_FILE_STRING_LENGTH);
//	SDL_Log("loading %s\n", resource_file_path);
	audio_data = ALmixer_LoadAll(resource_file_path, AL_FALSE);
	if(NULL == audio_data)
	{
		SDL_Log("could not load %s: %s\n", resource_file_path, SDL_GetError());
		fatalError("could not load sound");
	}
	g_gameSounds.fall = audio_data;

	/* load the sound */
	/* 
		Dave Des
		Creative Commons 0 1.0 Universal (CC0 1.0)
		http://www.freesound.org/people/dave.des/sounds/127197/

	 */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "davedes_fastsimplechop5b.wav", MAX_FILE_STRING_LENGTH);
//	SDL_Log("loading %s\n", resource_file_path);
	audio_data = ALmixer_LoadAll(resource_file_path, AL_FALSE);
	if(NULL == audio_data)
	{
		SDL_Log("could not load %s: %s\n", resource_file_path, SDL_GetError());
		fatalError("could not load sound");
	}
	g_gameSounds.flap = audio_data;
	
	
	/* load the sound */
	/*
		Mike Koenig
		Creative Commons Attribution 3.0
		http://soundbible.com/991-Left-Hook.html
	 */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "LeftHook_SoundBible_com-516660386.wav", MAX_FILE_STRING_LENGTH);
//	SDL_Log("loading %s\n", resource_file_path);
	audio_data = ALmixer_LoadAll(resource_file_path, AL_FALSE);
	if(NULL == audio_data)
	{
		SDL_Log("could not load %s: %s\n", resource_file_path, SDL_GetError());
		fatalError("could not load sound");
	}
	g_gameSounds.crash = audio_data;

	

	/* load the sound */
	/*
		man
		Creative Commons Attribution 3.0
		http://www.freesound.org/people/man/sounds/14609/
	 */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "14609__man__swosh.wav", MAX_FILE_STRING_LENGTH);
	audio_data = ALmixer_LoadAll(resource_file_path, AL_FALSE);
	if(NULL == audio_data)
	{
		SDL_Log("could not load %s: %s\n", resource_file_path, SDL_GetError());
		fatalError("could not load sound");
	}
	g_gameSounds.swoosh = audio_data;

	
}

void Flappy_DoFlap()
{
	if(g_gameInstanceData.isPaused)
	{
		return;
	}
	
	
	if(g_birdModelData.isPrelaunch)
	{
		g_birdModelData.isPrelaunch = SDL_FALSE;
		cpSpaceAddBody(g_mainSpace, g_birdModelData.birdBody);
		cpSpaceAddShape(g_mainSpace, g_birdModelData.birdShape);
		g_birdModelData.birdLaunchStartTime = TimeTicker_GetTime(g_gameClock);
        g_gameStateCurrentPhaseStartTime = g_birdModelData.birdLaunchStartTime;
        g_gameState = GAMESTATE_BIRD_LAUNCHED_BUT_BEFORE_PIPES;
        
        
	}
	if(g_birdModelData.isDead)
	{
		return;
	}
	

	
	/* In Flappy Bird, when the bird is above the screen,
		there is some kind of limit imposed so you can't fly up really high.
		This can be recreated easily by not allowing flaps when the bird is offscreen.
		We add the size because SDL places y at the top of the sprite, not the bottom.
		We add a fudge factor because Flappy Bird seems to give some extra space at the top.
	 */
	if(g_birdModelData.position.y + g_birdModelData.size.y + g_birdModelData.size.y >= 0)
	{
		//	cpBodyApplyImpulse(g_birdModelData.birdBody, cpv(0, 10), cpv(0,0));
		// We only want to use velocity for the y-axis because we are already manually computing x-axis movement.
		ALmixer_PlayChannel(-1, g_gameSounds.flap, 0);
//		cpBodySetVelocity((g_birdModelData.birdBody, cpv(g_birdModelData.velocity.x, 460));
//		cpBodySetVelocity((g_birdModelData.birdBody, cpv(g_birdModelData.velocity.x, 460));
		cpBodySetVelocity(g_birdModelData.birdBody, cpv(g_birdModelData.velocity.x, 580));
		cpBodySetAngle(g_birdModelData.birdBody, M_PI_4);
		cpBodySetAngularVelocity(g_birdModelData.birdBody, -2);
		/* Chipmunk API says to reindex after changing the angle */
		cpSpaceReindexShapesForBody(g_mainSpace, g_birdModelData.birdBody);
	}

}


/*
 loads the happyface graphic into a texture
 */
void
initializeTexture(SDL_Renderer* the_renderer)
{
	SDL_Surface* bmp_surface;
	SDL_Texture* the_texture;
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	char base_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(base_path, MAX_FILE_STRING_LENGTH);
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "background.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	//	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	g_gameTextures.background = the_texture;
	
	

	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "clouds.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	Flappy_InitializeCloudModelData(the_texture);
	g_gameTextures.clouds = the_texture;
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	/* fly0 is the wing in the neutral position */
	SDL_strlcat(resource_file_path, "fly0.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.bird[2] = the_texture;
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	/* fly1 is the wing in the neutral position */
	SDL_strlcat(resource_file_path, "fly1.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.bird[1] = the_texture;
	
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	/* fly2 is the wing in the up position */
	SDL_strlcat(resource_file_path, "fly2.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.bird[0] = the_texture;
	
	// Assumes all the bird textures are the sime width/height so any of the 3 may be passed in
	Flappy_InitializeBirdModelData(g_gameTextures.bird[1]);
	
	

	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "ground.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
//	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	Flappy_InitializeGroundModelData(the_texture);
	g_gameTextures.ground = the_texture;
	
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "bush.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	Flappy_InitializeBushModelData(the_texture);
	g_gameTextures.bush = the_texture;

	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "pipe_bottom.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.pipe_bottom = the_texture;
	
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "pipe_top.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.pipe_top = the_texture;
	
	// Intializes both the top and bottom. Assumes the pipe textures are the sime width/height so either may be passed in
	Flappy_InitializePipeModelData(g_gameTextures.pipe_bottom);
	
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "MedalBackground.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.medalBackground = the_texture;
	
	Flappy_InitializeMedalBackgroundData(the_texture);
	
	
	/* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "momoko_Bronze_Medallion.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.bronzeMedal = the_texture;
	
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "momoko_Silver_Medallion.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.silverMedal = the_texture;
	
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "momoko_Gold_Medallion.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.goldMedal = the_texture;
	
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "momoko_Platinum_Medallion.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.platinumMedal = the_texture;
	
	
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "Placeholder_Medallion.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.placeholderMedal = the_texture;
	
//	Flappy_InitializeMedalBackgroundData(the_texture);
	
    
    
    
    /* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "playbutton.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.playButton = the_texture;
	
    /* load the image */
	SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "quitbutton.png", MAX_FILE_STRING_LENGTH);
	the_texture = IMG_LoadTexture(the_renderer, resource_file_path);
	if(NULL == the_texture)
	{
		fatalError("could not create texture");
	}
	SDL_SetTextureBlendMode(the_texture, SDL_BLENDMODE_BLEND);
	//	SDL_SetTextureAlphaMod(the_texture, .6 * 255);
	g_gameTextures.quitButton = the_texture;
    
    Flappy_InitGuiButtons();
}



SDL_Texture* Flappy_CreateGameOverTexture()
{
	SDL_Color white_text_color = { 255, 255, 255, 0 };
//	SDL_Color green_text_color = { 0, 0, 0, 0 };
	SDL_Color black_text_color = { 0, 0, 0, 0 };
	SDL_Color orange_text_color = { 220, 120, 0, 255 };

	SDL_Surface* text_surface = NULL;
	TTF_Font* the_font = NULL;
	SDL_Texture* base_texture;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	

	
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
//	SDL_strlcat(resource_file_path, "acknowtt.ttf", MAX_FILE_STRING_LENGTH);
//	SDL_strlcat(resource_file_path, "DisposableDroidBB.otf", MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "04B_19__.TTF", MAX_FILE_STRING_LENGTH);
	
	
    the_font = TTF_OpenFont(resource_file_path, 110);
    if(NULL == the_font)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return NULL;
    }
	
		
	
	
	text_surface = TTF_RenderText_Blended(the_font, "Game Over", white_text_color);
	base_texture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);

	SDL_QueryTexture(base_texture, &the_format, &the_access, &the_width, &the_height);

	
	g_textTextureData.gameOverTextTexture = SDL_CreateTexture(g_mainRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, the_width+20, the_height+20);
	SDL_assert(NULL != g_textTextureData.gameOverTextTexture);
	SDL_SetTextureBlendMode(g_textTextureData.gameOverTextTexture, SDL_BLENDMODE_BLEND);

	
	
	
	SDL_SetRenderTarget(g_mainRenderer, g_textTextureData.gameOverTextTexture);
	
	// Clear screen
	SDL_SetRenderDrawColor(g_mainRenderer, 0, 0, 0, 0 );
	SDL_RenderClear(g_mainRenderer );

	
	{
		SDL_Rect dst_rect_center = {10, 10, the_width, the_height};
		SDL_Rect dst_rect;
	

	//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
	SDL_SetTextureColorMod(base_texture, 0, 0, 0);
	dst_rect = dst_rect_center;

	// left-top
	dst_rect.x = dst_rect.x - 10;
	dst_rect.y = dst_rect.y - 10;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);

	// right-top
	dst_rect.x = dst_rect.x + 20;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	// right-bottom
	dst_rect.y = dst_rect.y + 20;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);

	// left-bottom
	dst_rect.x = dst_rect.x - 20;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);

	// recenter
	dst_rect = dst_rect_center;
	// left
	dst_rect.x = dst_rect.x - 10;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	// right
	dst_rect.x = dst_rect.x + 20;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);

	// recenter
	dst_rect = dst_rect_center;

	// bottom
	dst_rect.y = dst_rect.y - 10;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
	// top
	dst_rect.y = dst_rect.y + 20;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
	// recenter
	dst_rect = dst_rect_center;

	
	SDL_SetTextureColorMod(base_texture, 255, 255, 255);
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);

	
	
	
	dst_rect.x = dst_rect.x - 6;
	dst_rect.y = dst_rect.y - 6;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	dst_rect.x = dst_rect.x + 12;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	dst_rect.y = dst_rect.y + 12;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	dst_rect.x = dst_rect.x - 12;
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
	
	dst_rect = dst_rect_center;

	SDL_SetTextureColorMod(base_texture, 220, 120, 0);
	SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
	
	}
		
	
	SDL_SetRenderTarget(g_mainRenderer, NULL);

	
	SDL_DestroyTexture(base_texture);
	SDL_FreeSurface(text_surface);

	TTF_CloseFont(the_font);
	
	return g_textTextureData.gameOverTextTexture;
	
}


void Flappy_CreateGetReadyTexture()
{
	SDL_Color white_text_color = { 255, 255, 255, 0 };
	SDL_Color black_text_color = { 0, 0, 0, 0 };
	SDL_Color green_text_color = { 64, 128, 0, 255 };
	
	SDL_Surface* text_surface = NULL;
	TTF_Font* the_font = NULL;
	SDL_Texture* base_texture;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	
	
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
//	SDL_strlcat(resource_file_path, "acknowtt.ttf", MAX_FILE_STRING_LENGTH);
    SDL_strlcat(resource_file_path, "04B_19__.TTF", MAX_FILE_STRING_LENGTH);

	
    the_font = TTF_OpenFont(resource_file_path, 90);
    if(NULL == the_font)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return;
    }
	
	
	
	
	text_surface = TTF_RenderText_Blended(the_font, "Get Ready!", white_text_color);
	base_texture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
	
	SDL_QueryTexture(base_texture, &the_format, &the_access, &the_width, &the_height);
	
	
	g_textTextureData.getReadyTextTexture = SDL_CreateTexture(g_mainRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, the_width+20, the_height+20);
	SDL_assert(NULL != g_textTextureData.getReadyTextTexture);
	SDL_SetTextureBlendMode(g_textTextureData.getReadyTextTexture, SDL_BLENDMODE_BLEND);
	
	
	
	
	SDL_SetRenderTarget(g_mainRenderer, g_textTextureData.getReadyTextTexture);
	
	// Clear screen
	SDL_SetRenderDrawColor(g_mainRenderer, 0, 0, 0, 0 );
	SDL_RenderClear(g_mainRenderer );
	
	
	{
		SDL_Rect dst_rect_center = {10, 10, the_width, the_height};
		SDL_Rect dst_rect;
		
		
		//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		dst_rect = dst_rect_center;
		
		// left-top
		dst_rect.x = dst_rect.x - 10;
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-top
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-bottom
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// left-bottom
		dst_rect.x = dst_rect.x - 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		// recenter
		dst_rect = dst_rect_center;
		
		
		SDL_SetTextureColorMod(base_texture, 255, 255, 255);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		
		
		dst_rect.x = dst_rect.x - 6;
		dst_rect.y = dst_rect.y - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.y = dst_rect.y + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x - 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		
		dst_rect = dst_rect_center;
		
		SDL_SetTextureColorMod(base_texture, 64, 128, 0);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
	}
	
	
	SDL_SetRenderTarget(g_mainRenderer, NULL);
	
	
	SDL_DestroyTexture(base_texture);
	SDL_FreeSurface(text_surface);
	
	TTF_CloseFont(the_font);
	
	
}



void Flappy_CreateFlappyBlurrrTexture()
{
	SDL_Color white_text_color = { 255, 255, 255, 0 };
	SDL_Color black_text_color = { 0, 0, 0, 0 };
	SDL_Color green_text_color = { 64, 128, 0, 255 };
	
	SDL_Surface* text_surface = NULL;
	TTF_Font* the_font = NULL;
	SDL_Texture* base_texture;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	
	
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
	//	SDL_strlcat(resource_file_path, "acknowtt.ttf", MAX_FILE_STRING_LENGTH);
    SDL_strlcat(resource_file_path, "04B_19__.TTF", MAX_FILE_STRING_LENGTH);
	
	
    the_font = TTF_OpenFont(resource_file_path, 100);
    if(NULL == the_font)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return;
    }
	
	
	
	
	text_surface = TTF_RenderText_Blended(the_font, "FlappyBlurrr", white_text_color);
	base_texture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
	
	SDL_QueryTexture(base_texture, &the_format, &the_access, &the_width, &the_height);
	
	
	g_textTextureData.flappyBlurrrTexture = SDL_CreateTexture(g_mainRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, the_width+20, the_height+20);
	SDL_assert(NULL != g_textTextureData.flappyBlurrrTexture);
	SDL_SetTextureBlendMode(g_textTextureData.flappyBlurrrTexture, SDL_BLENDMODE_BLEND);
	
	
	
	
	SDL_SetRenderTarget(g_mainRenderer, g_textTextureData.flappyBlurrrTexture);
	
	// Clear screen
	SDL_SetRenderDrawColor(g_mainRenderer, 0, 0, 0, 0 );
	SDL_RenderClear(g_mainRenderer );
	
	
	{
		SDL_Rect dst_rect_center = {10, 10, the_width, the_height};
		SDL_Rect dst_rect;
		
#if 0
		//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		dst_rect = dst_rect_center;
		
		// left-top
		dst_rect.x = dst_rect.x - 10;
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-top
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-bottom
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// left-bottom
		dst_rect.x = dst_rect.x - 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		// recenter
		dst_rect = dst_rect_center;
		
		
		SDL_SetTextureColorMod(base_texture, 255, 255, 255);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
#endif
		
		
		dst_rect = dst_rect_center;
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		
//		dst_rect.x = dst_rect.x + 4;
		dst_rect.y = dst_rect.y + 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		dst_rect.x = dst_rect.x + 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x - 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		
		dst_rect = dst_rect_center;
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);

		dst_rect.x = dst_rect.x - 4;
		dst_rect.y = dst_rect.y - 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x + 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.y = dst_rect.y + 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x - 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
/*
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 6;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 12;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
*/
		
		
		dst_rect = dst_rect_center;
		
		SDL_SetTextureColorMod(base_texture, 255, 255, 255);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
	}
	
	
	SDL_SetRenderTarget(g_mainRenderer, NULL);
	
	
	SDL_DestroyTexture(base_texture);
	SDL_FreeSurface(text_surface);
	
	TTF_CloseFont(the_font);
	
	
}



void Flappy_CreateTaglineTexture()
{
	SDL_Color white_text_color = { 255, 255, 255, 0 };
	SDL_Color black_text_color = { 0, 0, 0, 0 };
	SDL_Color green_text_color = { 64, 128, 0, 255 };
	
	SDL_Surface* text_surface = NULL;
	TTF_Font* the_font = NULL;
	SDL_Texture* base_texture;
	
	Uint32 the_format;
	int the_access;
	int the_width;
	int the_height;
	
	
	
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];
	
	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
//	SDL_strlcat(resource_file_path, "acknowtt.ttf", MAX_FILE_STRING_LENGTH);
//	SDL_strlcat(resource_file_path, "VeraMono.ttf", MAX_FILE_STRING_LENGTH);
    SDL_strlcat(resource_file_path, "04B_19__.TTF", MAX_FILE_STRING_LENGTH);
	
	
    the_font = TTF_OpenFont(resource_file_path, 30);
    if(NULL == the_font)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return;
    }
	
	
	
	
	text_surface = TTF_RenderText_Blended_Wrapped(the_font, "A tutorial for making a meticulous\nFlappy Bird clone with Blurrr SDK\n\n        http://playcontrol.net", white_text_color, SCREEN_WIDTH/3 + 120);
	base_texture = SDL_CreateTextureFromSurface(g_mainRenderer, text_surface);
	
	SDL_QueryTexture(base_texture, &the_format, &the_access, &the_width, &the_height);
	
	
	g_textTextureData.taglineTexture = SDL_CreateTexture(g_mainRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, the_width+10, the_height+10);
	SDL_assert(NULL != g_textTextureData.taglineTexture);
	SDL_SetTextureBlendMode(g_textTextureData.taglineTexture, SDL_BLENDMODE_BLEND);
	
	
	
	
	SDL_SetRenderTarget(g_mainRenderer, g_textTextureData.taglineTexture);
	
	// Clear screen
	SDL_SetRenderDrawColor(g_mainRenderer, 0, 0, 0, 0 );
	SDL_RenderClear(g_mainRenderer );
	
	
	{
		SDL_Rect dst_rect_center = {10, 10, the_width, the_height};
		SDL_Rect dst_rect;
		
#if 0
		//			SDL_Rect dst_rect = {g_medalBackgroundData.dstRect.w - the_width - RIGHT_MARGIN, 10, g_medalBackgroundData.dstRect.w, g_medalBackgroundData.dstRect.h};
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		dst_rect = dst_rect_center;
		
		// left-top
		dst_rect.x = dst_rect.x - 10;
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-top
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right-bottom
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// left-bottom
		dst_rect.x = dst_rect.x - 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		// left
		dst_rect.x = dst_rect.x - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// right
		dst_rect.x = dst_rect.x + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// recenter
		dst_rect = dst_rect_center;
		
		// bottom
		dst_rect.y = dst_rect.y - 10;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		// top
		dst_rect.y = dst_rect.y + 20;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		// recenter
		dst_rect = dst_rect_center;
		
		
		SDL_SetTextureColorMod(base_texture, 255, 255, 255);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
#endif
		
		/*
		dst_rect = dst_rect_center;
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		
		//		dst_rect.x = dst_rect.x + 4;
		dst_rect.y = dst_rect.y + 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		
		dst_rect.x = dst_rect.x + 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x - 8;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		*/
		
		dst_rect = dst_rect_center;
		SDL_SetTextureColorMod(base_texture, 0, 0, 0);
		
		dst_rect.x = dst_rect.x - 2;
		dst_rect.y = dst_rect.y - 2;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x + 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.y = dst_rect.y + 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		dst_rect.x = dst_rect.x - 4;
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
		/*
		 // recenter
		 dst_rect = dst_rect_center;
		 // left
		 dst_rect.x = dst_rect.x - 6;
		 SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		 
		 // right
		 dst_rect.x = dst_rect.x + 12;
		 SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		 
		 // recenter
		 dst_rect = dst_rect_center;
		 
		 // bottom
		 dst_rect.y = dst_rect.y - 6;
		 SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		 
		 // top
		 dst_rect.y = dst_rect.y + 12;
		 SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		 */
		
		
		dst_rect = dst_rect_center;
		
		SDL_SetTextureColorMod(base_texture, 255, 255, 255);
		SDL_RenderCopy(g_mainRenderer, base_texture, NULL, &dst_rect);
		
	}
	
	
	SDL_SetRenderTarget(g_mainRenderer, NULL);
	
	
	SDL_DestroyTexture(base_texture);
	SDL_FreeSurface(text_surface);
	
	TTF_CloseFont(the_font);
	
	
}




void initializeFont()
{
#define MAX_FILE_STRING_LENGTH 2048
	char resource_file_path[MAX_FILE_STRING_LENGTH];

	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "VeraMono.ttf", MAX_FILE_STRING_LENGTH);

    s_veraMonoFont = TTF_OpenFont(resource_file_path, 16);
    if(NULL == s_veraMonoFont)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return;
    }
	
	
	BlurrrPath_GetResourceDirectoryString(resource_file_path, MAX_FILE_STRING_LENGTH);
	SDL_strlcat(resource_file_path, "acknowtt.ttf", MAX_FILE_STRING_LENGTH);
	
    s_retroFontForMainScore = TTF_OpenFont(resource_file_path, 130);
    if(NULL == s_retroFontForMainScore)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
		return;
    }
	
	
	
	g_textTextureData.fontForMedalLabels = TTF_OpenFont(resource_file_path, 56);
    if(NULL == g_textTextureData.fontForMedalLabels)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
    }
	
	g_textTextureData.fontForScoreNumbers = TTF_OpenFont(resource_file_path, 76);
    if(NULL == g_textTextureData.fontForScoreNumbers)
	{
        SDL_Log("Couldn't load font %s: %s\n", resource_file_path, SDL_GetError());
    }
	
	
	Flappy_UpdateScoreTexture(0);
//	Flappy_CreateGameOverTexture();
	Flappy_InitializeGameOverDisplayData();
	Flappy_CreateGetReadyTexture();
	Flappy_CreateFlappyBlurrrTexture();
	Flappy_CreateTaglineTexture();
}



#if 0
static int report (lua_State *L, int status) {
	const char *msg;
	if (status) {
		msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error with no message)";
		SDL_Log("status=%d, %s\n", status, msg);
		lua_pop(L, 1);
	}
	return status;
}
#endif

void Flappy_SoundFinishedCallback(ALint which_channel, ALuint al_source, ALmixer_Data* almixer_data, ALboolean finished_naturally, void* user_data)
{
	if(almixer_data == g_gameSounds.crash)
	{
		if(g_birdModelData.isFalling)
		{
			g_birdModelData.isReadyToPlayFallingSound = SDL_TRUE;
		}
	}
}

cpBool Flappy_OnScorePointCallback(cpArbiter* the_arbiter, cpSpace* the_space, void* user_data)
{

	
	ALmixer_PlayChannel(-1, g_gameSounds.coin, 0);

	
	
	g_gameInstanceData.currentScore = g_gameInstanceData.currentScore + 1;
	Flappy_UpdateScoreTexture(g_gameInstanceData.currentScore);
	

	
	return cpFalse;

}


cpBool Flappy_OnPipeCollisionCallback(cpArbiter* the_arbiter, cpSpace* the_space, void* user_data)
{
	cpShape* a;
	cpShape* b;
	int i;
	
/*
	cpContactPointSet set = cpArbiterGetContactPointSet(the_arbiter);
	//	SDL_Log("set.count: %d", set.count);
	for(i=0; i<set.count; i++){
		// get and work with the collision point normal and penetration distance:
		//		SDL_Log("\np:%f %f",set.points[i].point.x,set.points[i].point.y);
		//printf("\nn:%f %f",set.points[i].normal.x,set.points[i].normal.y);
		//set.points[i].dist
	}
	
	cpArbiterGetShapes(the_arbiter, &a, &b);
	//	SDL_Log("\non player collision a,b %lx %lx", a, b);
	//	SDL_Log("on player collision s_playerFeet %lx", s_playerFeet);
*/
	/*
//	if( (b == g_groundPhysicsData.shape) || (a == g_groundPhysicsData.shape) )
	{
		SDL_Log("hit ground");
		
		ALmixer_PlayChannel(-1, g_gameSounds.crash, 0);
		//			cpFloat ground_height = (1024/16 * 1.9) * 768 / 1024;
		//		cpBodySetPosition(g_birdModelData.birdBody, cpv(g_birdModelData.birdBody->p.x, ground_height + g_groundPhysicsData.body->p.y + g_birdModelData.size.y/2));
		//		cpBodySetPosition(g_birdModelData.birdBody, cpv(g_birdModelData.birdBody->p.x, ground_height + g_birdModelData.size.y/2));
		// TODO: Fix graphics to cap penetration in draw, Chipmunk will correct the penetration by itself eventually
		
		return cpTrue;
	}
	*/
//	else
	
	// Don't let the bird smack the bottom pipe when falling from the top pipe
	if(!g_birdModelData.isDead)
	{
		g_gameInstanceData.diedAtTime = TimeTicker_GetTime(g_gameClock);
//		SDL_Log("hit pipe");
		g_birdModelData.isFalling = SDL_TRUE;
		g_birdModelData.isDead = SDL_TRUE;
		g_gameInstanceData.needsWhiteOut = SDL_TRUE;
		
		ALmixer_PlayChannel(-1, g_gameSounds.crash, 0);
		return cpFalse;
		
	}
	return cpFalse;
}

cpBool Flappy_OnGroundCollisionCallback(cpArbiter* the_arbiter, cpSpace* the_space, void* user_data)
{
	cpShape* a;
	cpShape* b;
	int i;
	
	/*
	cpContactPointSet set = cpArbiterGetContactPointSet(the_arbiter);
	//	SDL_Log("set.count: %d", set.count);
	for(i=0; i<set.count; i++){
		// get and work with the collision point normal and penetration distance:
		//		SDL_Log("\np:%f %f",set.points[i].point.x,set.points[i].point.y);
		//printf("\nn:%f %f",set.points[i].normal.x,set.points[i].normal.y);
		//set.points[i].dist
	}
	
	cpArbiterGetShapes(the_arbiter, &a, &b);
	//	SDL_Log("\non player collision a,b %lx %lx", a, b);
	//	SDL_Log("on player collision s_playerFeet %lx", s_playerFeet);
	*/
//	if( (b == g_groundPhysicsData.shape) || (a == g_groundPhysicsData.shape) )
	{

//		SDL_Log("hit ground");
		if(! g_birdModelData.isDead)
		{
			ALmixer_PlayChannel(-1, g_gameSounds.crash, 0);
			g_birdModelData.isDead = SDL_TRUE;
			g_gameInstanceData.needsWhiteOut = SDL_TRUE;
			g_gameInstanceData.diedAtTime = TimeTicker_GetTime(g_gameClock);
		}
		else
		{
//			g_birdModelData.isFalling = SDL_FALSE;
		}
		g_birdModelData.isGameOver = SDL_TRUE;

		g_gameState = GAMESTATE_DEAD_TO_GAME_OVER_TRANSITION;
		g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
//		g_medalBackgroundData.sweepInStartTime = TimeTicker_GetTime(g_gameClock);
//		g_gameOverDisplayData.sweepInStartTime = TimeTicker_GetTime(g_gameClock);
		
//		ALmixer_PlayChannel(-1, g_gameSounds.swoosh, 0);
//			cpFloat ground_height = (1024/16 * 1.9) * 768 / 1024;
//		cpBodySetPosition(g_birdModelData.birdBody, cpv(g_birdModelData.birdBody->p.x, ground_height + g_groundPhysicsData.body->p.y + g_birdModelData.size.y/2));
//		cpBodySetPosition(g_birdModelData.birdBody, cpv(g_birdModelData.birdBody->p.x, ground_height + g_birdModelData.size.y/2));
		// TODO: Fix graphics to cap penetration in draw, Chipmunk will correct the penetration by itself eventually
		
		return cpTrue;
	}
	/*
	else
	{
		SDL_Log("hit pipe");
//		g_birdModelData.isFalling = SDL_TRUE;
		ALmixer_PlayChannel(-1, g_gameSounds.crash, 0);
		return cpFalse;

	}
	 */
	return cpTrue;
}


void InitPhysics()
{
	cpFloat mass = 10.0f;
	cpFloat moment;
//	cpFloat ground_height = (1024/16 * 1.9) * 768 / 1024;

	
	g_mainSpace = cpSpaceNew();
	cpSpaceSetIterations(g_mainSpace, 30);
//	cpSpaceSetGravity(g_mainSpace, cpv(0, -1300));
	cpSpaceSetGravity(g_mainSpace, cpv(0, -1900));
//	cpSpaceSetGravity(g_mainSpace, cpv(0, -1500));
	//	cpSpaceSetGravity(s_mainSpace, cpv(0, -10));
	//	cpSpaceSetGravity(s_mainSpace, cpv(0, 0));
	cpSpaceSetSleepTimeThreshold(g_mainSpace, 0.5);
	cpSpaceSetCollisionSlop(g_mainSpace, 0.5);
	
	
	

	moment = cpMomentForBox(mass, g_birdModelData.size.x, g_birdModelData.size.y);
	g_birdModelData.birdBody = cpBodyNew(mass, moment);
//	cpSpaceAddBody(g_mainSpace, g_birdModelData.birdBody);
	
//	cpBodySetPosition(g_birdModelData.birdBody, cpv( ((g_birdModelData.position.x + g_birdModelData.size.x) / 2.0f),
	cpBodySetPosition(g_birdModelData.birdBody, cpv( g_birdModelData.position.x,
											   Flappy_InvertY(
															  Flappy_lroundf(g_birdModelData.position.y + (g_birdModelData.size.y / 2))
															  ))
				 );
	
	// We only want to use velocity for the y-axis because we are already manually computing x-axis movement.
	cpBodySetVelocity(g_birdModelData.birdBody, cpv(0, 0));

	
//	g_birdModelData.birdShape = cpBoxShapeNew(g_birdModelData.birdBody, g_birdModelData.size.x, g_birdModelData.size.y);
	g_birdModelData.birdShape = cpCircleShapeNew(g_birdModelData.birdBody, g_birdModelData.size.y/2, cpvzero);
	cpShapeSetCollisionType(g_birdModelData.birdShape, BIRD_COLLISION_TYPE);
//	cpSpaceAddShape(g_mainSpace, g_birdModelData.birdShape);
//	cpShapeSetElasticity(g_birdModelData.birdShape, 0.0);
//	cpShapeSetFriction(g_birdModelData.birdShape, 1000.0);
	
	
	//g_groundPhysicsData.body = cpBodyNew(INFINITY, INFINITY);
	g_groundPhysicsData.body = cpBodyNewStatic();
	cpBodySetPosition(g_groundPhysicsData.body, cpv(0 + SCREEN_WIDTH/2, GROUND_HEIGHT/2.0));
//	cpBodySetPosition(g_groundPhysicsData.body, cpv(0 + SCREEN_WIDTH/2, GROUND_HEIGHT));
	
//	cpBodySetVelocity((g_groundPhysicsData.body, cpv(BIRD_VELOCITY_X, 0));

	
	// size is somewhat arbitrary
	// measured 1.9 cm for ground on iPad mini for 1024 pixels
//	g_groundPhysicsData.shape = cpBoxShapeNew(g_groundPhysicsData.body, SCREEN_WIDTH, GROUND_HEIGHT);
	g_groundPhysicsData.shape = cpBoxShapeNew(g_groundPhysicsData.body, SCREEN_WIDTH, GROUND_HEIGHT, 0.0);
//	cpShapeSetElasticity(g_groundPhysicsData.shape, 0.0);
//	cpShapeSetFriction(g_groundPhysicsData.shape, 1000.0);
	cpShapeSetCollisionType(g_groundPhysicsData.shape, GROUND_COLLISION_TYPE);

	// don't add body to space to make it a "rogue" body (intended to be controlled manually). But still must add shape to space.
	cpSpaceAddShape(g_mainSpace, g_groundPhysicsData.shape);

	//cpShapeSetFriction(g_groundPhysicsData.shape, INFINITY);
//	cpShapeSetFriction(g_groundPhysicsData.shape, 100000);

	
//	collisionBird = new cpSpaceAddCollisionHandlerContainer();
//	cpSpaceAddCollisionHandler(space, 0, 0, startCollision, null, null, endCollision, collisionBird);
	
	{
		cpCollisionHandler* collision_handler = cpSpaceAddCollisionHandler(g_mainSpace, BIRD_COLLISION_TYPE, GROUND_COLLISION_TYPE);
		collision_handler->beginFunc = Flappy_OnGroundCollisionCallback;
	}
	{
		cpCollisionHandler* collision_handler = cpSpaceAddCollisionHandler(g_mainSpace, BIRD_COLLISION_TYPE, PIPE_COLLISION_TYPE);
		collision_handler->beginFunc = Flappy_OnPipeCollisionCallback;
	}
	{
		cpCollisionHandler* collision_handler = cpSpaceAddCollisionHandler(g_mainSpace, BIRD_COLLISION_TYPE, SCORE_SENSOR_TYPE);
		collision_handler->beginFunc = Flappy_OnScorePointCallback;
	}


}

void Flappy_SetGameTimeScale(MyFloat new_value)
{
	Sint32 channel;
	Sint32 total_channels = ALmixer_CountTotalChannels();
	TimeTicker_SetSpeed(g_gameClock, new_value);
	for(channel=0; channel<total_channels; channel++)
	{
		ALuint al_source = ALmixer_GetSource(channel);
		alSourcef(al_source, AL_PITCH, new_value);
	}
}

void Flappy_DoPrimaryAction()
{
    if((g_gameState >= GAMESTATE_BIRD_PRELAUNCH) && (g_gameState <= GAMESTATE_MAIN_GAME_ACTIVE))
    {
        Flappy_DoFlap();
    }
    else if(GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
    {
//        InitNewGame();
		ALmixer_PlayChannel(-1, g_gameSounds.swoosh, 0);
		g_gameState = GAMESTATE_FADE_OUT_GAME_OVER;
		g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
    }
	else if(GAMESTATE_TITLE_SCREEN == g_gameState)
	{
		ALmixer_PlayChannel(-1, g_gameSounds.swoosh, 0);
		g_gameState = GAMESTATE_FADE_OUT_TITLE_SCREEN;
		g_gameStateCurrentPhaseStartTime = TimeTicker_GetTime(g_gameClock);
	}
    /*
        
	if(g_birdModelData.isGameOver)
	{
		InitNewGame();
	}
	else
	{
	}
     */
}

void Flappy_DoMouseDown(SDL_MouseButtonEvent the_event)
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
	   || (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
    {
        SDL_Rect point_rect = { the_event.x, the_event.y, 1, 1 };
        if(SDL_HasIntersection(&point_rect, &g_guiPlayButton.dstRect))
        {
            g_guiPlayButton.isPressed = SDL_TRUE;
        }
#if FLAPPY_PROVIDE_QUIT_BUTTON
        else if(SDL_HasIntersection(&point_rect, &g_guiQuitButton.dstRect))
        {
            g_guiQuitButton.isPressed = SDL_TRUE;
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
    }
    else
    {
        Flappy_DoPrimaryAction();
    }
}

void Flappy_DoMouseMoved(SDL_MouseMotionEvent the_event)
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
	   || (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
	{
        SDL_Rect point_rect = { the_event.x, the_event.y, 1, 1 };
        if((SDL_TRUE == g_guiPlayButton.isPressed) && (SDL_FALSE == SDL_HasIntersection(&point_rect, &g_guiPlayButton.dstRect)))
        {
            g_guiPlayButton.isPressed = SDL_FALSE;
        }
#if FLAPPY_PROVIDE_QUIT_BUTTON
        else if((SDL_TRUE == g_guiQuitButton.isPressed) && (SDL_FALSE == SDL_HasIntersection(&point_rect, &g_guiQuitButton.dstRect)))
        {
            g_guiQuitButton.isPressed = SDL_FALSE;
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */

        // Allows for if the person is dragging the mouse-down, leaves the button and goes over a button,
        // the button will now press.
        else if((SDL_TRUE == the_event.state) && (SDL_TRUE == SDL_HasIntersection(&point_rect, &g_guiPlayButton.dstRect)))
        {
            g_guiPlayButton.isPressed = SDL_TRUE;
        }
#if FLAPPY_PROVIDE_QUIT_BUTTON
       else if((SDL_TRUE == the_event.state) && (SDL_TRUE == SDL_HasIntersection(&point_rect, &g_guiQuitButton.dstRect)))
        {
            g_guiQuitButton.isPressed = SDL_TRUE;
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
       
    }
    else
    {
    }
}

void Flappy_DoMouseUp(SDL_MouseButtonEvent the_event)
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
		|| (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
    {
        SDL_Rect point_rect = { the_event.x, the_event.y, 1, 1 };
        if(SDL_TRUE == g_guiPlayButton.isPressed)
        {
            g_guiPlayButton.isPressed = SDL_FALSE;
            Flappy_DoPrimaryAction();
        }
#if FLAPPY_PROVIDE_QUIT_BUTTON
        else if(SDL_TRUE == g_guiQuitButton.isPressed)
        {
            SDL_QuitEvent quit_event = { SDL_QUIT, SDL_GetTicks() };
            SDL_Event quit_event_union;
            quit_event_union.quit = quit_event;

            g_guiQuitButton.isPressed = SDL_FALSE;
            
            SDL_PushEvent(&quit_event_union);
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
   }
    else
    {
        // Don't do PrimaryAction because we only want to do it on mouse down
    }
}



void Flappy_DoLeftAction()
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
	   || (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
    {
#if FLAPPY_PROVIDE_QUIT_BUTTON
        if(SDL_TRUE == g_guiQuitButton.isSelected)
        {
            g_guiQuitButton.isSelected = SDL_FALSE;
            g_guiPlayButton.isSelected = SDL_TRUE;
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
    }
}

void Flappy_DoRightAction()
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
	   || (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
    {
#if FLAPPY_PROVIDE_QUIT_BUTTON
       if(SDL_TRUE == g_guiPlayButton.isSelected)
        {
            g_guiQuitButton.isSelected = SDL_TRUE;
            g_guiPlayButton.isSelected = SDL_FALSE;
        }
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
    }
}


void Flappy_DoReturnKeyDownAction()
{
	if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
	   || (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
	{
		if(SDL_TRUE == g_guiPlayButton.isSelected)
		{
			/* Warning: The mouse, gamepad, and keyboard will fight over this state if users want to break it */
			g_guiPlayButton.isPressed = SDL_TRUE;
			Flappy_DoPrimaryAction();
		}
#if FLAPPY_PROVIDE_QUIT_BUTTON
		else if(SDL_TRUE == g_guiQuitButton.isSelected)
		{
			SDL_QuitEvent quit_event = { SDL_QUIT, SDL_GetTicks() };
			SDL_Event quit_event_union;
			quit_event_union.quit = quit_event;
			
			g_guiQuitButton.isPressed = SDL_TRUE;

			SDL_PushEvent(&quit_event_union);
		}
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
	}
    else
    {
        Flappy_DoPrimaryAction();
    }
}


void Flappy_DoReturnKeyUpAction()
{
	if( ((g_gameState >= GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION) && (g_gameState <= GAMESTATE_FADE_OUT_GAME_OVER) )
	   || ((g_gameState >= GAMESTATE_TITLE_SCREEN) && (g_gameState <= GAMESTATE_FADE_OUT_TITLE_SCREEN) )
	)
	{
		if(SDL_TRUE == g_guiPlayButton.isSelected)
		{
			/* Warning: The mouse, gamepad, and keyboard will fight over this state if users want to break it */
			g_guiPlayButton.isPressed = SDL_FALSE;
		}
#if FLAPPY_PROVIDE_QUIT_BUTTON
		else if(SDL_TRUE == g_guiQuitButton.isSelected)
		{
			g_guiQuitButton.isPressed = SDL_FALSE;
		}
#endif /* FLAPPY_PROVIDE_QUIT_BUTTON */
	}
    else
    {
    }
}


void Flappy_DoUpKeyAction()
{
    if((GAMESTATE_GAME_OVER_WAITING_FOR_USER_ACTION == g_gameState)
		|| (GAMESTATE_TITLE_SCREEN == g_gameState)
	)
    {

    }
    else
    {
        Flappy_DoPrimaryAction();
    }
}



static int TemplateHelper_HandleAppEvents(void* user_data, SDL_Event* the_event)
{
    switch(the_event->type)
    {
		case SDL_APP_TERMINATING:
			/* Terminate the app.
			 Shut everything down before returning from this function.
			 */
			Flappy_SaveHighScoreToStorageIfNeeded();
			
			return 0;
		case SDL_APP_LOWMEMORY:
			/* You will get this when your app is paused and iOS wants more memory.
			 Release as much memory as possible.
			 */
			return 0;
		case SDL_APP_WILLENTERBACKGROUND:
			/* Prepare your app to go into the background.  Stop loops, etc.
			 This gets called when the user hits the home button, or gets a call.
			 */
			Flappy_SaveHighScoreToStorageIfNeeded();
			
			Flappy_PauseGame();
			return 0;
		case SDL_APP_DIDENTERBACKGROUND:
			/* This will get called if the user accepted whatever sent your app to the background.
			 If the user got a phone call and canceled it, you'll instead get an    SDL_APP_DIDENTERFOREGROUND event and restart your loops.
			 When you get this, you have 5 seconds to save all your state or the app will be terminated.
			 Your app is NOT active at this point.
			 */
			return 0;
		case SDL_APP_WILLENTERFOREGROUND:
			/* This call happens when your app is coming back to the foreground.
			 Restore all your state here.
			 */
			return 0;
		case SDL_APP_DIDENTERFOREGROUND:
			/* Restart your loops here.
			 Your app is interactive and getting CPU again.
			 */
			Flappy_UnpauseGame();
			return 0;
		default:
			/* No special processing, add it to the event queue */
			return 1;
    }
}



void main_loop()
{
	Uint32 last_frame_time;
	Uint32 current_frame_time;
	Uint32 startFrame;
	Uint32 delta_time;
	Uint32 endFrame;
	Sint32 delay;
	SDL_Event event;
	int the_result;
	Uint32 base_time = g_baseTime;
	SDL_Renderer* renderer = g_mainRenderer;
	SDL_bool app_done = g_appDone;
	
	/* We use TimeTicker for game time stuff, where time (the game) can be paused, or reset for a new game. */
//	base_time = TimeTicker_UpdateTime(g_gameClock);
	
	
	last_frame_time = TimeTicker_GetTime(g_gameClock);
	current_frame_time = TimeTicker_UpdateTime(g_gameClock);
	
	startFrame = SDL_GetTicks();
	
	
	delta_time = current_frame_time - last_frame_time;
	
	ALmixer_Update();
	
#if 1
	
	/* Check for events */
	do
	{
		the_result = SDL_PollEvent(&event);
		if(the_result > 0)
		{
			switch (event.type)
			{
					
				case SDL_MOUSEMOTION:
					//	SDL_Log("SDL_MOUSEMOTION, %d", the_result);
					Flappy_DoMouseMoved(event.motion);
					
					break;
				case SDL_MOUSEBUTTONDOWN:
					//	SDL_Log("SDL_MOUSEBUTTONDOWN, %d", the_result);
					
					//						Flappy_DoPrimaryAction();
					Flappy_DoMouseDown(event.button);
					break;
				case SDL_MOUSEBUTTONUP:
					//	SDL_Log("SDL_MOUSEBUTTONDOWN, %d", the_result);
					Flappy_DoMouseUp(event.button);
					//						Flappy_DoPrimaryAction();
					break;
				case SDL_KEYDOWN:
					//						SDL_Log("SDL_KEYDOWN, %d", the_result);
					if(SDLK_AC_BACK == event.key.keysym.sym)
					{
						SDL_Log("Android back button pressed, going to quit, %d", the_result);
						
						app_done = 1;
					}
					else if(SDLK_SPACE == event.key.keysym.sym)
					{
						Flappy_DoReturnKeyDownAction();
						
					}
					else if(SDLK_UP == event.key.keysym.sym)
					{
						Flappy_DoUpKeyAction();
					}
					else if(SDLK_LEFT == event.key.keysym.sym)
					{
						Flappy_DoLeftAction();
					}
					else if(SDLK_RIGHT == event.key.keysym.sym)
					{
						Flappy_DoRightAction();
					}
					
					else if(SDLK_RETURN == event.key.keysym.sym)
					{
						
						/* CMD-Enter (Mac) or Ctrl-Enter (everybody else) to toggle fullscreen */
#if __MACOSX__
						if(event.key.keysym.mod & KMOD_GUI)
						{
							TemplateHelper_ToggleFullScreen(g_mainWindow, g_mainRenderer);
						}
#else
						if(event.key.keysym.mod & KMOD_ALT)
						{
							TemplateHelper_ToggleFullScreen(g_mainWindow, g_mainRenderer);
						}
#endif
						else
						{
							Flappy_DoReturnKeyDownAction();
						}
					}
					else if(SDLK_RETURN2 == event.key.keysym.sym)
					{
						Flappy_DoReturnKeyDownAction();
					}
					else if(SDLK_p == event.key.keysym.sym)
					{
						Flappy_TogglePause();
					}
					
					/* CMD-F (Mac) or Ctrl-F (everybody else) to toggle fullscreen */
					else if(SDLK_f == event.key.keysym.sym)
					{
#if __MACOSX__
						if(event.key.keysym.mod & KMOD_GUI)
						{
							TemplateHelper_ToggleFullScreen(g_mainWindow, g_mainRenderer);
						}
#else
						if(event.key.keysym.mod & KMOD_CTRL)
						{
							TemplateHelper_ToggleFullScreen(g_mainWindow, g_mainRenderer);
						}
#endif
						else
						{
							Flappy_DoPrimaryAction();
						}
					}
					
#if ! __APPLE__
					else if(SDLK_F4 == event.key.keysym.sym)
					{
						if(event.key.keysym.mod & KMOD_ALT)
						{
							app_done = 1;
						}
					}
#endif
					else if(SDLK_ESCAPE == event.key.keysym.sym)
					{
						app_done = 1;
					}
					
					
					else if(SDLK_0 == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(0.5);
					}
					else if(SDLK_1 == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(1.0);
						
					}
					else if(SDLK_2 == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(2.0);
						
					}
					else if(SDLK_LSHIFT == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(0.5);
						
					}
					else if(SDLK_RSHIFT == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(2.0);
						
					}
					break;
				case SDL_KEYUP:
					if(SDLK_LSHIFT == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(1.0);
						
					}
					else if(SDLK_RSHIFT == event.key.keysym.sym)
					{
						Flappy_SetGameTimeScale(1.0);
					}
					else if(SDLK_RETURN == event.key.keysym.sym)
					{
						
						/* CMD-Enter (Mac) or Ctrl-Enter (everybody else) to toggle fullscreen */
#if __MACOSX__
						if(event.key.keysym.mod & KMOD_GUI)
						{
						}
#else
						if(event.key.keysym.mod & KMOD_ALT)
						{
						}
#endif
						else
						{
							Flappy_DoReturnKeyUpAction();
						}
					}
					else if(SDLK_RETURN2 == event.key.keysym.sym)
					{
						Flappy_DoReturnKeyUpAction();
					}
					else if(SDLK_SPACE == event.key.keysym.sym)
					{
						Flappy_DoReturnKeyUpAction();
						
					}
					break;
				case SDL_CONTROLLERBUTTONDOWN:
					if(SDL_CONTROLLER_BUTTON_A == event.cbutton.button)
					{
						Flappy_DoReturnKeyDownAction();
					}
					else if(SDL_CONTROLLER_BUTTON_LEFTSHOULDER == event.cbutton.button)
					{
						Flappy_SetGameTimeScale(0.5);
					}
					else if(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER == event.cbutton.button)
					{
						Flappy_SetGameTimeScale(2.0);
					}
					else if(SDL_CONTROLLER_BUTTON_START == event.cbutton.button)
					{
						Flappy_TogglePause();
					}
					
					break;
				case SDL_CONTROLLERBUTTONUP:
					if(SDL_CONTROLLER_BUTTON_LEFTSHOULDER == event.cbutton.button)
					{
						Flappy_SetGameTimeScale(1.0);
					}
					if(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER == event.cbutton.button)
					{
						Flappy_SetGameTimeScale(1.0);
					}
					
					if(SDL_CONTROLLER_BUTTON_A == event.cbutton.button)
					{
						Flappy_DoReturnKeyUpAction();
					}
					break;
					
				case SDL_CONTROLLERDEVICEADDED:
					/* Note: This is the device id, not the instance id */
					Flappy_AddController(event.cdevice.which);
					
					break;
				case SDL_CONTROLLERDEVICEREMOVED:
					/* Note: This is the instance id, not the device id */
					Flappy_RemoveController(event.cdevice.which);
					break;
					
				case SDL_CONTROLLERAXISMOTION:
				{
					const Sint16 DEAD_ZONE = 15000;  /* !!! FIXME: real deadzone */
					
					/* horizontal axis */
					if(SDL_CONTROLLER_AXIS_LEFTX == event.caxis.axis)
					{
						//SDL_Log("event.caxis.value %d", event.caxis.value);
						if(event.caxis.value < -DEAD_ZONE)
						{
							Flappy_DoLeftAction();
						}
						else if(event.caxis.value > DEAD_ZONE)
						{
							Flappy_DoRightAction();
						}
					}
					break;
				}
				case SDL_WINDOWEVENT:
					switch(event.window.event)
				{
					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_MINIMIZED:
						//								Flappy_PauseGame();
						break;
					case SDL_WINDOWEVENT_RESIZED:
						SDL_RenderSetLogicalSize(g_mainRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);

					default:
						break;
				}
					break;
					
				case SDL_QUIT:
				case SDL_APP_TERMINATING:
					//						SDL_Log("SDL_QUIT, %d", the_result);
					app_done = 1;
					break;
				default:
					break;
			}
		}
	} while(the_result > 0);
	
#endif
	Flappy_Update(delta_time, base_time, current_frame_time);
	render(renderer);
	
	g_appDone = app_done;
	
	endFrame = SDL_GetTicks();
	TemplateHelper_UpdateFps();
	if( (g_myFPSPrintTimer +  3000) < (endFrame))
	{
		s_lastRecordedFPS = g_framesPerSecond;
		//			SDL_Log("%f\n", framespersecond);
		
		g_myFPSPrintTimer = endFrame;
	}
#define FPS_CAP
#ifdef FPS_CAP
#define SAFE_SLEEP_INTERVAL 10
	/* figure out how much time we have left, and then sleep */
	delay = MILLESECONDS_PER_FRAME - (Sint32)(endFrame - startFrame);
	if (delay <= 0)
	{
		delay = 0;
	}
	/*
		else if(delay < SAFE_SLEEP_INTERVAL)
		{
	 while((MILLESECONDS_PER_FRAME - (Sint32)(endFrame - startFrame)) >= 0)
	 {
	 //				SDL_Delay(0);
	 endFrame = SDL_GetTicks();
	 
	 }
		}
	 */
	/*
		else if (delay > MILLESECONDS_PER_FRAME) {
	 delay = MILLESECONDS_PER_FRAME;
	 //			SDL_Delay(delay);
	 while((MILLESECONDS_PER_FRAME - (Sint32)(endFrame - startFrame)) >= 0)
	 {
	 // SDL_Delay(0);
	 endFrame = SDL_GetTicks();
	 
	 }
	 
	 }
	 */
	else
	{
		//	SDL_Log("%d\n", delay);
		Uint32 remainder;
		Uint32 safe_sleep;
		safe_sleep = (delay / SAFE_SLEEP_INTERVAL) * SAFE_SLEEP_INTERVAL;
		//			remainder = delay % SAFE_SLEEP_INTERVAL;
		
#if 0
		SDL_Delay(safe_sleep);
		endFrame = SDL_GetTicks();
		while((MILLESECONDS_PER_FRAME - (Sint32)(endFrame - startFrame)) >= 0)
		{
			//				SDL_Delay(0);
			endFrame = SDL_GetTicks();
			
		}
#else
		SDL_Delay(delay);
		
#endif
		
	}
#endif /* FPS_CAP */
}

int main(int argc, char* argv[])
{
	SDL_Window* window;
	SDL_Renderer* renderer;

	g_myFPSPrintTimer = 0;
	
	
    /* initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		SDL_Log("Could not initialize SDL");
	}

	if(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF) < 0)
	{
		SDL_Log("Could not initialize SDL_image");
	}

	if(TTF_Init() < 0)
	{
		SDL_Log("Could not initialize SDL_ttf");
	}

	#ifdef __ANDROID__
	{
		jobject java_activity_context = (jobject)SDL_AndroidGetActivity();
		ALmixer_Android_Init(java_activity_context);
	}
	#endif
	if(ALmixer_Init(0, 0, 0) == AL_FALSE)
	{
		SDL_Log("Could not initialize ALmixer");
	}


#if 1
	SDL_SetEventFilter(TemplateHelper_HandleAppEvents, NULL);

	
	// This will prefer OpenGL as a backend. You may skip this if you don't care and SDL will pick its default.
	// On Windows, Direct3D will be picked as the default without this.
	// Direct3D has a problem where our render-to-texture targets can be suddenly lost, particularly during a screen-resize or fullscreen-toggle.
	// OpenGL avoids this problem.
	// Also note that the "overscan" experimental option may need some work with Direct3D
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	
	// This is an Blurrr experimental patch to SDL to support overscan instead of just letterbox.
	// overscan will zoom the screen so there are no black bars unlike letterbox, while preserving the aspect ratio (like letterbox)
	// FlappyBlurrr typically has enough space on the edges that it won't hurt to not see them, so it will look better without sidebars.
	// Comment out the line or use "letterbox" as the parameter to disable overscan.
	SDL_SetHint(SDL_HINT_RENDER_LOGICAL_SIZE_MODE, "overscan");
	

	
 //       SDL_Log("SDL_CreateWindow");
	
    //window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    window = SDL_CreateWindow("Flappy Blurrr (C)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
							  SDL_WINDOW_RESIZABLE
							  | SDL_WINDOW_FULLSCREEN_DESKTOP

							  );
	if(NULL == window)
	{
        SDL_Log("SDL_CreateWindow failed");
        SDL_Log("SDL_CreateWindow failed %s", SDL_GetError());
	}
	else
	{
//        SDL_Log("SDL_CreateWindow passed");
	}
	
	g_mainWindow = window;
	
    renderer = SDL_CreateRenderer(window, -1,
								  0
	 |	SDL_RENDERER_PRESENTVSYNC
	//	| SDL_RENDERER_ACCELERATED
	);
	if(NULL == renderer)
	{
        SDL_Log("SDL_CreateRenderer failed");
        SDL_Log("SDL_CreateRenderer failed %s", SDL_GetError());
		
	}
	else
	{
//        SDL_Log("SDL_CreateRenderer passed");
	}

	g_mainRenderer = renderer;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");  // going for an 8-bit retro blocky look, nearest is better than linear
//	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
//	SDL_SetHint(SDL_HINT_RENDER_LOGICAL_SIZE_MODE, "overscan");
//	SDL_SetHint(SDL_HINT_RENDER_LOGICAL_SIZE_MODE, "letterbox"); 
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	Flappy_InitGameControllerDataMap();
	
	/* We use rand() for pipe heights. Seed srand with results from time() so ever game should be unique. */
	srand(time(NULL));
	
	g_gameClock = TimeTicker_Create();
	
	
	initializeFont();

	initializeTexture(renderer);

 //       SDL_Log("initializeTexture passed");
	Flappy_LoadSounds();
	ALmixer_SetPlaybackFinishedCallback(Flappy_SoundFinishedCallback, NULL);
	
	InitPhysics();
	
	g_highScoreData.savedHighScore = Flappy_LoadSavedHighScoreFromStorage();
	
//	InitNewGame();
	InitTitleScreen();
	
#endif
	
	{
#define MAX_FILE_STRING_LENGTH 2048

		char base_path[MAX_FILE_STRING_LENGTH];
		char resource_file_path[MAX_FILE_STRING_LENGTH];
		BlurrrPath_GetResourceDirectoryString(base_path, MAX_FILE_STRING_LENGTH);

		SDL_strlcpy(resource_file_path, base_path, MAX_FILE_STRING_LENGTH);
		SDL_strlcat(resource_file_path, "gamecontrollerdb.txt", MAX_FILE_STRING_LENGTH);
	
		SDL_GameControllerAddMappingsFromFile(resource_file_path);
		
		{
			int i;
			int number_of_joysticks = SDL_NumJoysticks();
			for(i=0; i < number_of_joysticks; i++)
			{
				Flappy_AddController(i);
			}
		}

		
	}
	
	TemplateHelper_InitFps();
	
	g_appDone = 0;
//	base_time = SDL_GetTicks();
	TimeTicker_Start(g_gameClock);

	
	/* We use TimeTicker for game time stuff, where time (the game) can be paused, or reset for a new game. */
	g_baseTime = TimeTicker_UpdateTime(g_gameClock);
	
	while ( !g_appDone )
	{
		main_loop();
	

	}
	
	/* Disable the callback in case SDL tries to invoke it before quit */
	SDL_SetEventFilter(NULL, NULL);
	
	Flappy_SaveHighScoreToStorageIfNeeded();


	SDL_DestroyTexture(s_textureFPS);
	SDL_FreeSurface(s_surfaceFPS);

	SDL_DestroyTexture(texture);

	TTF_CloseFont(s_retroFontForMainScore);
	TTF_CloseFont(s_veraMonoFont);

	SDL_free(faces);

	/* Clean up the SDL library */
	ALmixer_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	return(0);
}

