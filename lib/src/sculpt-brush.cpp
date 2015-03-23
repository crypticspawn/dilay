#include <glm/gtx/norm.hpp>
#include "affected-faces.hpp"
#include "intersection.hpp"
#include "primitive/plane.hpp"
#include "primitive/sphere.hpp"
#include "sculpt-brush.hpp"
#include "util.hpp"
#include "winged/mesh.hpp"
#include "winged/util.hpp"
#include "winged/vertex.hpp"
#include "variant.hpp"

SBMoveDirectionalParameters::SBMoveDirectionalParameters ()
  : _intensityFactor     (0.0f)
  , _innerRadiusFactor   (0.0f)
  , _invert              (false)
  , _direction           (0.0f)
  , _useAverageDirection (true)
  , _useLastPosition     (false)
  , _useIntersection     (false)
  , _linearStep          (false)
{}

SBSmoothParameters::SBSmoothParameters ()
  : _relaxOnly (false)
  , _intensity (0.0f)
{}

SBFlattenParameters::SBFlattenParameters ()
  : _intensity (0.0f)
{}

struct SculptBrush :: Impl {
  SculptBrush*  self;
  float         radius;
  float         detailFactor;
  float         stepWidthFactor;
  bool          subdivide;
  WingedMesh*   mesh;
  WingedFace*   face;
  bool          hasPosition;
  glm::vec3    _lastPosition;
  glm::vec3    _position;

  Variant < SBMoveDirectionalParameters
          , SBSmoothParameters
          , SBFlattenParameters > parameters;

  Impl (SculptBrush* s) 
    : self            (s)
    , radius          (0.0f)
    , detailFactor    (0.0f)
    , stepWidthFactor (0.0f)
    , subdivide       (false)
    , hasPosition     (false)

  {}

  void sculpt (AffectedFaces& faces) const {
    assert (this->parameters.isSet ());

    this->parameters.caseOf <void>
      ( [this,&faces] (const SBMoveDirectionalParameters& p) {
          this->sculpt (p, faces); 
        }
      , [this,&faces] (const SBSmoothParameters& p) {
          this->sculpt (p, faces);
        }
      , [this,&faces] (const SBFlattenParameters& p) {
          this->sculpt (p, faces);
        }
      );
  }

  void sculpt (const SBMoveDirectionalParameters& parameters, AffectedFaces& faces) const {
    auto getSculptDirection = [this,&parameters] (const VertexPtrSet& vertices) -> glm::vec3 {
      if (parameters.useAverageDirection ()) {
        const glm::vec3 avgNormal = WingedUtil::averageNormal ( this->self->meshRef ()
                                                              , vertices);
        return parameters.invert () ? -avgNormal
                                    :  avgNormal;
      }
      else {
        return parameters.invert () ? -parameters.direction ()
                                    :  parameters.direction ();
      }
    };

    const glm::vec3 position = parameters.useLastPosition () ? this->lastPosition ()
                                                             : this->position     ();

    float (*stepFunction) (const glm::vec3&, const glm::vec3&, float, float) =
      parameters.linearStep () ? Util::linearStep : Util::smoothStep;

    PrimSphere  sphere (position, this->radius);
    WingedMesh& mesh   (this->self->meshRef ());

    if (parameters.useIntersection ()) {
      mesh.intersects (sphere, faces);
    }
    else {
      IntersectionUtil::extend (sphere, mesh, this->self->faceRef (), faces);
    }

    VertexPtrSet    vertices (faces.toVertexSet ());
    const glm::vec3 dir      (getSculptDirection (vertices));

    for (WingedVertex* v : vertices) {
      const glm::vec3 oldPos      = v->position (mesh);
      const float     intensity   = parameters.intensityFactor   () * this->radius;
      const float     innerRadius = parameters.innerRadiusFactor () * this->radius;
      const float     delta       = intensity
                                  * stepFunction ( oldPos, position
                                                 , innerRadius, this->radius );

      const glm::vec3 newPos = oldPos + (delta * dir);

      v->writePosition (mesh, newPos);
    }
  }

  void sculpt (const SBSmoothParameters& parameters, AffectedFaces& faces) const {
    PrimSphere  sphere (this->_position, this->radius);
    WingedMesh& mesh   (this->self->meshRef ());

    IntersectionUtil::extend ( sphere, this->self->meshRef ()
                             , this->self->faceRef (), faces );

    if (parameters.relaxOnly () == false) {
      VertexPtrSet vertices (faces.toVertexSet ());

      for (WingedVertex* v : vertices) {
        const glm::vec3 oldPos = v->position (mesh);
        const float     delta  = parameters.intensity ()
                               * Util::smoothStep ( oldPos, this->position ()
                                                  , 0.0f, this->radius);

        const glm::vec3 newPos = oldPos + (delta * (WingedUtil::center (mesh, *v) - oldPos));

        v->writePosition (mesh, newPos);
      }
    }
  }

  void sculpt (const SBFlattenParameters& parameters, AffectedFaces& faces) const {
    PrimSphere  sphere (this->_position, this->radius);
    WingedMesh& mesh   (this->self->meshRef ());

    IntersectionUtil::extend (sphere, mesh, this->self->faceRef (), faces);

    VertexPtrSet    vertices (faces.toVertexSet ());
    const glm::vec3 normal   (WingedUtil::averageNormal (mesh, vertices));
    const PrimPlane plane    ( WingedUtil::center (mesh, vertices)
                             , normal );

    for (WingedVertex* v : vertices) {
      const glm::vec3 oldPos   = v->position (mesh);
      const float     factor   = parameters.intensity ()
                               * Util::linearStep ( oldPos, this->position ()
                                                  , 0.0f, this->radius );
      const float     distance = glm::max (0.0f, plane.distance (oldPos));

      const glm::vec3 newPos = oldPos - (normal * factor * distance);

      v->writePosition (mesh, newPos);
    }
  }

  float subdivThreshold () const {
    return (1.0f - this->detailFactor) * this->radius;
  }

  const glm::vec3& lastPosition () const {
    assert (this->hasPosition);
    return this->_lastPosition;
  }

  const glm::vec3& position () const {
    assert (this->hasPosition);
    return this->_position;
  }

  glm::vec3 delta () const {
    assert (this->hasPosition);
    return this->_position - this->_lastPosition;
  }

  void setPosition (const glm::vec3& p) {
    this->hasPosition   = true;
    this->_lastPosition = p;
    this->_position     = p;
  }

  bool updatePosition (const glm::vec3& p) {
    if (this->hasPosition) {
      const float stepWidth = this->stepWidthFactor * this->self->radius ();

      if (glm::distance2 (p, this->_position) > stepWidth * stepWidth) {
        this->_lastPosition = this->_position;
        this->_position     = p;
        return true;
      }
      else {
        return false;
      }
    }
    else {
      this->setPosition (p);
      return true;
    }
  }

  void resetPosition () {
    this->hasPosition = false;
  }
};

DELEGATE_BIG6_SELF (SculptBrush)
  
DELEGATE1_CONST (void             , SculptBrush, sculpt, AffectedFaces&)
GETTER_CONST    (float            , SculptBrush, radius)
GETTER_CONST    (float            , SculptBrush, detailFactor)
GETTER_CONST    (float            , SculptBrush, stepWidthFactor)
GETTER_CONST    (bool             , SculptBrush, subdivide)
GETTER_CONST    (WingedMesh*      , SculptBrush, mesh)
GETTER_CONST    (WingedFace*      , SculptBrush, face)
SETTER          (float            , SculptBrush, radius)
SETTER          (float            , SculptBrush, detailFactor)
SETTER          (float            , SculptBrush, stepWidthFactor)
SETTER          (bool             , SculptBrush, subdivide)
SETTER          (WingedMesh*      , SculptBrush, mesh)
SETTER          (WingedFace*      , SculptBrush, face)
DELEGATE_CONST  (float            , SculptBrush, subdivThreshold)
GETTER_CONST    (bool             , SculptBrush, hasPosition)
DELEGATE_CONST  (const glm::vec3& , SculptBrush, lastPosition)
DELEGATE_CONST  (const glm::vec3& , SculptBrush, position)
DELEGATE_CONST  (glm::vec3        , SculptBrush, delta)
DELEGATE1       (void             , SculptBrush, setPosition, const glm::vec3&)
DELEGATE1       (bool             , SculptBrush, updatePosition, const glm::vec3&)
DELEGATE        (void             , SculptBrush, resetPosition)

template <typename T> 
const T& SculptBrush::constParameters () const {
  return this->impl->parameters.get <T> ();
}


template <typename T>
T& SculptBrush::parameters () {
  if (this->impl->parameters.is <T> () == false) {
    this->impl->parameters.init <T> ();
  }
  return this->impl->parameters.get <T> ();
}

template const SBMoveDirectionalParameters& 
SculptBrush::constParameters <SBMoveDirectionalParameters> () const;
template SBMoveDirectionalParameters& 
SculptBrush::parameters <SBMoveDirectionalParameters> ();

template const SBSmoothParameters& 
SculptBrush::constParameters <SBSmoothParameters> () const;
template SBSmoothParameters& 
SculptBrush::parameters <SBSmoothParameters> ();

template const SBFlattenParameters& 
SculptBrush::constParameters <SBFlattenParameters> () const;
template SBFlattenParameters& 
SculptBrush::parameters <SBFlattenParameters> ();
