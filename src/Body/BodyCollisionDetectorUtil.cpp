/**
   \file
   \author Shin'ichiro Nakaoka
*/

#include "BodyCollisionDetectorUtil.h"
#include <cnoid/ValueTree>
#include <boost/dynamic_bitset.hpp>

using namespace std;
using namespace boost;
using namespace cnoid;


namespace {

void addLinkToCollisionDetector(Link* link, CollisionDetector& detector, bool isStatic)
{
    const int id = detector.addGeometry(link->shape());
    if(isStatic){
        if(link->isFixedJoint()){
            detector.setGeometryStatic(id);
        } else {
            isStatic = false;
        }
    }
    for(Link* child = link->child(); child; child = child->sibling()){
        addLinkToCollisionDetector(child, detector, isStatic);
    }
}
}


/**
   @return top of the geometry indices assigned to the body.
   The geometry id corresponding to a link is <the top index> + <link->index()>.
*/
int cnoid::addBodyToCollisionDetector(Body& body, CollisionDetector& detector, bool enableSelfCollisions)
{
    const int idTop = detector.numGeometries();
    const int numLinks = body.numLinks();
    int excludeTreeDepth = 3;
    dynamic_bitset<> exclusions(numLinks);
    
    const Mapping& cdInfo = *body.info()->findMapping("collisionDetection");
    if(cdInfo.isValid()){
        excludeTreeDepth = cdInfo.get("excludeTreeDepth", excludeTreeDepth);
        const Listing& excludeLinks = *cdInfo.findListing("excludeLinks");
        for(int i=0; i < excludeLinks.size(); ++i){
            Link* link = body.link(excludeLinks[i].toString());
            if(link){
                exclusions[link->index()] = true;
            }
        }
    }

    bool isStatic = (body.isStaticModel() || body.rootLink()->isFixedJoint());
    if(isStatic){
        addLinkToCollisionDetector(body.rootLink(), detector, isStatic);
    } else {
        for(int i=0; i < numLinks; ++i){
            if(exclusions[i]){
                detector.addGeometry(0);
            } else {
                detector.addGeometry(body.link(i)->shape());
            }
        }
    }

    if(!enableSelfCollisions){
        // exclude all the self link pairs
        for(int i=0; i < numLinks; ++i){
            if(!exclusions[i]){
                for(int j=i+1; j < numLinks; ++j){
                    if(!exclusions[j]){
                        detector.setNonInterfarenceGeometyrPair(i + idTop, j + idTop);
                    }
                }
            }
        }
    } else {
        // exclude the link pairs whose distance in the tree is less than excludeTreeDepth
        for(int i=0; i < numLinks; ++i){
            if(!exclusions[i]){
                Link* link1 = body.link(i);
                for(int j=i+1; j < numLinks; ++j){
                    if(!exclusions[j]){
                        Link* link2 = body.link(j);
                        Link* parent1 = link1;
                        Link* parent2 = link2;
                        for(int k=0; k < excludeTreeDepth; ++k){
                            if(parent1){
                                parent1 = parent1->parent();
                            }
                            if(parent2){
                                parent2 = parent2->parent();
                            }
                            if(!parent1 && !parent2){
                                break;
                            }
                            if(parent1 == link2 || parent2 == link1){
                                detector.setNonInterfarenceGeometyrPair(i + idTop, j + idTop);
                            }
                        }
                    }
                }
            }
        }
    }

    return idTop;
}
