#ifndef SQUAD_TIMER_H
#define SQUAD_TIMER_H

class SquadManager;
struct Group;

class SquadTimer {
public:
    SquadManager* owner;
    Group* group;
    int next;
    unsigned int player;

    SquadTimer(SquadManager* p, Group* q);
    virtual void OnTimer();
    virtual ~SquadTimer() {}
    int TryGetBaseBuildOrScoutCentroid(Vec3* out);
    int GetAveragePosition(Vec3* out);
    int CountGroupUnitsInRadius(Vec3* pos, int radius);
};

class AssaultTimer : public SquadTimer {
public:
    int minimum;
    int maximum;
    int limit;
    int kind;
    int attacking;

    AssaultTimer(SquadManager* p, Group* q, int a, int b);
    virtual void OnTimer();
    void RebalanceAssaultGroupByCentroid(int kind, int limit);
};

class EscortTimer : public SquadTimer {
public:
    int other;

    EscortTimer(SquadManager* p, Group* q, int a);
    virtual void OnTimer();
};

class SquadScoutTimer : public SquadTimer {
public:
    SquadScoutTimer(SquadManager* p, Group* q);
    virtual void OnTimer();
};

class ScoutTimer : public SquadTimer {
public:
    ScoutTimer(SquadManager* p, Group* q);
    virtual void OnTimer();
};

class BuildTimer : public SquadTimer {
public:
    BuildTimer(SquadManager* p, Group* q);
    virtual void OnTimer();
};

class SpatialTimer : public SquadTimer {
public:
    Vec3 best;
    Vec3 probe;
    Vec3 step;
    int bestRating;

    SpatialTimer(SquadManager* p, void* q);
    virtual void OnTimer();
};

#endif
