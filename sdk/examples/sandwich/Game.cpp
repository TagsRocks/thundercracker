#include "Game.h"

static bool sNeighborDirty = false;

static void onNeighbor(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {
  sNeighborDirty = true;
}

static void onTouch(_SYSCubeID cid) {    
    pGame->ViewAt(cid)->touched = true;
}



void Game::ObserveNeighbors(bool flag) {
  if (flag) {
    _SYS_vectors.neighborEvents.add = onNeighbor;
    _SYS_vectors.neighborEvents.remove = onNeighbor;
  } else {
    _SYS_vectors.neighborEvents.add = 0;
    _SYS_vectors.neighborEvents.remove = 0;
  }
}

//------------------------------------------------------------------
// BOOTSTRAP API
//------------------------------------------------------------------

Game::Game() {
  const RoomData& room = gMapData[gQuestData->mapId].rooms[gQuestData->roomId];
  player.SetPosition(Vec2(
    128 * (gQuestData->roomId % gMapData[gQuestData->mapId].width) + 16 * room.centerx,
    128 * (gQuestData->roomId / gMapData[gQuestData->mapId].width) + 16 * room.centery
  ));
}

void Game::MainLoop() {
  // reset everything
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    v->Init();
  }
  _SYS_vectors.cubeEvents.touch = onTouch;

  // initial zoom out (yoinked and modded from TeleportTo)
  { 
    gChannelMusic.stop();
    PlaySfx(sfx_zoomIn);
    GameView* view = player.CurrentView();
    view->HidePlayer();
    Vec2 room = player.Location();
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(map.Data()->tileset),
          map.GetTileId(room, Vec2(x, y))
        );
      }
    }
    vid.BG2_setBorder(0x0000);
    vid.set();
    for (float t = 1.0f; t > 0.0f; t -= 0.05f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f + 9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }    
    System::paintSync();
    view->Init();
    System::paintSync();
  }  
  PlayMusic(music_castle);
  //PlaySfx(sfx_neighbor);

  mSimTime = System::clock();
  ObserveNeighbors(true);
  CheckMapNeighbors();

  mIsDone = false;
  mNeedsSync = 0;
  while(!mIsDone) {
    float dt = UpdateDeltaTime();
    if (sNeighborDirty) { 
      CheckMapNeighbors(); 
    }
    player.Update(dt);
    Paint();
  }
}

void Game::Paint(bool sync) {
  for(GameView *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->Update();
    p->touched = false;
    #ifdef KLUDGES
    p->GetCube()->vbuf.touch();
    #endif
  }
  if (sync || mNeedsSync) {
    System::paintSync();
    if (mNeedsSync > 0) {
      mNeedsSync--;
    }
  } else {
    System::paint();
  }
}

float Game::UpdateDeltaTime() {
  float now = System::clock();
  float result = now - mSimTime;
  mSimTime = now;
  return result;
}

void Game::MovePlayerAndRedraw(int dx, int dy) {
  player.Move(dx, dy);
  player.UpdateAnimation(UpdateDeltaTime());
  player.CurrentView()->UpdatePlayer();
  System::paint();
}

void Game::WalkTo(Vec2 position) {
  PlaySfx(sfx_running);
  Vec2 delta = position - player.Position();
  while(delta.x > WALK_SPEED) {
    MovePlayerAndRedraw(WALK_SPEED, 0);
    delta.x -= WALK_SPEED;
  }
  while(delta.x < -WALK_SPEED) {
    MovePlayerAndRedraw(-WALK_SPEED, 0);
    delta.x += WALK_SPEED;
  }
  if (delta.x != 0) {
    MovePlayerAndRedraw(delta.x, 0);
  }
  while(delta.y > WALK_SPEED) {
    MovePlayerAndRedraw(0, WALK_SPEED);
    delta.y -= WALK_SPEED;
  }
  while(delta.y < WALK_SPEED) {
    MovePlayerAndRedraw(0, -WALK_SPEED);
    delta.y += WALK_SPEED;
  }
  if (delta.y != 0) {
    MovePlayerAndRedraw(0, delta.y);
  }
}

void Game::TeleportTo(const MapData& m, Vec2 position) {
  gChannelMusic.stop();
  Vec2 room = position/128;
  GameView* view = player.CurrentView();
  view->HidePlayer();
  // blank other cubes
  for(GameView* p = ViewBegin(); p != ViewEnd(); ++p) {
    if (p != view) { p->HideRoom(); }
  }
  // zoom in
  { 
    System::paintSync();
    PlaySfx(sfx_zoomOut);
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(map.Data()->tileset),
          map.GetTileId(view->Location(), Vec2(x, y))
        );
      }
    }
    vid.BG2_setBorder(0x0000);
    vid.set();
    for (float t = 0; t < 1.0f; t += 0.05f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f+9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }
  }
  map.SetData(m);
  for(GameView* p = ViewBegin(); p!= ViewEnd(); ++p) {
    if (p != view) { p->DrawBackground(); }
  }
  // zoom out
  { 
    PlaySfx(sfx_zoomIn);
    VidMode_BG2 vid(view->GetCube()->vbuf);
    for(int x=0; x<8; ++x) {
      for(int y=0; y<8; ++y) {
        vid.BG2_drawAsset(
          Vec2(x<<1,y<<1),
          *(map.Data()->tileset),
          map.GetTileId(room, Vec2(x, y))
        );
      }
    }
    for (float t = 1.0f; t > 0.0f; t -= 0.05f) {
      AffineMatrix m = AffineMatrix::identity();
      m.translate(64, 64);
      m.scale(1.f+9.f*t);
      m.rotate(t * 1.1f);
      m.translate(-64, -64);
      vid.BG2_setMatrix(m);
      System::paint();
    }    

    System::paintSync();
  }
  
  PlayMusic(map.Data() == &gMapData[1] ? music_dungeon : music_castle);

  // walk out of the in-gate
  Vec2 target = map.GetRoom(room)->Center();
  player.SetLocation(position, InferDirection(target - position));
  view->Init();
  WalkTo(target);
  CheckMapNeighbors();

  // clear out any accumulated time
  UpdateDeltaTime();
}

//------------------------------------------------------------------
// MISC EVENTS
//------------------------------------------------------------------

void Game::OnInventoryChanged() {
  for(GameView *p=ViewBegin(); p!=ViewEnd(); ++p) {
    p->RefreshInventory();
  }
  #ifndef FAST_FORWARD
  const int firstSandwichId = 2;
  int count = 0;
  for(int i=firstSandwichId; i<firstSandwichId+4; ++i) {
    if(!player.HasItem(i)) {
      return;
    }
  }
  #endif
  mIsDone = true;
}

//------------------------------------------------------------------
// NEIGHBOR HANDLING
//------------------------------------------------------------------

static void VisitMapView(GameView* view, Vec2 loc, GameView* origin=0) {
  if (!view || view->visited) { return; }
  view->visited = true;
  if (view->ShowLocation(loc)) {
    PlaySfx(sfx_neighbor);
  }
  if (origin) {
    view->GetCube()->orientTo(*(origin->GetCube()));
  }
  for(Cube::Side i=0; i<NUM_SIDES; ++i) {
    VisitMapView(view->VirtualNeighborAt(i), loc+kSideToUnit[i], view);
  }
}

void Game::CheckMapNeighbors() {
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    v->visited = false;
  }
  VisitMapView(player.KeyView(), player.KeyView()->Location());
  for(GameView* v = ViewBegin(); v!=ViewEnd(); ++v) {
    if (!v->visited) { 
      if (v->HideRoom()) {
        PlaySfx(sfx_deNeighbor);
      }
    }
  }
  sNeighborDirty = false;
}
