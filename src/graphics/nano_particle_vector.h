#ifndef NANO_PARTICLE_VECTOR_H
#define NANO_PARTICLE_VECTOR_H

class NanoParticleVector {
public:
    union { int unknown_0; int alloc; };
    union { int field_4; int _First; };
    union { int field_8; int _Last; };

    int GetCount();
};

#endif
