/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CubeWrapper.h"
#include <sifteo.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const int kBgSize = 128;
const int kScale = 128 / kBgSize;

const int kPartSideRegions[4][4] =
{
    { 40 / kScale,           0, 48 / kScale, 48 / kScale },
    {           0, 40 / kScale, 48 / kScale, 48 / kScale },
    { 40 / kScale, 80 / kScale, 48 / kScale, 48 / kScale },
    { 80 / kScale, 40 / kScale, 48 / kScale, 48 / kScale },
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Sifteo::AssetImage &getBgAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return bg1;
        case 1: return bg2;
        case 2: return bg3;
        case 3: return bg4;
        case 4: return bg5;
        case 5: return bg6;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Sifteo::AssetImage &getPartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return parts1;
        case 1: return parts2;
        case 2: return parts3;
        case 3: return parts4;
        case 4: return parts5;
        case 5: return parts6;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper::CubeWrapper()
    : mCube()
    , mEnabled(false)
    , mBuddyId(0)
    , mPieces()
    , mPiecesSolution()
    , mMode(BUDDY_MODE_NORMAL)
    , mTouching(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Reset()
{
    mPiecesSolution[0].mBuddy = mBuddyId;
    mPiecesSolution[0].mPart = 0;
    mPiecesSolution[0].mRotation = 0;
    
    mPiecesSolution[1].mBuddy = mBuddyId;
    mPiecesSolution[1].mPart = 1;
    mPiecesSolution[1].mRotation = 0;
    
    mPiecesSolution[2].mBuddy = mBuddyId;
    mPiecesSolution[2].mPart = 2;
    mPiecesSolution[2].mRotation = 0;
    
    mPiecesSolution[3].mBuddy = mBuddyId;
    mPiecesSolution[3].mPart = 3;
    mPiecesSolution[3].mRotation = 0;
    
    for (unsigned int i = 0; i < arraysize(mPiecesSolution); ++i)
    {
        mPieces[i] = mPiecesSolution[i];
    }
    
    mMode = BUDDY_MODE_NORMAL;
    mTouching = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Paint()
{
    ASSERT(IsEnabled());
    
    Video().clear();
    Video().BG0_drawAsset(Vec2(0, 0), getBgAsset(mBuddyId));
    
    BG1Helper bg1helper(mCube);
    
    if (mMode == BUDDY_MODE_HINT)
    {
        PaintPiece(bg1helper, mPiecesSolution[SIDE_TOP], SIDE_TOP);
        PaintPiece(bg1helper, mPiecesSolution[SIDE_LEFT], SIDE_LEFT);
        PaintPiece(bg1helper, mPiecesSolution[SIDE_BOTTOM], SIDE_BOTTOM);
        PaintPiece(bg1helper, mPiecesSolution[SIDE_RIGHT], SIDE_RIGHT);
    }
    else
    {
        PaintPiece(bg1helper, mPieces[SIDE_TOP], SIDE_TOP);
        PaintPiece(bg1helper, mPieces[SIDE_LEFT], SIDE_LEFT);
        PaintPiece(bg1helper, mPieces[SIDE_BOTTOM], SIDE_BOTTOM);
        PaintPiece(bg1helper, mPieces[SIDE_RIGHT], SIDE_RIGHT);
    }
    
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Tick()
{
    ASSERT(IsEnabled());
    
    if (mTouching)
    {
        if (!mCube.touching())
        {
            OnButtonRelease();
            mTouching = false;
        }
    }
    else
    {
        if (mCube.touching())
        {
            OnButtonPress();
            mTouching = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsSolved() const
{
    ASSERT(IsEnabled());
    
    for (unsigned int i = 0; i < arraysize(mPiecesSolution); ++i)
    {
        if (mPieces[i].mBuddy != mPiecesSolution[i].mBuddy ||
            mPieces[i].mPart != mPiecesSolution[i].mPart ||
            mPieces[i].mRotation != mPiecesSolution[i].mRotation)
        {
            return false;
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::InitVideoRom()
{
    ASSERT(IsEnabled());
    
    VidMode_BG0_ROM rom(mCube.vbuf);
    rom.init();
    rom.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::PaintProgressBar()
{
    ASSERT(IsEnabled());
    
    VidMode_BG0_ROM rom(mCube.vbuf);
    rom.BG0_progressBar(Vec2(0, 7), mCube.assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsDoneLoading()
{
    ASSERT(IsEnabled());
    
    return mCube.assetDone(GameAssets);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsEnabled() const
{
    return mEnabled;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Enable(Cube::ID cubeId, unsigned int buddyId)
{
    ASSERT(!IsEnabled());
    
    mCube.enable(cubeId);
    mCube.loadAssets(GameAssets);
            
    mEnabled = true;
    mBuddyId = buddyId;
    
    Reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Disable()
{
    ASSERT(IsEnabled());
    
    mEnabled = false;
    mCube.disable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CubeWrapper::GetBuddyId() const
{
    return mBuddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Piece &CubeWrapper::GetPiece(unsigned int side) const
{
    ASSERT(side < arraysize(mPieces));
    
    return mPieces[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPiece(unsigned int side, const Piece &piece)
{
    ASSERT(side < arraysize(mPieces));
    
    mPieces[side] = piece;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

BuddyMode CubeWrapper::GetMode() const
{
    return mMode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsTouching() const
{
    return mTouching;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Sifteo::VidMode_BG0_SPR_BG1 CubeWrapper::Video()
{
    return VidMode_BG0_SPR_BG1(mCube.vbuf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::PaintPiece(BG1Helper &bg1Helper, const Piece &piece, unsigned int side)
{
    ASSERT(piece.mPart >= 0 && piece.mPart < NUM_SIDES);
    ASSERT(piece.mRotation >= 0 && piece.mRotation < 4);
    
    Vec2 point = Vec2(kPartSideRegions[side][0] * kScale / 8, kPartSideRegions[side][1] * kScale / 8);
    const Sifteo::AssetImage &asset = getPartsAsset(piece.mBuddy);
    unsigned int frame = (piece.mRotation * NUM_SIDES) + piece.mPart;
    
    ASSERT(frame < asset.frames);
    bg1Helper.DrawAsset(point, asset, frame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::OnButtonPress()
{
    mMode = BUDDY_MODE_HINT;
    Paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::OnButtonRelease()
{
    mMode = BUDDY_MODE_NORMAL;
    Paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
