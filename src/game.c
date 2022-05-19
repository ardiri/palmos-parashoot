/*
 * @(#)game.c
 *
 * Copyright 1999-2000, Aaron Ardiri (mailto:aaron@ardiri.com)
 * All rights reserved.
 *
 * This file was generated as part of the "parashoot" program developed 
 * for the Palm Computing Platform designed by Palm: 
 *
 *  http://www.palm.com/ 
 *
 * The contents of this file is confidential and proprietrary in nature 
 * ("Confidential Information"). Redistribution or modification without 
 * prior consent of the original author is prohibited. 
 */

#include "palm.h"

// interface

static void GameAdjustLevel(PreferencesType *)                      __GAME__;
static void GameIncrementScore(PreferencesType *)                   __GAME__;
static void GameMovePlayer(PreferencesType *)                       __GAME__;
static void GameMoveParachuters(PreferencesType *)                  __GAME__;
static void GameRemoveParachuter(PreferencesType *, UInt16, UInt16) __GAME__;

// global variable structure
typedef struct
{
  WinHandle winDigits;                      // scoring digits bitmaps
  WinHandle winMisses;                      // the lives notification bitmaps

  WinHandle winBlades;                      // the blade bitmaps
  Boolean   bladeChanged;                   // do we need to repaint the blade?
  Boolean   bladeOnScreen;                  // is blade bitmap on screen?
  UInt16    bladeOldPosition;               // the *old* position of the blade 

  WinHandle winSharks;                      // the shark bitmaps
  Boolean   sharkChanged;                   // do we need to repaint the shark?
  Boolean   sharkOnScreen;                  // is shark bitmap on screen?
  UInt16    sharkOldPosition;               // the *old* position of the shark 

  WinHandle winBoats;                       // the boat bitmaps
  Boolean   boatChanged;                    // do we need to repaint boat?
  Boolean   boatOnScreen;                   // is boat bitmap on screen?
  UInt16    boatOldPosition;                // the *old* position of boat 

  WinHandle winParachuters;                 // the parachuter bitmaps
  Boolean   parachuteChanged[4][7];         // do we need to repaint parachute
  Boolean   parachuteOnScreen[4][7];        // is parachute bitmap on screen?
  UInt16    parachuteOnScreenPosition[4][7];// the *old* position of parachute 

  WinHandle winParachuterDeaths;            // the parachuter death bitmaps

  UInt8     gameType;                       // the type of game active
  Boolean   playerDied;                     // has the player died?
  UInt8     moveDelayCount;                 // the delay between moves
  UInt8     moveLast;                       // the last move performed
  UInt8     moveNext;                       // the next desired move

  struct {

    Boolean    gamePadPresent;              // is the gamepad driver present
    UInt16     gamePadLibRef;               // library reference for gamepad

  } hardware;

} GameGlobals;

/**
 * Initialize the Game.
 */  
Boolean   
GameInitialize()
{
  GameGlobals *gbls;
  Err         err;
  Boolean     result;

  // create the globals object, and register it
  gbls = (GameGlobals *)MemPtrNew(sizeof(GameGlobals));
  MemSet(gbls, sizeof(GameGlobals), 0);
  FtrSet(appCreator, ftrGameGlobals, (UInt32)gbls);

  // load the gamepad driver if available
  {
    Err err;

    // attempt to load the library
    err = SysLibFind(GPD_LIB_NAME,&gbls->hardware.gamePadLibRef);
    if (err == sysErrLibNotFound)
      err = SysLibLoad('libr',GPD_LIB_CREATOR,&gbls->hardware.gamePadLibRef);

    // lets determine if it is available
    gbls->hardware.gamePadPresent = (err == errNone);

    // open the library if available
    if (gbls->hardware.gamePadPresent)
      GPDOpen(gbls->hardware.gamePadLibRef);
  }

  // initialize our "bitmap" windows
  err = errNone;
  {
    UInt16 i, j;
    Err    e;

    gbls->winDigits = 
      WinCreateOffscreenWindow(70, 12, screenFormat, &e); err |= e;

    gbls->winMisses = 
      WinCreateOffscreenWindow(132, 16, screenFormat, &e); err |= e;

    gbls->winBlades = 
      WinCreateOffscreenWindow(72, 6, screenFormat, &e); err |= e;
    gbls->bladeChanged     = true;
    gbls->bladeOnScreen    = false;
    gbls->bladeOldPosition = 0;

    gbls->winSharks = 
      WinCreateOffscreenWindow(160, 16, screenFormat, &e); err |= e;
    gbls->sharkChanged     = true;
    gbls->sharkOnScreen    = false;
    gbls->sharkOldPosition = 0;

    gbls->winBoats = 
      WinCreateOffscreenWindow(96, 14, screenFormat, &e); err |= e;
    gbls->boatChanged     = true;
    gbls->boatOnScreen    = false;
    gbls->boatOldPosition = 0;

    gbls->winParachuters = 
      WinCreateOffscreenWindow(126, 80, screenFormat, &e); err |= e;
    for (i=0; i<4; i++) {
      for (j=0; j<7; j++) {
        gbls->parachuteChanged[i][j]          = true;
        gbls->parachuteOnScreen[i][j]         = false;
        gbls->parachuteOnScreenPosition[i][j] = 0;
      }
    }

    gbls->winParachuterDeaths = 
      WinCreateOffscreenWindow(144, 10, screenFormat, &e); err |= e;
  }

  // no problems creating back buffers? load images.
  if (err == errNone) {

    WinHandle currWindow;
    MemHandle bitmapHandle;

    currWindow = WinGetDrawWindow();

    // digits
    WinSetDrawWindow(gbls->winDigits);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapDigits);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // misses
    WinSetDrawWindow(gbls->winMisses);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapMisses);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // blade
    WinSetDrawWindow(gbls->winBlades);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapBlades);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // shark
    WinSetDrawWindow(gbls->winSharks);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapSharks);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // boat
    WinSetDrawWindow(gbls->winBoats);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapBoats);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // parachuter
    WinSetDrawWindow(gbls->winParachuters);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapParachuters);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // parachuter deaths
    WinSetDrawWindow(gbls->winParachuterDeaths);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapParachuterDeath);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    WinSetDrawWindow(currWindow);
  }

  result = (err == errNone);

  return result;
}

/**
 * Reset the Game.
 * 
 * @param prefs the global preference data.
 * @param gameType the type of game to configure for.
 */  
void   
GameReset(PreferencesType *prefs, Int8 gameType)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // turn off all the "bitmaps"
  FrmDrawForm(FrmGetActiveForm());

  // turn on all the "bitmaps"
  {
    RectangleType rect    = { {   0,   0 }, {   0,   0 } };
    RectangleType scrRect = { {   0,   0 }, {   0,   0 } };
    UInt16        i;

    //
    // draw the score
    //

    for (i=0; i<4; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i,
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = 8 * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
    }

    //
    // draw the misses
    //

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0,
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 44;
    scrRect.extent.y  = 16;
    rect.topLeft.x    = 2 * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    //
    // draw the blades
    //

    for (i=0; i<2; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteBlade, 0, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 36;
      scrRect.extent.y  = 6;
      rect.topLeft.x    = 0;  
      rect.topLeft.y    = 0;  
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // what is the rectangle we need to copy?
      rect.topLeft.x = i * scrRect.extent.x; 
      rect.topLeft.y = 0;
      rect.extent.x  = scrRect.extent.x;
      rect.extent.y  = scrRect.extent.y;

      // draw the blade bitmap!
      WinCopyRectangle(gbls->winBlades, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }
    
    // 
    // draw the sharks
    //

    for (i=0; i<5; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteShark, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 32;
      scrRect.extent.y  = 16;
      rect.topLeft.x    = i * scrRect.extent.x; 
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the shark bitmap!
      WinCopyRectangle(gbls->winSharks, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    //
    // draw the parachuters
    //

    for (i=0; i<5; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteParachuter, i,
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 18;
      scrRect.extent.y  = 20;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 0 * scrRect.extent.y;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the parachute bitmap!
      WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    for (i=0; i<6; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteParachuter, 7 + i,
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 18;
      scrRect.extent.y  = 20;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 1 * scrRect.extent.y;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the parachute bitmap!
      WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    for (i=0; i<7; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteParachuter, 14 + i,
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 18;
      scrRect.extent.y  = 20;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 2 * scrRect.extent.y;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the parachute bitmap!
      WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    for (i=0; i<2; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteParachuter, 21 + i,
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 18;
      scrRect.extent.y  = 20;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 3 * scrRect.extent.y;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the parachute bitmap!
      WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    //
    // draw the parachuters deaths :))
    //

    for (i=0; i<6; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteParachuterDeath, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 24;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = i * scrRect.extent.x; 
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the parachuter bitmap!
      WinCopyRectangle(gbls->winParachuterDeaths, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    // 
    // draw the boat
    //

    for (i=0; i<6; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteBoat, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 32;
      scrRect.extent.y  = 14;
      rect.topLeft.x    = i * scrRect.extent.x; 
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the boat bitmap
      WinCopyRectangle(gbls->winBoats, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }
  }

  // wait a good two seconds :))
  SysTaskDelay(2 * SysTicksPerSecond());

  // turn off all the "bitmaps"
  FrmDrawForm(FrmGetActiveForm());

  // reset the preferences
  GameResetPreferences(prefs, gameType);
}

/**
 * Reset the Game preferences.
 * 
 * @param prefs the global preference data.
 * @param gameType the type of game to configure for.
 */  
void   
GameResetPreferences(PreferencesType *prefs, Int8 gameType)
{
  GameGlobals *gbls;
  UInt16      i, j;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // now we are playing
  prefs->game.gamePlaying                   = true;
  prefs->game.gamePaused                    = false;
  prefs->game.gameWait                      = true;
  prefs->game.gameAnimationCount            = 0;

  // reset score and lives
  prefs->game.gameScore                     = 0;
  prefs->game.gameLives                     = 3;

  // reset parashoot specific things
  prefs->game.parashoot.gameType            = gameType;
  prefs->game.parashoot.gameLevel           = 1;
  prefs->game.parashoot.bonusAvailable      = true;
  prefs->game.parashoot.bonusScoring        = false;

  prefs->game.parashoot.bladeWait           = 0;
  prefs->game.parashoot.bladePosition       = 0;
  prefs->game.parashoot.sharkWait           = 0;
  prefs->game.parashoot.sharkPosition       = 0;

  prefs->game.parashoot.boatPosition        = 0;
  prefs->game.parashoot.boatNewPosition     = 0;
  for (i=0; i<4; i++) {
    prefs->game.parashoot.parachuteCount[i] = 0;
    MemSet(prefs->game.parashoot.parachutePosition[i], sizeof(UInt16) * 7, 0);
    MemSet(prefs->game.parashoot.parachuteWait[i],     sizeof(UInt16) * 7, 0);
  }
  prefs->game.parashoot.parachuteDeathPosition = 0;

  // reset the "backup" and "onscreen" flags
  gbls->bladeChanged                        = true;
  gbls->sharkChanged                        = true;
  gbls->boatChanged                         = true;
  for (i=0; i<4; i++) {
    for (j=0; j<7; j++) {
      gbls->parachuteChanged[i][j]          = true;
      gbls->parachuteOnScreen[i][j]         = false;
    }
  }

  gbls->gameType                            = gameType;
  gbls->playerDied                          = false;
  gbls->moveDelayCount                      = 0;
  gbls->moveLast                            = moveNone;
  gbls->moveNext                            = moveNone;
}

/**
 * Process key input from the user.
 * 
 * @param prefs the global preference data.
 * @param keyStatus the current key state.
 */  
void   
GameProcessKeyInput(PreferencesType *prefs, UInt32 keyStatus)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  keyStatus &= (prefs->config.ctlKeyLeft  |
                prefs->config.ctlKeyRight);

  // additional checks here
  if (gbls->hardware.gamePadPresent) {

    UInt8 gamePadKeyStatus;
    Err   err;

    // read the state of the gamepad
    err = GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
    if (err == errNone) {

      // process
      if (((gamePadKeyStatus & GAMEPAD_LEFT)      != 0) ||
          ((gamePadKeyStatus & GAMEPAD_LEFTFIRE)  != 0))
        keyStatus |= prefs->config.ctlKeyLeft;
      if (((gamePadKeyStatus & GAMEPAD_RIGHT)     != 0) ||
          ((gamePadKeyStatus & GAMEPAD_RIGHTFIRE) != 0))
        keyStatus |= prefs->config.ctlKeyRight;
	
      // special purpose :)
      if  ((gamePadKeyStatus & GAMEPAD_SELECT)    != 0) {

        // wait until they let it go :)
        do {
          GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
        } while ((gamePadKeyStatus & GAMEPAD_SELECT) != 0);

        keyStatus = 0;
        prefs->game.gamePaused = !prefs->game.gamePaused;
      }
      if  ((gamePadKeyStatus & GAMEPAD_START)     != 0) {

        // wait until they let it go :)
        do {
          GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
        } while ((gamePadKeyStatus & GAMEPAD_START) != 0);

        keyStatus = 0;
	GameReset(prefs, prefs->game.parashoot.gameType);
      }
    }
  }

  // did they press at least one of the game keys?
  if (keyStatus != 0) {

    // if they were waiting, we should reset the game animation count
    if (prefs->game.gameWait) { 
      prefs->game.gameAnimationCount = 0;
      prefs->game.gameWait           = false;
    }

    // great! they wanna play
    prefs->game.gamePaused = false;
  }

  // move left
  if (
      ((keyStatus &  prefs->config.ctlKeyLeft) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveLeft)
      )
     ) {

    // adjust the position if possible
    if (prefs->game.parashoot.boatPosition > 0) {
      prefs->game.parashoot.boatNewPosition = 
        prefs->game.parashoot.boatPosition - 1;
    }
  }

  // move right
  else
  if (
      ((keyStatus & prefs->config.ctlKeyRight) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveRight)
      )
     ) {

    // adjust the position if possible
    if (prefs->game.parashoot.boatPosition < 2) {
      prefs->game.parashoot.boatNewPosition = 
        prefs->game.parashoot.boatPosition + 1;
    }
  }
}
  
/**
 * Process stylus input from the user.
 * 
 * @param prefs the global preference data.
 * @param x the x co-ordinate of the stylus event.
 * @param y the y co-ordinate of the stylus event.
 */  
void   
GameProcessStylusInput(PreferencesType *prefs, Coord x, Coord y)
{
  RectangleType rect;
  UInt16        i;

  // lets take a look at all the possible "positions"
  for (i=0; i<3; i++) {

    // get the bounding box of the position
    GameGetSpritePosition(spriteBoat, i,
                          &rect.topLeft.x, &rect.topLeft.y);
    rect.extent.x = 32;
    rect.extent.y = 14; 

    // did they tap inside this rectangle?
    if (RctPtInRectangle(x, y, &rect)) {

      // ok, this is where we are going to go :)
      prefs->game.parashoot.boatNewPosition = i;

      // if they were waiting, we should reset the game animation count
      if (prefs->game.gameWait) { 
        prefs->game.gameAnimationCount = 0;
        prefs->game.gameWait           = false;
      }

      // great! they wanna play
      prefs->game.gamePaused = false;
      break;                                       // stop looking
    }
  }
}

/**
 * Process the object movement in the game.
 * 
 * @param prefs the global preference data.
 */  
void   
GameMovement(PreferencesType *prefs)
{
  const CustomPatternType erase = { 0,0,0,0,0,0,0,0 };
  const RectangleType     rect  = {{   0,  16 }, { 160, 16 }};

  GameGlobals    *gbls;
  SndCommandType deathSnd = {sndCmdFreqDurationAmp,0, 512,50,sndMaxAmp};
  UInt16         i, j;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  //
  // the game is NOT paused.
  //

  if (!prefs->game.gamePaused) {

    // animate the blade
    if (prefs->game.parashoot.bladeWait == 0) {
    
      prefs->game.parashoot.bladePosition =
        1 - prefs->game.parashoot.bladePosition;
      prefs->game.parashoot.bladeWait     = 4;
  
      gbls->bladeChanged = true;
    }
    else {
      prefs->game.parashoot.bladeWait--;
    }

    // animate the shark
    if (prefs->game.parashoot.sharkWait == 0) {
    
      prefs->game.parashoot.sharkPosition =
        (prefs->game.parashoot.sharkPosition + 1) % 5;
      prefs->game.parashoot.sharkWait     = 4;
  
      gbls->sharkChanged = true;
    }
    else {
      prefs->game.parashoot.sharkWait--;
    }

    // we must make sure the user is ready for playing 
    if (!prefs->game.gameWait) {

      // we cannot be dead yet :)
      gbls->playerDied = false;

      // are we in bonus mode?
      if ((prefs->game.parashoot.bonusScoring) &&
          (prefs->game.gameAnimationCount % GAME_FPS) < (GAME_FPS >> 1)) {

        Char   str[32];
        FontID currFont = FntGetFont();

        StrCopy(str, "    * BONUS PLAY *    ");
        FntSetFont(boldFont);
        WinDrawChars(str, StrLen(str), 
                     80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
        FntSetFont(currFont);
      }
      else {

        // erase the status area
        WinSetPattern(&erase);
        WinFillRectangle(&rect, 0);
      }

      // player gets first move
      GameMovePlayer(prefs);
      GameMoveParachuters(prefs);

      // is it time to upgrade the game?
      if (prefs->game.gameAnimationCount >= 
           ((gbls->gameType == GAME_A) ? 0x17f : 0x100)) {

        prefs->game.gameAnimationCount = 0;
        prefs->game.parashoot.gameLevel++;

        // upgrading of difficulty?
        if (
            (gbls->gameType                  == GAME_A) &&
            (prefs->game.parashoot.gameLevel > 12)
           ) {

          gbls->gameType                   = GAME_B;
          prefs->game.parashoot.gameLevel -= 2; // give em a break :)
        }
      } 

      // has the player died in this frame?
      if (gbls->playerDied) {

        UInt16        index;
        RectangleType rect    = { {   0,   0 }, {   0,   0 } };
        RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

        // erase the previous shark (wherever it was)
        if (gbls->sharkOnScreen) {

          index = gbls->sharkOldPosition;

          // what is the rectangle we need to copy?
          GameGetSpritePosition(spriteShark, index, 
                                &scrRect.topLeft.x, &scrRect.topLeft.y);
          scrRect.extent.x  = 32;
          scrRect.extent.y  = 16;
          rect.topLeft.x    = index * scrRect.extent.x; 
          rect.topLeft.y    = 0;
          rect.extent.x     = scrRect.extent.x;
          rect.extent.y     = scrRect.extent.y;

          // invert the shark bitmap!
          WinCopyRectangle(gbls->winSharks, WinGetDrawWindow(),
                           &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
          gbls->sharkOnScreen = false;
        }

        //
        // animate the chasing and eating of the parachuter
        //

        for (i=prefs->game.parashoot.parachuteDeathPosition; i<6; i++) {

          // do twice, first time = draw, second time = erase
          for (j=0; j<2; j++) {

            // draw parachuter in water
            {
              // what is the rectangle we need to copy?
              GameGetSpritePosition(spriteParachuterDeath, i, 
                                    &scrRect.topLeft.x, &scrRect.topLeft.y);
              scrRect.extent.x  = 24;
              scrRect.extent.y  = 10;
              rect.topLeft.x    = i * scrRect.extent.x; 
              rect.topLeft.y    = 0;
              rect.extent.x     = scrRect.extent.x;
              rect.extent.y     = scrRect.extent.y;

              // invert the parachuter bitmap!
              WinCopyRectangle(gbls->winParachuterDeaths, WinGetDrawWindow(),
                               &rect, scrRect.topLeft.x, scrRect.topLeft.y, winInvert);
            }

            // draw shark chasing parachuter
            if (i > 0) {

              // what is the rectangle we need to copy?
              GameGetSpritePosition(spriteShark, (i-1), 
                                    &scrRect.topLeft.x, &scrRect.topLeft.y);
              scrRect.extent.x  = 32;
              scrRect.extent.y  = 16;
              rect.topLeft.x    = (i-1) * scrRect.extent.x; 
              rect.topLeft.y    = 0;
              rect.extent.x     = scrRect.extent.x;
              rect.extent.y     = scrRect.extent.y;

              // invert the shark bitmap!
              WinCopyRectangle(gbls->winSharks, WinGetDrawWindow(),
                               &rect, scrRect.topLeft.x, scrRect.topLeft.y, winInvert);
            }
            if (j == 1) continue; // we only want sound in first pass

            // play death sound
            DevicePlaySound(&deathSnd);
            SysTaskDelay(50);
          }
        }

        // lose a life :(
        prefs->game.gameLives--;

        // no more lives left: GAME OVER!
        if (prefs->game.gameLives == 0) {

          EventType event;

          // GAME OVER - return to main screen
          MemSet(&event, sizeof(EventType), 0);
          event.eType            = menuEvent;
          event.data.menu.itemID = gameMenuItemExit;
          EvtAddEventToQueue(&event);

          prefs->game.gamePlaying = false;
        }

        // continue game
        else {
          GameAdjustLevel(prefs);
          prefs->game.parashoot.bonusScoring = false;
          prefs->game.gameWait               = true;
        }
      }
    }

    // we have to display "GET READY!"
    else {

      // flash on:
      if ((prefs->game.gameAnimationCount % GAME_FPS) < (GAME_FPS >> 1)) {

        Char   str[32];
        FontID currFont = FntGetFont();

        StrCopy(str, "    * GET READY *    ");
        FntSetFont(boldFont);
        WinDrawChars(str, StrLen(str), 
                     80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
        FntSetFont(currFont);
      }

      // flash off:
      else {

        // erase the status area
        WinSetPattern(&erase);
        WinFillRectangle(&rect, 0);
      }
    }

    // update the animation counter
    prefs->game.gameAnimationCount++;
  }

  //
  // the game is paused.
  //

  else {

    Char   str[32];
    FontID currFont = FntGetFont();

    StrCopy(str, "    *  PAUSED  *    ");
    FntSetFont(boldFont);
    WinDrawChars(str, StrLen(str), 
                 80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
    FntSetFont(currFont);
  }
}

/**
 * Draw the game on the screen.
 * 
 * @param prefs the global preference data.
 */
void   
GameDraw(PreferencesType *prefs)
{
  GameGlobals   *gbls;
  UInt16        i, j, index;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // 
  // DRAW INFORMATION/BITMAPS ON SCREEN
  //

  // draw the score
  {
    UInt16 base;
 
    base = 1000;  // max score (4 digits)
    for (i=0; i<4; i++) {

      index = (prefs->game.gameScore / base) % 10;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
      base /= 10;
    }
  }

  // draw the misses that have occurred :( 
  if (prefs->game.gameLives < 3) {
  
    index = 2 - prefs->game.gameLives;

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 44;
    scrRect.extent.y  = 16;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
  }

  // no missed, make sure none are shown
  else {
  
    index = 2;  // the miss with *all* three misses

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 44;
    scrRect.extent.y  = 16;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // invert the three miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
  }

  // draw the blade on the screen (only if it has changed)
  if (gbls->bladeChanged) {

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteBlade, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 36;
    scrRect.extent.y  = 6;
    rect.topLeft.x    = 0;  
    rect.topLeft.y    = 0;  
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // 
    // erase the previous blade 
    // 

    if (gbls->bladeOnScreen) {

      index = gbls->bladeOldPosition;

      // what is the rectangle we need to copy?
      rect.topLeft.x = index * scrRect.extent.x; 
      rect.topLeft.y = 0;
      rect.extent.x  = scrRect.extent.x;
      rect.extent.y  = scrRect.extent.y;

      // invert the blade bitmap!
      WinCopyRectangle(gbls->winBlades, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->bladeOnScreen = false;
    }

    // 
    // draw the blade at the new position
    // 

    index = prefs->game.parashoot.bladePosition;

    // what is the rectangle we need to copy?
    rect.topLeft.x = index * scrRect.extent.x; 
    rect.topLeft.y = 0;
    rect.extent.x  = scrRect.extent.x;
    rect.extent.y  = scrRect.extent.y;

    // save this location, record blade is onscreen
    gbls->bladeOnScreen    = true;
    gbls->bladeOldPosition = index;

    // draw the blade bitmap!
    WinCopyRectangle(gbls->winBlades, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // dont draw until we need to
    gbls->bladeChanged = false;
  }

  // draw the shark on the screen (only if it has changed)
  if (gbls->sharkChanged) {

    // 
    // erase the previous shark 
    // 

    if (gbls->sharkOnScreen) {

      index = gbls->sharkOldPosition;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteShark, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 32;
      scrRect.extent.y  = 16;
      rect.topLeft.x    = index * scrRect.extent.x; 
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // invert the shark bitmap!
      WinCopyRectangle(gbls->winSharks, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->sharkOnScreen = false;
    }

    // 
    // draw the shark at the new position
    // 

    index = prefs->game.parashoot.sharkPosition;

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteShark, index, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 32;
    scrRect.extent.y  = 16;
    rect.topLeft.x    = index * scrRect.extent.x; 
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // save this location, record shark is onscreen
    gbls->sharkOnScreen    = true;
    gbls->sharkOldPosition = index;

    // draw the shark bitmap!
    WinCopyRectangle(gbls->winSharks, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // dont draw until we need to
    gbls->sharkChanged = false;
  }

  // draw the parachutes
  for (i=0; i<4; i++) {

    // process each row individually
    for (j=0; j<prefs->game.parashoot.parachuteCount[i]; j++) {

      // draw the parachute on the screen (only if it has changed)
      if (gbls->parachuteChanged[i][j]) {

        //
        // erase the previous parachuter
        //
 
        if (gbls->parachuteOnScreen[i][j]) {

          index = (i * 7) +  // take into account 'layers'
                  gbls->parachuteOnScreenPosition[i][j];

          // what is the rectangle we need to copy?
          GameGetSpritePosition(spriteParachuter, index,
                                &scrRect.topLeft.x, &scrRect.topLeft.y);
          scrRect.extent.x  = 18;
          scrRect.extent.y  = 20;
          rect.topLeft.x    = (index % 7) * scrRect.extent.x;
          rect.topLeft.y    = i * scrRect.extent.y;
          rect.extent.x     = scrRect.extent.x;
          rect.extent.y     = scrRect.extent.y;

          // invert the old parachute bitmap!
          WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                           &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
        }

        //
        // draw the parachuter at the new position
        //

        index = (i * 7) +  // take into account 'layers'
                prefs->game.parashoot.parachutePosition[i][j];

        // what is the rectangle we need to copy?
        GameGetSpritePosition(spriteParachuter, index,
                              &scrRect.topLeft.x, &scrRect.topLeft.y);
        scrRect.extent.x  = 18;
        scrRect.extent.y  = 20;
        rect.topLeft.x    = (index % 7) * scrRect.extent.x;
        rect.topLeft.y    = i * scrRect.extent.y;
        rect.extent.x     = scrRect.extent.x;
        rect.extent.y     = scrRect.extent.y;

        // save this location, record parachuter is onscreen
        gbls->parachuteOnScreen[i][j]         = true;
        gbls->parachuteOnScreenPosition[i][j] = prefs->game.parashoot.parachutePosition[i][j];

        // draw the parachute bitmap!
        WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                         &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

        // dont draw until we need to
        gbls->parachuteChanged[i][j] = false;
      }
    }
  }

  // draw boat (only if it has changed)
  if (gbls->boatChanged) {

    // 
    // erase the previous boat
    // 

    if (gbls->boatOnScreen) {

      index = gbls->boatOldPosition;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteBoat, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 32;
      scrRect.extent.y  = 14;
      rect.topLeft.x    = index * scrRect.extent.x; 
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // invert the old boat bitmap
      WinCopyRectangle(gbls->winBoats, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->boatOnScreen    = false;
    }

    // 
    // draw boat at the new position
    // 

    index = prefs->game.parashoot.boatPosition;

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteBoat, index, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 32;
    scrRect.extent.y  = 14;
    rect.topLeft.x    = index * scrRect.extent.x; 
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // save this location, record boat is onscreen
    gbls->boatOnScreen    = true;
    gbls->boatOldPosition = index;

    // draw the boat bitmap!
    WinCopyRectangle(gbls->winBoats, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // dont draw until we need to
    gbls->boatChanged = false;
  }
}

/**
 * Get the position of a particular sprite on the screen.
 *
 * @param spriteType the type of sprite.
 * @param index the index required in the sprite position list.
 * @param x the x co-ordinate of the position
 * @param y the y co-ordinate of the position
 */
void
GameGetSpritePosition(UInt8 spriteType, 
                      UInt8 index, 
                      Coord *x, 
                      Coord *y)
{
  switch (spriteType) 
  {
    case spriteDigit: 
         {
           *x = 4 + (index * 9);
           *y = 38;
         }
         break;

    case spriteMiss: 
         {
           *x = 109;
           *y = 104;
         }
         break;

    case spriteBlade: 
         {
           *x = 119;
           *y = 40;
         }
         break;

    case spriteShark: 
         {
           Coord positions[][2] = {
                                   {  78, 118 },
                                   {  46, 118 },
                                   {  31, 130 },
                                   {  60, 130 },
                                   {  88, 122 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    case spriteBoat: 
         {
           Coord positions[][2] = {
                                   {  15, 100 },
                                   {  47, 100 },
                                   {  77, 100 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    case spriteParachuter: 
         {
           Coord positions[][2] = {
                                   { 124, 57 }, // 1st wave of falling
                                   { 112, 56 },
                                   {  96, 65 },
                                   {  91, 78 },
                                   {  89, 93 },
                                   {   0,  0 },
                                   {   0,  0 },
                                   { 111, 51 }, // 2nd wave of falling
                                   {  98, 53 },
                                   {  85, 53 },
                                   {  71, 65 },
                                   {  63, 77 },
                                   {  57, 92 },
                                   {   0,  0 },
                                   { 106, 42 }, // 3rd wave of falling
                                   {  88, 46 },
                                   {  74, 47 },
                                   {  60, 52 },
                                   {  45, 65 },
                                   {  34, 76 },
                                   {  26, 92 },
                                   { 124, 73 }, // hanging in space
                                   { 109, 73 },
                                   {   0,  0 },
                                   {   0,  0 },
                                   {   0,  0 },
                                   {   0,  0 },
                                   {   0,  0 }
                                  };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    case spriteParachuterDeath: 
         {
           Coord positions[][2] = {
                                   {  86, 116 },
                                   {  55, 115 },
                                   {  23, 115 },
                                   {  43, 129 },
                                   {  71, 129 },
                                   { 118, 125 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    default:
         break;
  }
}

/**
 * Terminate the game.
 */
void   
GameTerminate()
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // unlock the gamepad driver (if available)
  if (gbls->hardware.gamePadPresent) {

    Err    err;
    UInt32 gamePadUserCount;

    err = GPDClose(gbls->hardware.gamePadLibRef, &gamePadUserCount);
    if (gamePadUserCount == 0)
      SysLibRemove(gbls->hardware.gamePadLibRef);
  }

  // clean up windows/memory
  if (gbls->winDigits != NULL)
    WinDeleteWindow(gbls->winDigits,           false);
  if (gbls->winMisses != NULL)
    WinDeleteWindow(gbls->winMisses,           false);
  if (gbls->winBlades != NULL)
    WinDeleteWindow(gbls->winBlades,           false);
  if (gbls->winSharks != NULL)
    WinDeleteWindow(gbls->winSharks,           false);
  if (gbls->winBoats != NULL)
    WinDeleteWindow(gbls->winBoats,            false);
  if (gbls->winParachuters != NULL)
    WinDeleteWindow(gbls->winParachuters,      false);
  if (gbls->winParachuterDeaths != NULL)
    WinDeleteWindow(gbls->winParachuterDeaths, false);
  MemPtrFree(gbls);

  // unregister global data
  FtrUnregister(appCreator, ftrGameGlobals);
}

/**
 * Adjust the level (reset positions)
 *
 * @param prefs the global preference data.
 */
static void 
GameAdjustLevel(PreferencesType *prefs)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // player should stay were the were
  prefs->game.parashoot.boatNewPosition = prefs->game.parashoot.boatPosition;
  gbls->boatChanged                     = true;

  // player is not dead
  gbls->playerDied                      = false;
}

/**
 * Increment the players score. 
 *
 * @param prefs the global preference data.
 */
static void 
GameIncrementScore(PreferencesType *prefs)
{
  GameGlobals    *gbls;
  UInt16         i, index;
  RectangleType  rect     = { {   0,   0 }, {   0,   0 } };
  RectangleType  scrRect  = { {   0,   0 }, {   0,   0 } };
  SndCommandType scoreSnd = {sndCmdFreqDurationAmp,0,1024, 5,sndMaxAmp};

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // adjust accordingly
  prefs->game.gameScore += prefs->game.parashoot.bonusScoring ? 2 : 1;

  // redraw score bitmap
  {
    UInt16 base;
 
    base = 1000;  // max score (4 digits)
    for (i=0; i<4; i++) {

      index = (prefs->game.gameScore / base) % 10;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
      base /= 10;
    }
  }

  // play the sound
  DevicePlaySound(&scoreSnd);
  SysTaskDelay(5);

  // is it time for a bonus?
  if (
      (prefs->game.gameScore >= 300) &&
      (prefs->game.parashoot.bonusAvailable)
     ) {

    SndCommandType snd = {sndCmdFreqDurationAmp,0,0,5,sndMaxAmp};

    // give a little fan-fare sound
    for (i=0; i<15; i++) {
      snd.param1 += 256 + (1 << i);  // frequency
      DevicePlaySound(&snd);

      SysTaskDelay(2); // small deley 
    }

    // apply the bonus!
    if (prefs->game.gameLives == 3) 
      prefs->game.parashoot.bonusScoring = true;
    else
      prefs->game.gameLives = 3;

    prefs->game.parashoot.bonusAvailable = false;
  }

#ifdef PROTECTION_ON
  // is it time to say 'bye-bye' to our freeware guys?
  if (
      (prefs->game.gameScore >= 50) && 
      (!CHECK_SIGNATURE(prefs))
     ) {

    EventType event;

    // "please register" dialog :)
    ApplicationDisplayDialog(rbugForm);

    // GAME OVER - return to main screen
    MemSet(&event, sizeof(EventType), 0);
    event.eType            = menuEvent;
    event.data.menu.itemID = gameMenuItemExit;
    EvtAddEventToQueue(&event);

    // stop the game in its tracks
    prefs->game.gamePlaying = false;
  }
#endif
}

/**
 * Move the player.
 *
 * @param prefs the global preference data.
 */
static void
GameMovePlayer(PreferencesType *prefs) 
{
  GameGlobals    *gbls;
  SndCommandType plymvSnd = {sndCmdFreqDurationAmp,0, 768, 5,sndMaxAmp};

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  //
  // where does boat want to go today?
  //

  // current position differs from new position?
  if (prefs->game.parashoot.boatPosition != 
      prefs->game.parashoot.boatNewPosition) {

    // need to move left
    if (prefs->game.parashoot.boatPosition > 
        prefs->game.parashoot.boatNewPosition) {

      gbls->moveNext = moveLeft;
    }

    // need to move right
    else
    if (prefs->game.parashoot.boatPosition < 
        prefs->game.parashoot.boatNewPosition) {

      gbls->moveNext = moveRight;
    }
  }

  // lets make sure they are allowed to do the move
  if (
      (gbls->moveDelayCount == 0) || 
      (gbls->moveLast != gbls->moveNext) 
     ) {
    gbls->moveDelayCount = 
     ((gbls->gameType == GAME_A) ? 4 : 3);
  }
  else {
    gbls->moveDelayCount--;
    gbls->moveNext = moveNone;
  }

  // which direction do they wish to move?
  switch (gbls->moveNext)
  {
    case moveLeft:
         {
           prefs->game.parashoot.boatPosition--;
           gbls->boatChanged = true;
         }
         break;

    case moveRight:
         {
           prefs->game.parashoot.boatPosition++;
           gbls->boatChanged = true;
         }
         break;

    default:
         break;
  }

  gbls->moveLast = gbls->moveNext;
  gbls->moveNext = moveNone;

  // do we need to play a movement sound? 
  if (gbls->boatChanged)  
    DevicePlaySound(&plymvSnd);
}

/**
 * Move the parachuters.
 *
 * @param prefs the global preference data.
 */
static void
GameMoveParachuters(PreferencesType *prefs)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // only do this if the player is still alive
  if (!gbls->playerDied) {

    SndCommandType grdmvSnd = {sndCmdFreqDurationAmp,0, 384, 5,sndMaxAmp};
    UInt16         i, j, k, pos;

    // process the parachuters!
    for (i=0; i<3; i++) {

      // each row individually
      j = 0;
      while (j<prefs->game.parashoot.parachuteCount[i]) {

        Boolean removal = false;

        if (prefs->game.parashoot.parachuteWait[i][j] == 0) {

          UInt8 hangFactor = (gbls->gameType == GAME_A) ? 8 : 4;

          // can the parachuter get stuck?
          if (
              (i == 0) && 
              (prefs->game.parashoot.parachutePosition[i][j] == 1) &&
              (prefs->game.parashoot.parachuteCount[3] == 0)       &&
              ((SysRandom(0) % hangFactor) == 0)
             ) {

            // add the 'hanging' parachuter
            prefs->game.parashoot.parachuteCount[3]       = 1;
            prefs->game.parashoot.parachutePosition[3][0] = 0;
            prefs->game.parashoot.parachuteWait[3][0]     =
              (gbls->gameType == GAME_A) ? 6 : 5;
            gbls->parachuteChanged[3][0]          = true;
            gbls->parachuteOnScreen[3][0]         = false;
            gbls->parachuteOnScreenPosition[3][0] = 0;

            // remove the parachuter
            GameRemoveParachuter(prefs, i, j); removal = true;
          }
 
          // normal progression, down to the water
          else {
          
            Boolean ok;

            // lets make sure it is not moving into a parachuter in front of us?
            ok = true;
            for (k=0; k<prefs->game.parashoot.parachuteCount[i]; k++) {

              ok &= (
                     (prefs->game.parashoot.parachutePosition[i][j]+1 !=
                      prefs->game.parashoot.parachutePosition[i][k])
                    );
            }

            // the coast is clear, move!
            if (ok) {

              prefs->game.parashoot.parachutePosition[i][j]++;
              prefs->game.parashoot.parachuteWait[i][j] =
                (gbls->gameType == GAME_A) ? 6 : 4;
              gbls->parachuteChanged[i][j] = true;
 
              // has the parachuter fallen into the water?
              if (prefs->game.parashoot.parachutePosition[i][j] == 5+i) {

                gbls->playerDied |= true;
                prefs->game.parashoot.parachuteDeathPosition = i;

                // remove the parachuter
                GameRemoveParachuter(prefs, i, j); removal = true;
              }

              DevicePlaySound(&grdmvSnd);
            }
          }
        }
        else {
          prefs->game.parashoot.parachuteWait[i][j]--;

          pos = 2 - prefs->game.parashoot.boatPosition;

          // lets check if the parachuter is being saved by the boat?
          if (i == pos) {

            if (prefs->game.parashoot.parachutePosition[i][j] == (4 + pos)) {

              // increase score
              GameIncrementScore(prefs);

              // we need to remove the parachuter
              GameRemoveParachuter(prefs, i, j); removal = true;
            }
          }
        }

        if (!removal) j++;
      }
    }

    // process the hanging parachuter
    if (prefs->game.parashoot.parachuteCount[3] != 0) {

      if (prefs->game.parashoot.parachuteWait[3][0] == 0) {

        UInt8   freeFactor = (gbls->gameType == GAME_A) ? 4 : 3;
        Boolean ok;

        // primary condition = theres space, in right pos and RANDOM :P
        ok = (
              (prefs->game.parashoot.parachuteCount[0] < 5)        &&
              (prefs->game.parashoot.parachutePosition[3][0] == 1) &&
              ((SysRandom(0) % freeFactor) == 0)
             ); 

        // lets make sure no 'parachuter' is in the way of exiting?
        for (i=0; i<prefs->game.parashoot.parachuteCount[0]; i++) {
          ok &= (
                 (prefs->game.parashoot.parachutePosition[0][i] != 2) &&
                 (prefs->game.parashoot.parachutePosition[0][i] != 3)
                ); 
        }

        // can the parachuter get free?
        if (ok) {

          // we need to remove the hanging parachuter
          GameRemoveParachuter(prefs, 3, 0);

          // add a new parachuter to the first falling fleet
          pos = prefs->game.parashoot.parachuteCount[0]++;

          prefs->game.parashoot.parachutePosition[0][pos] = 3;
          prefs->game.parashoot.parachuteWait[0][pos]     = 
            (gbls->gameType == GAME_A) ? 6 : 4;
          gbls->parachuteChanged[0][pos]          = true;
          gbls->parachuteOnScreen[0][pos]         = false;
          gbls->parachuteOnScreenPosition[0][pos] = 0;

          DevicePlaySound(&grdmvSnd);
        }

        // keep swinging
        else {
          prefs->game.parashoot.parachutePosition[3][0] = 
            (prefs->game.parashoot.parachutePosition[3][0] + 1) % 2;
          prefs->game.parashoot.parachuteWait[3][0]     =
            (gbls->gameType == GAME_A) ? 6 : 5;
          gbls->parachuteChanged[3][0] = true;
        }
      }
      else {
        prefs->game.parashoot.parachuteWait[3][0]--;
      }
    }

    // new parachuter appearing on screen?
    {
      Boolean ok;
      UInt8   birthFactor            = (gbls->gameType == GAME_A) ? 8 : 4;
      UInt8   maxOnScreenParachuters = prefs->game.parashoot.gameLevel;
      UInt8   totalOnScreen, new;

      totalOnScreen = (prefs->game.parashoot.parachuteCount[0] + 
                       prefs->game.parashoot.parachuteCount[1] + 
                       prefs->game.parashoot.parachuteCount[2]);
      new = SysRandom(0) % 3;

      // we must be able to add a parachuter (based on level)
      ok = (
            (totalOnScreen < maxOnScreenParachuters) &&
            (prefs->game.parashoot.parachuteCount[new] < (5+new)) &&
            ((SysRandom(0) % birthFactor) == 0)
           );

      // lets check that there is no parachuter at index = 0;
      for (i=0; i<prefs->game.parashoot.parachuteCount[new]; i++) {
        ok &= (prefs->game.parashoot.parachutePosition[new][i] != 0);
      }

      // lets add a new parachuter
      if (ok) {
        pos = prefs->game.parashoot.parachuteCount[new]++;
        prefs->game.parashoot.parachutePosition[new][pos] = 0;
        prefs->game.parashoot.parachuteWait[new][pos]     = 
          (gbls->gameType == GAME_A) ? 6 : 4;
        gbls->parachuteChanged[new][pos]          = true;
        gbls->parachuteOnScreen[new][pos]         = false;
        gbls->parachuteOnScreenPosition[new][pos] = 0;

        DevicePlaySound(&grdmvSnd);
      }
    }
  }
}

/**
 * Remove a parachute from the game.
 *
 * @param prefs the global preference data.
 * @param rowIndex the index of the parachuter path.
 * @param parachuteIndex the index of the parachuter to remove.
 */
static void 
GameRemoveParachuter(PreferencesType *prefs, 
                     UInt16          rowIndex, 
                     UInt16          parachuteIndex)
{
  GameGlobals   *gbls;
  UInt16        index;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // 
  // remove the bitmap from the screen
  //
 
  if (gbls->parachuteOnScreen[rowIndex][parachuteIndex]) {

    index = (rowIndex * 7) +  // take into account 'layers'
            gbls->parachuteOnScreenPosition[rowIndex][parachuteIndex];

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteParachuter, index,
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 18;
    scrRect.extent.y  = 20;
    rect.topLeft.x    = (index % 7) * scrRect.extent.x;
    rect.topLeft.y    = rowIndex * scrRect.extent.y;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // invert the old parachute bitmap!
    WinCopyRectangle(gbls->winParachuters, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
  }

  //
  // update the information arrays
  //

  // we will push the 'parachute' out of the array
  //
  // before: 1234567---  after: 1345672---
  //          ^     |                 |
  //                end point         end point

  prefs->game.parashoot.parachuteCount[rowIndex]--;

  // removal NOT from end?
  if (prefs->game.parashoot.parachuteCount[rowIndex] > parachuteIndex) {

    UInt16 i, count;

    count = prefs->game.parashoot.parachuteCount[rowIndex] - parachuteIndex;

    // shift all elements down
    for (i=parachuteIndex; i<(parachuteIndex+count); i++) {
      prefs->game.parashoot.parachutePosition[rowIndex][i] = prefs->game.parashoot.parachutePosition[rowIndex][i+1];
      prefs->game.parashoot.parachuteWait[rowIndex][i]     = prefs->game.parashoot.parachuteWait[rowIndex][i+1];
      gbls->parachuteChanged[rowIndex][i]                  = gbls->parachuteChanged[rowIndex][i+1];
      gbls->parachuteOnScreen[rowIndex][i]                 = gbls->parachuteOnScreen[rowIndex][i+1];
      gbls->parachuteOnScreenPosition[rowIndex][i]         = gbls->parachuteOnScreenPosition[rowIndex][i+1];
    }
  }
}
