#ifndef DILAY_OCTREE
#define DILAY_OCTREE

#include <unordered_set>
#include <glm/fwd.hpp>
#include "iterator.hpp"
#include "macro.hpp"

class WingedFace;
class WingedVertex;
class WingedMesh;
class Triangle;
class OctreeNode;
class OctreeNodeFaceIterator;
class ConstOctreeNodeFaceIterator;
class OctreeFaceIterator;
class ConstOctreeFaceIterator;
class OctreeNodeIterator;
class ConstOctreeNodeIterator;
class Ray;
class FaceIntersection;
class Sphere;
class Id;

/** Internal template for iterators over all faces of a node */
template <bool> struct OctreeNodeFaceIteratorTemplate;

/** Internal template for iterators over all faces of an octree */
template <bool> struct OctreeFaceIteratorTemplate;

/** Internal template for iterators over all nodes of an octree */
template <bool> struct OctreeNodeIteratorTemplate;

/** Octree node interface */
class OctreeNode {
  public: 
    class Impl;

    OctreeNode            (Impl*);
    DELETE_COPYMOVEASSIGN (OctreeNode)

    Id                id              () const;
    int               depth           () const;
    const glm::vec3&  center          () const;
    float             looseWidth      () const;
    float             width           () const;
    void              intersectRay    (WingedMesh&, const Ray&, FaceIntersection&);
    void              intersectSphere ( const WingedMesh&, const Sphere&
                                      , std::unordered_set<Id>&);
    void              intersectSphere ( const WingedMesh&, const Sphere&
                                      , std::unordered_set<WingedVertex*>&);
    unsigned int      numFaces        () const;
    OctreeNode*       nodeSLOW        (const Id&);

    OctreeNodeFaceIterator      faceIterator ();
    ConstOctreeNodeFaceIterator faceIterator () const;

  private:
    friend class Octree;
    Impl* impl;
};

/** Octree main class */
class Octree { 
  public: 
    class Impl; 
    
    DECLARE_BIG3 (Octree)

    WingedFace& insertFace      (const WingedFace&, const Triangle&);
    WingedFace& realignFace     (const WingedFace&, const Triangle&, bool* = nullptr);
    void        deleteFace      (const WingedFace&);
    bool        hasFace         (const Id&) const;
    WingedFace* face            (const Id&);
    void        render          ();
    void        intersectRay    (WingedMesh&, const Ray&, FaceIntersection&);
    void        intersectSphere ( const WingedMesh&, const Sphere&
                                , std::unordered_set<Id>&);
    void        intersectSphere ( const WingedMesh&, const Sphere&
                                , std::unordered_set<WingedVertex*>&);
    void        reset           ();
    void        initRoot        (const glm::vec3&, float);
    void        shrinkRoot      ();
    OctreeNode& nodeSLOW        (const Id&);

    OctreeFaceIterator      faceIterator ();
    ConstOctreeFaceIterator faceIterator () const;
    OctreeNodeIterator      nodeIterator ();
    ConstOctreeNodeIterator nodeIterator () const;

    SAFE_REF1 (WingedFace,face,const Id&)

  private:
    Impl* impl;
};

/** Iterator over all faces of a node */
class OctreeNodeFaceIterator : public Iterator <WingedFace> {
  public: 
    DECLARE_BIG6 (OctreeNodeFaceIterator, OctreeNode::Impl&)

    bool         isValid () const;
    WingedFace&  element () const;
    void         next    ();
    int          depth   () const;

    using Impl = OctreeNodeFaceIteratorTemplate <false>;
  private:
    Impl* impl;
};

/** Constant iterator over all faces of a node */
class ConstOctreeNodeFaceIterator : public ConstIterator <WingedFace> {
  public: 
    DECLARE_BIG6 (ConstOctreeNodeFaceIterator, const OctreeNode::Impl&)

    bool              isValid () const;
    const WingedFace& element () const;
    void              next    ();
    int               depth   () const;

    using Impl = OctreeNodeFaceIteratorTemplate <true>;
  private:
    Impl* impl;
};

/** Iterator over all faces of an octree */
class OctreeFaceIterator : public Iterator <WingedFace> {
  public: 
    DECLARE_BIG6 (OctreeFaceIterator, Octree::Impl&)

    bool         isValid () const;
    WingedFace&  element () const;
    void         next    ();
    int          depth   () const;

  private:
    using Impl = OctreeFaceIteratorTemplate <false>;
    Impl* impl;
};

/** Constant iterator over all faces of an octree */
class ConstOctreeFaceIterator : public ConstIterator <WingedFace> {
  public: 
    DECLARE_BIG6 (ConstOctreeFaceIterator, const Octree::Impl&)
     
    bool              isValid () const;
    const WingedFace& element () const;
    void              next    ();
    int               depth   () const;

  private:
    using Impl = OctreeFaceIteratorTemplate <true>;
    Impl* impl;
};

/** Iterator over all nodes of an octree */
class OctreeNodeIterator : public Iterator <OctreeNode> {
  public: 
    DECLARE_BIG6 (OctreeNodeIterator, Octree::Impl&)

    bool         isValid () const;
    OctreeNode&  element () const;
    void         next    ();

  private:
    using Impl = OctreeNodeIteratorTemplate <false>;
    Impl* impl;
};

/** Constant iterator over all nodes of an octree */
class ConstOctreeNodeIterator : public ConstIterator <OctreeNode> {
  public: 
    DECLARE_BIG6 (ConstOctreeNodeIterator, const Octree::Impl&)

    bool              isValid () const;
    const OctreeNode& element () const;
    void              next    ();

  private:
    using Impl = OctreeNodeIteratorTemplate <true>;
    Impl* impl;
};

#endif
