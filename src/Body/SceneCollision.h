/**
   @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_BODY_SCENE_COLLISION_H_INCLUDED
#define CNOID_BODY_SCENE_COLLISION_H_INCLUDED

#include "CollisionLinkPair.h"
#include <cnoid/SceneShape>
#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT SceneCollision : public SgLineSet
{
public:
    SceneCollision(boost::shared_ptr< std::vector<CollisionLinkPairPtr> > collisionPairs);

    void setDirty() { isDirty = true; }

    virtual void accept(SceneVisitor& visitor);

private:
    SceneCollision(const SceneCollision& org);

    boost::shared_ptr< std::vector<CollisionLinkPairPtr> > collisionPairs;
    SgVertexArrayPtr vertices_;
    bool isDirty;
};
    
typedef ref_ptr<SceneCollision> SceneCollisionPtr;
}

#endif
