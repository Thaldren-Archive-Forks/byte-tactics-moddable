#ifndef UNIT_SCRIPT_H
#define UNIT_SCRIPT_H

#include "cob_script.h"

struct ObjectState;

class UnitScript : public CobScript {
public:
    ObjectState* state;

    virtual void SetPieceTranslation(int index, int slot, int v);
    virtual void SetPieceRotation(int index, int slot, int v);
    virtual void SetPieceVisible(int index, int flag);
    virtual void SetPieceCached(int index, int flag);
    virtual void SetPieceShaded(int index, int flag);
    virtual int GetPieceTranslation(int index, int slot);
    virtual int GetPieceRotation(int index, int slot);
    virtual int IsPieceVisible(int index);
    virtual int IsPieceCached(int index);
    virtual int IsPieceShaded(int index);
    virtual void ExplodeLegacy(int, int, int);
    virtual void PlaySoundNoop(int);
    virtual void EmitSfx(int a, int b);
    virtual void ExplodePiece(int a, unsigned int b);
    virtual void AttachUnit(unsigned short id, int a, int b);
    virtual void DropUnit(unsigned short id);
    virtual void SetUnitValue(int which, int value);
    virtual int GetUnitValue(int which, int a, int b, int c, int d);
    virtual int IsCarryingUnit(int id);
    virtual int GetTransporterId();
    void SetObjectState(int newState);
};

#endif
