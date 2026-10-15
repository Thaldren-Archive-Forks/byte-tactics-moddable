// LosTables: the line-of-sight table set loaded from gamedata\los.tdf (its
// numtables / TABLEINFO / numlines / TABLE%d sections): the vector of tables at
// +0 and the methods that load and walk it. The one declaration of the class
// for the files that see or call it; the element types behind the vector and
// the TDF parser stay private to the files that build them.
#ifndef LOS_TABLES_H
#define LOS_TABLES_H

// The vector type of the member at +0, declared only: the files that build the
// tables define it.
struct Elem_00434020;
#ifdef __clang__
namespace std {
    template<class T> class allocator;
    template<class T, class A> class vector;
}
#define LOS_ELEM_VECTOR std::vector<Elem_00434020, std::allocator<Elem_00434020> >
#else
namespace std {
    template<class T> class allocator;
    template<class T, class A = allocator<T> > class vector;
}
#define LOS_ELEM_VECTOR std::vector<Elem_00434020>
#endif

class LosTable;
class TdfFile;

class LosTables {
public:
    void FreeTables();
    void LoadLosTable(TdfFile* file, short table);
    LOS_ELEM_VECTOR* GetLosTable(int n);
    int GetLosTableCount();
    void LoadLosTables();
    void ResizeTables(short n);
};

#endif
